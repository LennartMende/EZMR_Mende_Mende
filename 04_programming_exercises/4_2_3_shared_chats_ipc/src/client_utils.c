#include "client_utils.h"




// return -1 if failure (restart loop with continue) else request_length for success
int get_delimited_request(char* json_request, char request_with_delimiter[BUFFER_SIZE], size_t buffer_size) {
    int request_length = snprintf(
        request_with_delimiter,
        buffer_size,
        "%s\n",
        json_request
    );
    if (request_length < 0 || request_length >= (int) buffer_size) {
        fprintf(stderr, "Request is too long\n");
        free(json_request);
        return -1;
    }
    return request_length;
}



// writes a json request on the fifo to the server
// returns 0 if successful, else -1 (if -1: restart the loop containing the function with continue)
int write_and_free_json_req(char* json_request, int request_fifo_fd, char* fifo_path_c2s, char* request_with_delimiter, size_t request_length) {
    // send JSON request to the server
    if (json_request == NULL) {
        printf("Failed to create request string\n");
        return -1;
    } else {
        if (write(request_fifo_fd, request_with_delimiter, request_length) == -1) {
            if (errno == EPIPE) {
                fprintf(stderr, "\nServer disconnected: no reader on request FIFO so writing failed\n");
                printf("Closing and reopening the client's C2S FIFO\n");
                close(request_fifo_fd);
                printf("Waiting for a server to reconnect at the reading end of the C2S pipe.\n");
                request_fifo_fd = open(fifo_path_c2s, O_WRONLY); // blocking until a server reconnects.
                while (write(request_fifo_fd, request_with_delimiter, (size_t)request_length) == -1) {
                    close(request_fifo_fd);
                    if (errno == EPIPE) {
                        request_fifo_fd = open(fifo_path_c2s, O_WRONLY); // blocking until a server reconnects.
                        }
                        if (request_fifo_fd == -1) {
                            if (errno == EINTR) {
                                return -2;
                            }
                            perror("Could not reopen request FIFO");
                            return -1;
                        }
                    }
                printf("Server reconnected.\n\n");
            } else {
                fprintf(stderr, "Error occured during writing request into request FIFO\n");
            }
        }
        printf("\nWriting request into request FIFO: %s\n", json_request);
    }

    // free the memory at json_request
    free(json_request);

    return 0;
}


// receives a json response on the fifo from the server
// returns 0 if successful, else -1 (request couldn't be read) or -2 (fifo closed by server) or -3 (file reopened)
// how to handle return values: if negative: restart the loop containing the function with continue
int receive_json_response(char* response_str, int* response_fifo_fd, char* fifo_path_s2c) {
    ssize_t bytes_read = read(*response_fifo_fd, response_str, BUFFER_SIZE - 1);
    if (bytes_read == -1) {
        if (errno == EINTR){
            return -4;
        }
        fprintf(stderr, "Could not read the request from the FIFO");
        return -1;
    } else if (bytes_read == 0) {
        printf("Server closed the FIFO. Waiting for a new server...\n"); // meaning the open request FIFO has to be reopened
        close(*response_fifo_fd);
        *response_fifo_fd = open(fifo_path_s2c, O_RDONLY);
        if (*response_fifo_fd == -1) {
            fprintf(stderr, "Response FIFO could not be reopened\n");
            return -2;
        }
        printf("FIFO file for response reopened\n");
        return -3;
    }
    response_str[bytes_read] = '\0';
    printf("Received response: %s\n\n", response_str);

    return 0;
}


// receives a json response str on the fifo from the server and parses it into a JsonPdu
// returns 0 if successful, else negative numbers
// how to handle return values: if negative: restart the loop containing the function with continue
int parse_json_response_base_fields(JsonPdu* response_pdu, char* response_str, uint32_t *response_json_msg_id, int request_fifo_fd) {
    response_pdu->json_msg = cJSON_Parse(response_str);
    
    if (test_json_msg(response_pdu, response_json_msg_id, request_fifo_fd) < 0) {
        return -1;
    }
    if (test_header(response_pdu, response_json_msg_id, request_fifo_fd) < 0) {
        return -2;
    }
    if (test_payload(response_pdu, response_json_msg_id, request_fifo_fd) < 0) {
        return -3;
    }
    if (test_response_success(response_pdu, response_json_msg_id, request_fifo_fd) < 0) {
        return -4;
    }
    if (test_json_msg_id(response_pdu, response_json_msg_id, request_fifo_fd) < 0) {
        return -5;
    }
    if (test_time_stamp(response_pdu, response_json_msg_id, request_fifo_fd) < 0) {
        return -6;
    }
    if (test_chat_id(response_pdu, response_json_msg_id, request_fifo_fd) < 0) {
        return -7;
    }
    // if (test_response_content(&response_pdu, response_json_msg_id, request_fifo_fd, content_length) < 0) {
    //     return -8;
    // }
    return 0;
}