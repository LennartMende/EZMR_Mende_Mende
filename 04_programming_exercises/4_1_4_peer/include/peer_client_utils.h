#ifndef PEER_CLIENT_UTILS_H
#define PEER_CLIENT_UTILS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include <cjson/cJSON.h>

#include "constants.h"

#include "peer_utils.h"
#include "request.h"
#include "random_request_generator.h"





int init_peer_client(uint16_t server_port);

int send_request(int client_fd, const char *request);

int receive_response(int client_fd, char *buffer, size_t buffer_size);

// manipulates req so that the req string is written into req->request
//void create_string_request(Request* req);
Request create_random_request(Peer peer, uint64_t request_msg_id, 
        int available_peers[MAX_PEER_COUNT], int available_peer_count);

#endif
