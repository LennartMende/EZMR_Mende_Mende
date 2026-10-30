#ifndef PEER_SERVER_UTILS_H
#define PEER_SERVER_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include "peer.h"


int utils_sleep_ms(int ms);

// creates a server and returns server_fd
// int init_peer_server(Peer *peer);

// waits for the first client to connect
// returns 0 if successful and -1 if an error occured
int wait_for_peer(Peer* peer, int print);

int connect_more_peers(Peer *peer, int print);


#endif