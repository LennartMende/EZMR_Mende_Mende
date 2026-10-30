#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>

#include <cjson/cJSON.h>

#include "random_request_generator.h"
#include "pdu_parsing.h"


#define BUFFER_SIZE 1024
#define SENDING_RATE_IN_S 3

// volatile sig_atomic_t interrupted = 0; // to handle keyboard interrupts and manage resources

struct client_context {
    int request_fifo_fd;
    int response_fifo_fd;
    char fifo_path_s2c[256];
};

// signal handler to handle a keyboard interrupt
// void handle_sigint(int signal, int* interrupted);

// return -1 if failure (restart loop with continue) else request_length for success
int get_delimited_request(char* json_request, char request_with_delimiter[BUFFER_SIZE], size_t buffer_size);

// writes a json request on the fifo to the server
// returns 0 if successful, else -1 (if -1: restart the loop containing the function with continue)
int write_and_free_json_req(char* json_request, int request_fifo_fd, char* fifo_path_c2s, char* request_with_delimiter, size_t request_length);

// receives a json response on the fifo from the server
// returns 0 if successful, else -1 (request couldn't be read) or -2 (fifo closed by server) or -3 (file reopened)
// how to handle return values: if negative: restart the loop containing the function with continue
int receive_json_response(char* response_str, int* response_fifo_fd, char* fifo_path_s2c);

// receives a json response str on the fifo from the server and parses it into a JsonPdu
// returns 0 if successful, else negative numbers
// how to handle return values: if negative: restart the loop containing the function with continue
int parse_json_response_base_fields(JsonPdu* response_pdu, char* response_str, uint32_t *response_json_msg_id, int request_fifo_fd);

#endif