CC ?= gcc
CPPFLAGS ?= -Iinclude
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic -Wformat-truncation -O2
LDLIBS ?= -lcjson
SRC_DIR ?= src
BUILD_DIR ?= build


TARGETS := \
	$(BUILD_DIR)/server \
	$(BUILD_DIR)/client

.PHONY: all clean

all: $(TARGETS)


$(BUILD_DIR):
	mkdir -p "$@"


$(BUILD_DIR)/server: $(SRC_DIR)/server.c $(SRC_DIR)/pdu_parsing.c $(SRC_DIR)/error_handling.c $(SRC_DIR)/server_utils.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ $(LDLIBS)

$(BUILD_DIR)/client: $(SRC_DIR)/client.c $(SRC_DIR)/random_request_generator.c $(SRC_DIR)/pdu_parsing.c $(SRC_DIR)/error_handling.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ $(LDLIBS)

clean:
	rm -f $(TARGETS)
