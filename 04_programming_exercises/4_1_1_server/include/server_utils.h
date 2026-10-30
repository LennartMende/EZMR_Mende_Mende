#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>

#define MAX_CLIENT_COUNT 100

// rearange peer_fd lists
void reorder_outgoing_connections(
    int outgoing_peer_fds[MAX_CLIENT_COUNT],
    uint16_t connected_peer_server_ports[MAX_CLIENT_COUNT]);
void reorder_list(int peer_fds[MAX_CLIENT_COUNT]);
int detect_gaps_in_fds(int peer_fd_list[MAX_CLIENT_COUNT], int non_negative_fd_idxs[MAX_CLIENT_COUNT], int print);
int has_gaps(int peer_fd_list[MAX_CLIENT_COUNT], 
        int non_negative_fd_count, int gap_list_idxs[non_negative_fd_count], int print);
int rearange_peer_fd_list(int peer_fd_list[MAX_CLIENT_COUNT], int non_negative_fd_idxs[MAX_CLIENT_COUNT], 
        int non_negative_fd_count, int gap_list_idxs[non_negative_fd_count], int gap_count, int print);


#endif