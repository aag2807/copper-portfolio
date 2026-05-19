CC = gcc
CFLAGS = -Wall -Wextra -g -O2 -Iinclude

# Detect Windows (cmd.exe + mingw32-make sets OS=Windows_NT) vs POSIX.
ifeq ($(OS),Windows_NT)
    EXE     := .exe
    LDFLAGS := -lpthread -lws2_32
    RM      := del /Q /F
    FixPath  = $(subst /,\,$1)
else
    EXE     :=
    LDFLAGS := -lpthread
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

.PHONY: all clean run
