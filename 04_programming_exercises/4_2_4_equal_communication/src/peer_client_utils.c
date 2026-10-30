#include "peer_client_utils.h"

#include <fcntl.h>
#include <unistd.h>


// sends the char* request (is in json format) on peer_fd
int send_request(int fifo_fd, const char *request) {
    size_t request_len = strlen(request);
    char *framed_request = malloc(request_len + 2);
    if (framed_request == NULL) {
        return 3;
    }

    snprintf(framed_request, request_len + 2, "%s\n", request);

    size_t framed_request_len = request_len + 1;

    size_t total_sent = 0;
    while (total_sent < framed_request_len) { // as long as the message isn't fully sent
        ssize_t sent = write(
            fifo_fd,
            framed_request + total_sent,
            framed_request_len - total_sent
        );
        if (sent < 0) {
            if (errno == EINTR) {
                continue; // interrupted
            }

            fprintf(stderr, "Error occurred during writing to FIFO");
            free(framed_request);
            return 1;
        }
        if (sent == 0) {
            fprintf(stderr, "No bytes written to FIFO!\n");
            free(framed_request);
            return 2;
        }
        total_sent += (size_t)sent;
    }

    printf("Sent request on FIFO with fd %d: %s\n\n", fifo_fd, request);
    free(framed_request);
    return 0;
}



int receive_response(int peer_fd, char *buffer, size_t buffer_size) {
    ssize_t bytes_received = recv(peer_fd, buffer, buffer_size - 1, 0);

    if (bytes_received < 0) {
        perror("recv");
        return -1;
    }

    if (bytes_received == 0) {
        printf("Server closed the connection\n\n");
        return -1;
    }

    buffer[bytes_received] = '\0';
    printf("Received response: %s\n\n", buffer);
    
    return 0;
}


// void create_string_request(Request* req) {
//     printf("Selected peer slot: %d\n", req->peer_index);
//     printf("Selected socket fd: %d\n", req->peer_fd);

//     req->request_str = add_message(req->request_msg_id, req->chat_id);
// }

// Request create_random_request(Peer peer, uint64_t request_msg_id, 
//         int available_peers[MAX_PEER_COUNT], int available_peer_count) {
    
//     Request req;

//     req.request_msg_id = request_msg_id;

//     req.peer_index = available_peers[rand() % available_peer_count];
//     req.peer_fd = peer.outgoing_peer_fds[req.peer_index];

//     //req.chat_id = rand() % MAX_CHAT_COUNT;

//     req.request_str = NULL;

//     return req;
// }
