CC ?= gcc
CPPFLAGS ?= -Iinclude
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic
LDLIBS ?= -lcjson
SRC_DIR ?= src
BUILD_DIR ?= build


TARGETS := \
	$(BUILD_DIR)/server

.PHONY: all clean

all: $(TARGETS)


$(BUILD_DIR):
	mkdir -p "$@"


$(BUILD_DIR)/server: $(SRC_DIR)/server.c $(SRC_DIR)/pdu_parsing.c $(SRC_DIR)/calculator.c $(SRC_DIR)/reorder_fd_list.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ $(LDLIBS)

clean:
	rm -f $(TARGETS)