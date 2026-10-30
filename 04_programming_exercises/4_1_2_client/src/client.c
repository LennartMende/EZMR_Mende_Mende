#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include "random_request_generator.h"

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12345

int main(void)
{
    int client_fd;
    struct sockaddr_in server_addr;
    
    char buffer[1024];

    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(client_fd);
        return EXIT_FAILURE;
    }

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server at %s:%d\n", SERVER_IP, SERVER_PORT);

    uint32_t request_msg_id = 0;

    while (1) {

        if (request_msg_id == UINT32_MAX) {
            request_msg_id = 0;
        } else {
            request_msg_id++;
        }

        char *json_request = generate_random_request(request_msg_id);

        if (json_request == NULL) {
            fprintf(stderr, "Failed to generate JSON request\n");
            break;
        }

        // send JSON request to the server
        ssize_t bytes_sent = send(client_fd, json_request, strlen(json_request), 0);
        
        if (bytes_sent < 0) {
            perror("send");
            close(client_fd);
            return EXIT_FAILURE;
        }
        else if ((size_t)bytes_sent < strlen(json_request)) {
            printf("The data was not sent in its entirety.\n");
            close(client_fd);
            return EXIT_FAILURE;
        }
        else {
            printf("Sent request: %s\n", json_request);
        }

        // receive response from the server
        ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received < 0) {
            perror("recv");
            close(client_fd);
            exit(EXIT_FAILURE);
        }
        else if (bytes_received == 0) {
            printf("Server closed the connection\n");
            close(client_fd);
            exit(EXIT_FAILURE);
        }
        else {
            buffer[bytes_received] = '\0';
            printf("Received response: %s\n", buffer);
        }

        free(json_request);

        // wait for 10 seconds before sending the next request
        sleep(10);
    }

    close(client_fd);
    return 0;
}
