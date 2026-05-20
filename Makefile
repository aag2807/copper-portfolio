CC = gcc
CFLAGS = -Wall -Wextra -g -O2 -Iinclude

# Detect Windows (cmd.exe + mingw32-make sets OS=Windows_NT) vs POSIX.
ifeq ($(OS),Windows_NT)
    EXE     := .exe
    LDFLAGS := -lpthread -lws2_32 -lcurl -lcrypto
    RM      := del /Q /F
    FixPath  = $(subst /,\,$1)
else
    EXE     :=
    LDFLAGS := -lpthread -lcurl -lcrypto
    RM      := rm -f
    FixPath  = $1
endif

SRC         := $(wildcard src/*.c)
CONTROLLERS := $(wildcard controllers/*.c)
MODELS      := $(wildcard models/*.c)
OBJ         := $(SRC:.c=.o) $(CONTROLLERS:.c=.o) $(MODELS:.c=.o)
TARGET      := server$(EXE)

all: $(TARGET)

$(TARGET): main.c $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
ifeq ($(OS),Windows_NT)
	-$(RM) $(call FixPath,$(TARGET)) 2>nul
	-$(RM) $(call FixPath,src/*.o) 2>nul
	-$(RM) $(call FixPath,controllers/*.o) 2>nul
	-$(RM) $(call FixPath,models/*.o) 2>nul
else
	$(RM) $(TARGET) src/*.o controllers/*.o models/*.o
endif

run: $(TARGET)
ifeq ($(OS),Windows_NT)
	$(TARGET)
else
	./$(TARGET)
endif

# ----------------------------------------------------------------------------
# Cloud Run deploy targets
# ----------------------------------------------------------------------------
# Override on the command line:
#   make deploy TAG=v0.05
#   make deploy GCLOUD_PROJECT=other-project REGION=us-east1
# Use := + $(or ...) so the shell commands run ONCE at parse time and the same
# TAG is reused across docker-build, docker-push, and deploy. Command-line
# overrides (make TAG=v0.05 ...) still take precedence — make resolves those
# before any assignment in the file runs.
GCLOUD_PROJECT := $(or $(GCLOUD_PROJECT),$(shell gcloud config get-value project 2>/dev/null))
TAG            := $(or $(TAG),v$(shell date +%Y%m%d-%H%M%S))
REGION         ?= us-central1
SERVICE        ?= c-copper
AR_REPO        := $(REGION)-docker.pkg.dev/$(GCLOUD_PROJECT)/c-copper/server

docker-build:
	@test -n "$(GCLOUD_PROJECT)" || (echo "GCLOUD_PROJECT is empty. Run 'gcloud config set project <id>' or pass GCLOUD_PROJECT=<id>." && exit 1)
	docker build -t $(AR_REPO):$(TAG) -t $(AR_REPO):latest .

docker-push: docker-build
	docker push $(AR_REPO):$(TAG)
	docker push $(AR_REPO):latest

deploy: docker-push
	gcloud run deploy $(SERVICE) \
	  --image=$(AR_REPO):$(TAG) \
	  --region=$(REGION)

# Roll traffic back to the previous revision in one shot.
rollback:
	gcloud run services update-traffic $(SERVICE) --region=$(REGION) \
	  --to-revisions=$$(gcloud run revisions list --service=$(SERVICE) --region=$(REGION) \
	    --format='value(name)' --limit=2 | tail -n1)=100

.PHONY: all clean run docker-build docker-push deploy rollback
