#ifndef PEER_H
#define PEER_H

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


// peer struct
typedef struct {
    struct sockaddr_in addr;

    int pid; // fd of this peer's server
    pid_t available_peer_pids[MAX_PEER_COUNT];
    pid_t connected_peer_pids[MAX_PEER_COUNT];
    int outgoing_fifo_fds[MAX_PEER_COUNT];
    char outgoing_fifo_file_paths[MAX_PEER_COUNT][FIFO_PATH_LENGTH];
    int incoming_fifo_fds[MAX_PEER_COUNT];
    char incoming_fifo_file_paths[MAX_PEER_COUNT][FIFO_PATH_LENGTH];

    char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH];


    int contacted_peer_pids[MAX_PEER_COUNT]; // peers that was the latest message sent to
    int successfully_synchronized_peer_pids[MAX_PEER_COUNT]; // peers that received the latest message and sent an acknowledge back
} Peer;

#endif