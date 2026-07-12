CC       ?= cc
CFLAGS   ?= -std=c11 -Wall -Wextra -Wpedantic -O2 -g

CTESTPROBE_DIR := ../../ctestprobe
CTESTPROBE_LIB := $(CTESTPROBE_DIR)/build/libctestprobe.a

CPPFLAGS := -Iinclude -I$(CTESTPROBE_DIR)/include

BUILD    := build
SRC      := $(wildcard src/*.c)
OBJ      := $(SRC:src/%.c=$(BUILD)/%.o)
TEST_SRC := $(wildcard tests/*.c)
TEST_BIN := $(BUILD)/test_shannon_fano

.PHONY: all test clean

all: $(TEST_BIN)

$(TEST_BIN): $(OBJ) $(TEST_SRC) $(CTESTPROBE_LIB) | $(BUILD)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $(OBJ) $(TEST_SRC) $(CTESTPROBE_LIB)

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(BUILD):
	mkdir -p $(BUILD)

$(CTESTPROBE_LIB):
	$(MAKE) -C $(CTESTPROBE_DIR) lib

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -rf $(BUILD)
