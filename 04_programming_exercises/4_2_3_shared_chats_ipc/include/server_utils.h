#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>


#include "constants.h"


int register_fifo_files(char response_fifo_file_paths[MAX_CLIENT_COUNT][FIFO_PATH_LENGTH]);
int extend_fifo_files(int connected_clients, char response_fifo_file_paths[MAX_CLIENT_COUNT][FIFO_PATH_LENGTH]);
int discovery(int* connected_client_count, int response_fifo_fds[MAX_CLIENT_COUNT], 
        char response_fifo_file_paths[MAX_CLIENT_COUNT][FIFO_PATH_LENGTH]);
int runtime_discovery(int* connected_client_count, int response_fifo_fds[MAX_CLIENT_COUNT], 
        char response_fifo_file_paths[MAX_CLIENT_COUNT][FIFO_PATH_LENGTH]);

int init_server();
// int wait_for_client(int server_fd, int* client_fds);
// void connect_more_clients(int server_fd, int* client_fds);
void send_json_response(char* response_string, int client_fd);
void initialize_chats(void);

#endif