CC       ?= cc
CFLAGS   ?= -std=c11 -Wall -Wextra -Wpedantic -O2 -g
AR       ?= ar
ARFLAGS  := rcs

CPPFLAGS := -Iinclude

BUILD := build
LIB   := $(BUILD)/libcompression_common.a
SRC   := $(wildcard src/*.c)
OBJ   := $(SRC:src/%.c=$(BUILD)/%.o)

.PHONY: all lib clean

all: lib

lib: $(LIB)

$(LIB): $(OBJ) | $(BUILD)
	$(AR) $(ARFLAGS) $@ $(OBJ)

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

$(BUILD):
	mkdir -p $(BUILD)

clean:
	rm -rf $(BUILD)
