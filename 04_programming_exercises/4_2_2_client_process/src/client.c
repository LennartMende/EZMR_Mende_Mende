#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>

#include <signal.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include "random_request_generator.h"

const int BUFFER_SIZE = 1024;
const int SENDING_RATE_IN_S = 3;

volatile sig_atomic_t interrupted = 0;

struct client_context {
    int request_fifo_fd;
    int response_fifo_fd;
    char fifo_path_s2c[256];
};

// signal handler to handle a keyboard interrupt
void handle_sigint(int signal) {
    (void) signal;
    interrupted = 1;
}


int main(void)
{
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

    char IPC_FILE_DIR[] = "../../ipc_files/";
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

    while (!interrupted) {

        if (request_msg_id == UINT32_MAX) {
            request_msg_id = 0;
        } else {
            request_msg_id++;
        }

        char *json_request = generate_random_request(request_msg_id++, pid);

        char request_with_delimiter[BUFFER_SIZE];
        int request_length = snprintf(
            request_with_delimiter,
            sizeof(request_with_delimiter),
            "%s\n",
            json_request
        );
        if (request_length < 0 || request_length >= (int) sizeof(request_with_delimiter)) {
            fprintf(stderr, "Request is too long\n");
            free(json_request);
            continue;
        }

        // send JSON request to the server
        if (json_request == NULL) {
            printf("Failed to create request string\n");
            continue;
        } else {
            if (write(request_fifo_fd, request_with_delimiter, (size_t)request_length) == -1) {
                if (errno == EPIPE) {
                    fprintf(stderr, "\nServer disconnected: no reader on request FIFO so writing failed\n");
                    printf("Closing and reopening the client's C2S FIFO\n");
                    close(request_fifo_fd);
                    printf("Waiting for a server to reconnect at the reading end of the C2S pipe.\n");
                    request_fifo_fd = open(fifo_path_c2s, O_WRONLY); // blocking until a server reconnects.
                    if (request_fifo_fd == -1) {
                        if (errno == EINTR && interrupted) {
                            break;
                        }

                        perror("Could not reopen request FIFO");
                        break;
                    }
                    while (write(request_fifo_fd, request_with_delimiter, (size_t)request_length) == -1) {
                        close(request_fifo_fd);
                        if (errno == EPIPE) {
                            request_fifo_fd = open(fifo_path_c2s, O_WRONLY); // blocking until a server reconnects.
                            }
                        }
                    printf("Server reconnected.\n\n");
                } else {
                    fprintf(stderr, "Error occured during writing request into request FIFO\n");
                }
            }
            printf("\nWriting request into request FIFO: %s\n", json_request);
        }

        // receive response from the server
        char response_str[BUFFER_SIZE];
        ssize_t bytes_read = read(response_fifo_fd, response_str, BUFFER_SIZE - 1);
        if (bytes_read == -1) {
            perror("Could not read the request from the FIFO");
            continue;
        } else if (bytes_read == 0) {
            printf("Server closed the FIFO. Waiting for a new server...\n"); // meaning the open request FIFO has to be reopened
            close(response_fifo_fd);
            response_fifo_fd = open(fifo_path_s2c, O_RDONLY);
            if (response_fifo_fd == -1) {
                fprintf(stderr, "Response FIFO could not be reopened\n");
                return 3;
            }
            printf("FIFO file for response reopened\n");

            continue;
        }
        response_str[bytes_read] = '\0';
        printf("Received response: %s\n\n", response_str);

        free(json_request);

        // wait for SENDING_RATE_IN_S seconds before sending the next request
        sleep(SENDING_RATE_IN_S);
    }

    printf("\nClosing client and delete resources...\n");

    close(request_fifo_fd);
    close(response_fifo_fd);

    unlink(fifo_path_s2c);

    printf("Client beendet.\n");

    return 0;
}
