# syntax=docker/dockerfile:1.7

# ---------- build stage ----------
# Compiles the C server and runs webpack to produce the JS + lua bundle.
# node:20-bookworm-slim = Debian bookworm + Node 20. Bookworm's own nodejs
# package is v18, which is missing Array.prototype.toSorted that newer
# webpack plugins require.
FROM node:20-bookworm-slim AS build

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates \
        build-essential \
        libcurl4-openssl-dev \
        libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src

# Resolve npm deps in a separate layer so source edits don't bust the cache.
COPY package.json package-lock.json ./
RUN npm ci --no-audit --no-fund

# Project source. .dockerignore keeps node_modules / .env / build artifacts out.
COPY . .

# Produce public/framework/lua-framework.js + public/lua/*.lua, then the C binary.
RUN npx webpack && make

# ---------- runtime stage ----------
# Minimal image: just the C binary, views, public assets, and the libcurl runtime.
FROM debian:bookworm-slim AS runtime

ENV DEBIAN_FRONTEND=noninteractive

# ca-certificates: needed for outbound TLS to api.resend.com
# libcurl4:        runtime for our libcurl Resend call (pulls libssl3 as a dep)
# curl:            for the container healthcheck probe
RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates \
        libcurl4 \
        curl \
    && rm -rf /var/lib/apt/lists/*

# Run as a non-root user. 8080 is unprivileged so this is fine.
RUN useradd --system --create-home --shell /usr/sbin/nologin app

WORKDIR /app

COPY --from=build --chown=app:app /src/server /app/server
COPY --from=build --chown=app:app /src/views  /app/views
COPY --from=build --chown=app:app /src/public /app/public

USER app

EXPOSE 8080

# .env is intentionally NOT baked in. Provide it at runtime via:
#   docker run -v "$(pwd)/.env:/app/.env:ro" ...
#   or via docker compose (see docker-compose.yml)
CMD ["./server", "8080"]
