# ArenaBinAllocator – Makefile
#
# Targets:
#   make           – build release binary (./allocator)
#   make debug     – build with AddressSanitizer + debug symbols
#   make run       – build release and execute
#   make clean     – remove build artefacts

CC      := gcc
TARGET  := allocator
SRCS    := allocator.c main.c
OBJS    := $(SRCS:.c=.o)

# Standard flags used by both builds
CFLAGS_COMMON := -std=c11 -Wall -Wextra -Wpedantic \
                 -Wshadow -Wformat=2 -Wcast-align \
                 -Wconversion -Wsign-conversion -Wnull-dereference

# Release build
CFLAGS  := $(CFLAGS_COMMON) -O2

# Debug build adds sanitisers and debug info
CFLAGS_DBG := $(CFLAGS_COMMON) -g3 -O0 \
              -fsanitize=address,undefined

# ── Default target ─────────────────────────────────────────────────────
.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c allocator.h
	$(CC) $(CFLAGS) -c -o $@ $<

# ── Debug target ───────────────────────────────────────────────────────
.PHONY: debug
debug: CFLAGS := $(CFLAGS_DBG)
debug: clean $(TARGET)

# ── Run target ─────────────────────────────────────────────────────────
.PHONY: run
run: all
	./$(TARGET)

# ── Clean target ───────────────────────────────────────────────────────
.PHONY: clean
clean:
	rm -f $(OBJS) $(TARGET)
