CC ?= gcc
CPPFLAGS ?= -Iinclude
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic -Wformat-truncation -g -O2
LDLIBS ?= -lcjson
SRC_DIR ?= src
BUILD_DIR ?= build


TARGETS := \
	$(BUILD_DIR)/peer

.PHONY: all clean

all: $(TARGETS)


$(BUILD_DIR):
	mkdir -p "$@"


$(BUILD_DIR)/peer: $(SRC_DIR)/peer.c $(SRC_DIR)/peer_utils.c $(SRC_DIR)/peer_synchronization.c $(SRC_DIR)/peer_server_utils.c $(SRC_DIR)/peer_client_utils.c $(SRC_DIR)/pdu_parsing.c $(SRC_DIR)/error_handling.c $(SRC_DIR)/random_request_generator.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ $(LDLIBS)

clean:
	rm -f $(TARGETS)
