#include "client_utils.h"

#include "constants.h"

volatile sig_atomic_t interrupted = 0; // to handle keyboard interrupts and manage resources


// signal handler to handle a keyboard interrupt
static void handle_sigint(int signal) {
    (void) signal;
    interrupted = 1;
}



int main(void) {
    // keboard interrupt
    struct sigaction sa = {0};
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return EXIT_FAILURE;
    }

    // signal handling
    signal(SIGPIPE, SIG_IGN); // so that the program doesn't terminate if SIGPIPE occurs when trying to write onto an invalid pipe



    // pid is important for knowing on which pipe to write the request
    pid_t pid = getpid(); // use the PID to name the FIFO file that way
    printf("client pid: %d\n", pid);

    printf("FIFO CLIENT STARTED!\n");

    char fifo_path_c2s[256];
    char fifo_path_s2c[256];

    snprintf(fifo_path_c2s, sizeof(fifo_path_c2s), "%sc2s", IPC_FILE_DIR);
    snprintf(fifo_path_s2c, sizeof(fifo_path_s2c), "%ss2c%d", IPC_FILE_DIR, pid);

    
    // create FIFO dir
    if (mkdir(IPC_FILE_DIR, 0777) == -1) {
        if (errno != EEXIST) { // error is other that that the FIFO file already exists
            fprintf(stderr, "FIFO directory could not be created!\n");
            return 1;
        }
    }
    
    // create FIFO for requests
    if (mkfifo(fifo_path_c2s, 0777) == -1) {
        if (errno != EEXIST) { // error is other that that the FIFO file already exists
            fprintf(stderr, "FIFO file for requests could not be created!\n");
            return 2;
        }
    }

    // create FIFO for response
    if (mkfifo(fifo_path_s2c, 0777) == -1) {
        if (errno != EEXIST) { // error is other that that the FIFO file already exists
            fprintf(stderr, "FIFO file for responses could not be created!\n");
            return 3;
        }
    }

    printf("Created FIFO files.\n");

    int request_fifo_fd = open(fifo_path_c2s, O_WRONLY);
    if (request_fifo_fd == -1) {
        fprintf(stderr, "Request FIFO could not be opened");
        return 4;
    }
    printf("FIFO file for request opened\n");

    int response_fifo_fd = open(fifo_path_s2c, O_RDONLY);
    if (response_fifo_fd == -1) {
        fprintf(stderr, "Response FIFO could not be opened");
        return 5;
    }
    printf("FIFO file for response opened\n");

    uint32_t request_msg_id = 0;
    uint32_t response_json_msg_id = 0;

    while (!interrupted) {
        char *json_request;
        request_msg_id++;

        // generate random chat_id between 0 and 99
        uint8_t request_chat_id = rand() % MAX_CHAT_COUNT;

        // 1. get chat
        json_request = get_chat(request_msg_id, request_chat_id, pid);

        char request_with_delimiter[BUFFER_SIZE];
        int request_length = get_delimited_request(json_request, request_with_delimiter, BUFFER_SIZE);
        if (request_length == -1) {
            continue;
        }

        int write_and_free_json_req_error_code = 
            write_and_free_json_req(json_request, request_fifo_fd, fifo_path_c2s, request_with_delimiter, (size_t) request_length);
        if (write_and_free_json_req_error_code == -2) {
            break;
        }
        else if (write_and_free_json_req_error_code != 0) {
            continue;
        }


        // receive response from the server
        char response_str[BUFFER_SIZE];
        int receive_json_response_error_code = receive_json_response(response_str, &response_fifo_fd, fifo_path_s2c);
        if (receive_json_response_error_code == -4) {
            break;
        }
        else if (receive_json_response_error_code != 0) {
            continue;
        }
        // parse response from server
        JsonPdu response_pdu;
        response_pdu.json_msg = cJSON_Parse(response_str);
        size_t content_length = 0;
        uint8_t chat_id = 0;
        if (parse_json_response_base_fields(&response_pdu, response_str, &response_json_msg_id, request_fifo_fd) != 0) {
            continue;
        }
        if (test_response_content(&response_pdu, &response_json_msg_id, request_fifo_fd, &content_length) < 0) {
            continue;
        }

        chat_id = cJSON_GetNumberValue(response_pdu.chat_id);

        sleep(1);
        if (errno == EINTR) {
            break;
        }

        request_msg_id++;
        // add message
        json_request = add_message(request_msg_id, chat_id, pid);

        // char request_with_delimiter[BUFFER_SIZE];
        request_length = get_delimited_request(json_request, request_with_delimiter, BUFFER_SIZE);
        if (request_length == -1) {
            continue;
        }

        // write the req onto the FIFO
        if (write_and_free_json_req(json_request, request_fifo_fd, fifo_path_c2s, request_with_delimiter, (size_t) request_length) != 0) {
            continue;
        }

        // receive and process the response
        if (receive_json_response(response_str, &response_fifo_fd, fifo_path_s2c) != 0) {
            continue;
        }
        // parse response from server
        if (parse_json_response_base_fields(&response_pdu, response_str, &response_json_msg_id, request_fifo_fd) != 0) {
            continue;
        }
        
        chat_id = cJSON_GetNumberValue(response_pdu.chat_id);

        sleep(1);
        if (errno == EINTR) {
            break;
        }

        request_msg_id++;


        // 4. get message for verification of add message
        json_request = get_chat(request_msg_id, chat_id, pid);
        
        request_length = get_delimited_request(json_request, request_with_delimiter, BUFFER_SIZE);
        if (request_length == -1) {
            continue;
        }

        if (write_and_free_json_req(json_request, request_fifo_fd, fifo_path_c2s, request_with_delimiter, (size_t) request_length) != 0) {
            continue;
        }

        // receive the json msg
        if (receive_json_response(response_str, &response_fifo_fd, fifo_path_s2c) != 0) {
            continue;
        }


        // parse response from server
        // JsonPdu response_pdu;
        //response_pdu.json_msg = cJSON_Parse(response_str);
        if (parse_json_response_base_fields(&response_pdu, response_str, &response_json_msg_id, request_fifo_fd) != 0) {
            continue;
        }
        if (test_response_content(&response_pdu, &response_json_msg_id, request_fifo_fd, &content_length) < 0) {
            continue;
        }
        chat_id = cJSON_GetNumberValue(response_pdu.chat_id);
        
        
        sleep(1);
        if (errno == EINTR) {
            break;
        }

        if (content_length > 0) {

            request_msg_id++;

            int msg_id = rand() % content_length;

            // 5. edit message
            json_request = edit_message(request_msg_id, chat_id, msg_id, pid);
            request_length = get_delimited_request(json_request, request_with_delimiter, BUFFER_SIZE);
            if (request_length == -1) {
                continue;
            }

            if (write_and_free_json_req(json_request, request_fifo_fd, fifo_path_c2s, request_with_delimiter, (size_t) request_length) != 0) {
                continue;
            }

            // receive the json msg
            if (receive_json_response(response_str, &response_fifo_fd, fifo_path_s2c) != 0) {
                continue;
            }
            // parse response from server
            if (parse_json_response_base_fields(&response_pdu, response_str, &response_json_msg_id, request_fifo_fd) != 0) {
                continue;
            }
            
            
            // if (test_response_content(&response_pdu, &response_json_msg_id, request_fifo_fd, &content_length) < 0) {
            //     continue;
            // }
            chat_id = cJSON_GetNumberValue(response_pdu.chat_id);

            sleep(1);
            if (errno == EINTR) {
                break;
            }

            request_msg_id++;

            // 6. get message for verification of edit message
            json_request = get_message(request_msg_id, chat_id, msg_id, pid);
            request_length = get_delimited_request(json_request, request_with_delimiter, BUFFER_SIZE);
            if (request_length == -1) {
                continue;
            }
            if (write_and_free_json_req(json_request, request_fifo_fd, fifo_path_c2s, request_with_delimiter, (size_t) request_length) != 0) {
                continue;
            }

            // receive the json response
            if (receive_json_response(response_str, &response_fifo_fd, fifo_path_s2c) != 0) {
                continue;
            }

            // parse response from server
            if (parse_json_response_base_fields(&response_pdu, response_str, &response_json_msg_id, request_fifo_fd) != 0) {
                continue;
            }
            // test_response_content_get_msgt is different from test_response_content because a single chat is no array...
            if (test_response_content_get_msg(&response_pdu, &response_json_msg_id, request_fifo_fd) < 0) {
                continue;
            }
        }
        
        sleep(SENDING_RATE_IN_S);
        if (errno == EINTR) {
            break;
        }
    }

    printf("\nClosing client and delete resources...\n");

    close(request_fifo_fd);
    close(response_fifo_fd);

    unlink(fifo_path_s2c);

    printf("Client disconnected.\n");

    return 0;
}
