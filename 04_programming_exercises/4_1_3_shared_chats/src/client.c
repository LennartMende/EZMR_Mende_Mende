#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include <cjson/cJSON.h>

#include "constants.h"
#include "random_request_generator.h"
#include "pdu_parsing.h"




static int init_client(uint16_t server_port) {

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
        perror("connect");
        close(client_fd);
        return -1;
    }

    printf("Connected to the server at %s:%u\n\n", SERVER_IP, server_port);

    return client_fd;
}



static int send_request(int client_fd, const char *request) {

    size_t request_length = strlen(request);
    ssize_t bytes_sent = send(client_fd, request, request_length, 0);

    if (bytes_sent < 0) {
        perror("send");
        return -1;
    }

    if ((size_t)bytes_sent < request_length) {
        fprintf(stderr, "The data was not sent in its entirety.\n\n");
        return -1;
    }

    printf("Sent request: %s\n\n", request);

    return 0;
}



static int receive_response(int client_fd, char *buffer, size_t buffer_size) {

    ssize_t bytes_received = recv(client_fd, buffer, buffer_size - 1, 0);

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





int main(void) {

    int client_fd = init_client(SERVER_PORT);
    if (client_fd < 0) {
        exit(EXIT_FAILURE);
    }
    
    char buffer[1024];

    uint32_t request_msg_id = 0;
    uint32_t response_json_msg_id = 0;

    while (1) {

        char *request;

        request_msg_id++;

        // generate random chat_id between 0 and MAX_CHAT_COUNT-1
        uint8_t request_chat_id = rand() % (MAX_CHAT_COUNT);

        // 1. get chat
        request = get_chat(request_msg_id, request_chat_id);

        if (request == NULL) {
            fprintf(stderr, "Failed to generate GET_CHAT request\n\n");
            return -1;
        }

        if (send_request(client_fd, request) != 0) {
            free(request);
            return -1;
        }

        free(request);

        if (receive_response(client_fd, buffer, sizeof(buffer)) != 0) {
            return -1;
        }


        // 2. parse response from server
        JsonPdu response_pdu;
        response_pdu.json_msg = cJSON_Parse(buffer);
        size_t content_length = 0;
        uint8_t chat_id = 0;
        
        if (test_json_msg(&response_pdu, &response_json_msg_id, client_fd) < 0) {
            continue;
        }
        if (test_header(&response_pdu, &response_json_msg_id, client_fd) < 0) {
            continue;
        }
        if (test_payload(&response_pdu, &response_json_msg_id, client_fd) < 0) {
            continue;
        }
        if (test_response_success(&response_pdu, &response_json_msg_id, client_fd) < 0) {
            continue;
        }
        if (test_json_msg_id(&response_pdu, &response_json_msg_id, client_fd) < 0) {
            continue;
        }
        if (test_time_stamp(&response_pdu, &response_json_msg_id, client_fd) < 0) {
            continue;
        }
        if (test_chat_id(&response_pdu, &response_json_msg_id, client_fd) < 0) {
            continue;
        }
        if (test_response_content(&response_pdu, &response_json_msg_id, client_fd, &content_length) < 0) {
            continue;
        }

        chat_id = cJSON_GetNumberValue(response_pdu.chat_id);


        sleep(1);


        request_msg_id++;

        // 3. add message
        request = add_message(request_msg_id, chat_id);

        if (request == NULL) {
            fprintf(stderr, "Failed to generate ADD_MSG request\n\n");
            return -1;
        }

        if (send_request(client_fd, request) != 0) {
            free(request);
            return -1;
        }

        free(request);

        if (receive_response(client_fd, buffer, sizeof(buffer)) != 0) {
            return -1;
        }


        sleep(1);


        request_msg_id++;


        // 4. get message for verification of add message
        request = get_chat(request_msg_id, chat_id);

        if (request == NULL) {
            fprintf(stderr, "Failed to generate GET_CHAT request\n\n");
            return -1;
        }

        if (send_request(client_fd, request) != 0) {
            free(request);
            return -1;
        }

        free(request);

        if (receive_response(client_fd, buffer, sizeof(buffer)) != 0) {
            return -1;
        }


        sleep(1);

        if (content_length > 0) {

            request_msg_id++;

            int msg_id = rand() % content_length;

            // 5. edit message
            request = edit_message(request_msg_id, chat_id, msg_id);

            if (request == NULL) {
                fprintf(stderr, "Failed to generate EDIT_MSG request\n\n");
                return -1;
            }

            if (send_request(client_fd, request) != 0) {
                free(request);
                return -1;
            }

            free(request);

            if (receive_response(client_fd, buffer, sizeof(buffer)) != 0) {
                return -1;
            }


            sleep(1);


            request_msg_id++;

            // 6. get message for verification of edit message
            request = get_message(request_msg_id, chat_id, msg_id);

            if (request == NULL) {
                fprintf(stderr, "Failed to generate GET_MSG request\n\n");
                return -1;
            }

            if (send_request(client_fd, request) != 0) {
                free(request);
                return -1;
            }

            free(request);

            if (receive_response(client_fd, buffer, sizeof(buffer)) != 0) {
                return -1;
            }
        }
        
        sleep(10);
    }

    close(client_fd);

    return 0;
}
