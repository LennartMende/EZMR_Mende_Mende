#include "peer_client_utils.h"

#include <fcntl.h>
#include <unistd.h>


int init_peer_client(uint16_t server_port) {

    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(client_fd);
        return -1;
    }

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        close(client_fd);
        return -1;
    }

    int flags = fcntl(client_fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        close(client_fd);
        return -1;
    }

    if (fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL O_NONBLOCK");
        close(client_fd);
        return -1;
    }

    printf("Connected to the peer listening at %s:%u\n\n", SERVER_IP, server_port);

    return client_fd;
}

// sends the char* request (is in json format) on peer_fd
int send_request(int peer_fd, const char *request) {
    size_t request_length = strlen(request);
    ssize_t bytes_sent = send(peer_fd, request, request_length, MSG_NOSIGNAL);

    if (bytes_sent < 0) {
        perror("send");
        return -1;
    }

    if ((size_t)bytes_sent < request_length) {
        fprintf(stderr, "The data was not sent in its entirety.\n\n");
        return -1;
    }

    printf("Sent request to peer with fd %d: %s\n\n", peer_fd, request);
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



Request create_random_request(Peer peer, uint64_t request_msg_id, 
        int available_peers[MAX_PEER_COUNT], int available_peer_count) {
    
    Request req;

    req.request_msg_id = request_msg_id;
    req.peer_index = available_peers[rand() % available_peer_count];
    req.peer_fd = peer.outgoing_peer_fds[req.peer_index];
    req.request_str = NULL;

    return req;
}