CC ?= gcc
CPPFLAGS ?= -Iinclude
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic
LDLIBS ?= -lcjson
SRC_DIR ?= src
BUILD_DIR ?= build


TARGETS := \
	$(BUILD_DIR)/client

.PHONY: all clean

all: $(TARGETS)


$(BUILD_DIR):
	mkdir -p "$@"


$(BUILD_DIR)/client: $(SRC_DIR)/client.c $(SRC_DIR)/random_request_generator.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ $(LDLIBS)

clean:
	rm -f $(TARGETS)
