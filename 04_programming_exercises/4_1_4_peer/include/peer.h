#ifndef PEER_H
#define PEER_H

#include <arpa/inet.h>
#include <sys/socket.h>

#include "constants.h"


// peer struct
typedef struct {
    struct sockaddr_in addr;

    uint16_t server_port;
    char* server_ip;

    int server_fd; // fd of this peer's server
    int outgoing_peer_fds[MAX_PEER_COUNT];
    int incoming_peer_fds[MAX_PEER_COUNT]; // peer fds of all connected peers
    int connected_peer_server_ports[MAX_PEER_COUNT]; // ports of the connected peers' servers

    char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH];


    int contacted_peer_fds[MAX_PEER_COUNT]; // peers that was the latest message sent to
    int successfully_synchronized_peer_fds[MAX_PEER_COUNT]; // peers that received the latest message and sent an acknowledge back
} Peer;

#endif
