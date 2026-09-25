CC = gcc
# Production flags: hardening on, no debug info. `make DEBUG=1` for a -g -O0 build.
CFLAGS = -Wall -Wextra -Wformat=2 -O2 -MMD -MP \
         -fstack-protector-strong -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2
ifdef DEBUG
    CFLAGS := $(filter-out -O2 -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2,$(CFLAGS)) -g -O0
endif

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
OBJ         := $(SRC:.c=.o) $(CONTROLLERS:.c=.o)
TARGET      := server$(EXE)

# Each tests/test_*.c is its own binary, linked against the framework (src/)
# but not main.c or the controllers.
TEST_SRC    := $(wildcard tests/test_*.c)
TEST_BIN    := $(TEST_SRC:.c=$(EXE))

DEPS        := $(OBJ:.o=.d) main.d $(TEST_SRC:.c=.d)

all: $(TARGET)

$(TARGET): main.o $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

tests/test_%$(EXE): tests/test_%.c $(SRC:.c=.o)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test: $(TEST_BIN)
	@for t in $(TEST_BIN); do echo "== $$t"; ./$$t || exit 1; done

-include $(DEPS)

clean:
ifeq ($(OS),Windows_NT)
	-$(RM) $(call FixPath,$(TARGET)) 2>nul
	-$(RM) $(call FixPath,main.o) $(call FixPath,main.d) 2>nul
	-$(RM) $(call FixPath,src/*.o) $(call FixPath,src/*.d) 2>nul
	-$(RM) $(call FixPath,controllers/*.o) $(call FixPath,controllers/*.d) 2>nul
	-$(RM) $(call FixPath,tests/*.d) $(call FixPath,tests/*.exe) 2>nul
else
	$(RM) $(TARGET) main.o main.d src/*.o src/*.d controllers/*.o controllers/*.d \
	      tests/*.d $(TEST_BIN)
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

.PHONY: all clean run test docker-build docker-push deploy rollback
