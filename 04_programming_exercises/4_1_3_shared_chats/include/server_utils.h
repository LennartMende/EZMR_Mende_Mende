#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include "constants.h"

int init_server();
int wait_for_client(int server_fd, int* client_fds);
void connect_more_clients(int server_fd, int* client_fds);
void send_json_response(char* response_string, int client_fd);
void initialize_chats(void);

// reordering of client_fd_list
void reorder_list(int client_fds[MAX_CLIENT_COUNT], int print);
int detect_gaps_in_fds(int client_fd_list[MAX_CLIENT_COUNT], int non_negative_fd_idxs[MAX_CLIENT_COUNT], int print);
int has_gaps(int client_fd_list[MAX_CLIENT_COUNT], 
        int non_negative_fd_count, int gap_list_idxs[non_negative_fd_count], int print);
int rearange_client_fd_list(int client_fd_list[MAX_CLIENT_COUNT], int non_negative_fd_idxs[MAX_CLIENT_COUNT], 
        int non_negative_fd_count, int gap_list_idxs[non_negative_fd_count], int gap_count, int print);

#endif