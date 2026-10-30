#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <time.h>

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include <cjson/cJSON.h>

#include "pdu_parsing.h"
#include "calculator.h"

//nc 127.0.0.1 12345

// fifo programming: https://www.youtube.com/watch?v=2hba3etpoJg

#define MAX_CLIENT_COUNT 100
#define FIFO_PATH_LENGTH 100
#define BUFFER_SIZE 1024

char IPC_FILE_DIR[] = "../../ipc_files/";

// create arr for fifo ids and paths with -1 in all entries
int response_fifo_fds[MAX_CLIENT_COUNT];
char response_fifo_file_paths[MAX_CLIENT_COUNT][FIFO_PATH_LENGTH];


// the following routine searches in the ipc_files dir for FIFO files 
// and stores them in response_fifo_file_paths
static int register_fifo_files() {
    DIR *dir = opendir(IPC_FILE_DIR);

    if (dir == NULL) {
        perror("Could not open directory");
        return -1;
    }

    struct dirent *entry; // directory entry as file
    struct stat file_info;

    int connected_clients = 0;
    while ((entry = readdir(dir)) != NULL) {
        // skip first entries
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0 ||
            strstr(entry->d_name, "c2s")) {
            continue;
        }

        // read file name and path into different variables
        char file_name[256]; 
        snprintf(file_name, sizeof(file_name), "%s", entry->d_name);

        char full_file_path[256+17]; // length = 256 + strlen(IPC_FILE_DIR)
        snprintf(full_file_path, sizeof(full_file_path), "%s%s", IPC_FILE_DIR, entry->d_name);

        if (lstat(full_file_path, &file_info) == -1) {
            perror("lstat");
            continue;
        }

        // now check if the file at full_file_path is a FIFO file, if so write the file path into the paths list
        if (S_ISFIFO(file_info.st_mode)) {
            strcpy(response_fifo_file_paths[connected_clients], IPC_FILE_DIR);
            strcat(response_fifo_file_paths[connected_clients], entry->d_name);
            printf("FIFO file found: %s\n", entry->d_name);
            connected_clients++;
        }
    }

    closedir(dir);

    return connected_clients;
}


// extend the existing fifo file list during runtime for new clients
static int extend_fifo_files(int connected_clients) { // connected_clients means the number of already connected clients
    DIR *dir = opendir(IPC_FILE_DIR);

    if (dir == NULL) {
        perror("Could not open directory");
        return -1;
    }

    struct dirent *entry; // directory entry as file
    struct stat file_info;

    int connected_clients_new = connected_clients;
    while ((entry = readdir(dir)) != NULL) {
        // skip first entries
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0 ||
            strstr(entry->d_name, "c2s")) {
                continue;
        }

        // read file name and path into different variables
        char file_name[256]; 
        snprintf(file_name, sizeof(file_name), "%s", entry->d_name);
        char full_file_path[256+17]; // length = 256 + strlen(IPC_FILE_DIR)
        snprintf(full_file_path, sizeof(full_file_path), "%s%s", IPC_FILE_DIR, entry->d_name);

        // now check if the file path is already in the connected_clients_list:
        int client_already_connected = 0;
        for (int i = 0; i < connected_clients_new; i++) {
            if (strcmp(full_file_path, response_fifo_file_paths[i]) == 0) {
                client_already_connected = 1;
                break;
            }
        }
        if (client_already_connected) {
            continue;
        }

        if (lstat(full_file_path, &file_info) == -1) {
            perror("lstat");
            continue;
        }

        // now check if the file at full_file_path is a FIFO file, if so write the file path into the paths list
        if (S_ISFIFO(file_info.st_mode)) {
            strcpy(response_fifo_file_paths[connected_clients_new], IPC_FILE_DIR);
            strcat(response_fifo_file_paths[connected_clients_new], entry->d_name);
            printf("FIFO file found: %s\n", entry->d_name);
            connected_clients_new++;
        }
    }

    closedir(dir);

    return connected_clients_new;
}


// the following function initiates a Writing end on the s2c FIFOs
// and stores their fds into response_fifo_fd
static int discovery(int* connected_client_count) {
    *connected_client_count = register_fifo_files();
    if (*connected_client_count == -1) {
        return 1;
    }

    for (int i = 0; i < *connected_client_count; i++) {
        printf("Trying to open reponse FIFO at file path %s\n", response_fifo_file_paths[i]);
        response_fifo_fds[i] = open(response_fifo_file_paths[i], O_WRONLY);
        if (response_fifo_fds[i] == -1) {
            fprintf(stderr, "Response FIFO could not be opened\n");
            return 2;
        }
        printf("FIFO file for response opened\n");
    }
    return 0;
}


// the following function initiates a Writing end on the s2c FIFOs
// and stores their fds into response_fifo_fd
static int runtime_discovery(int* connected_client_count) {
    int old_client_count = *connected_client_count;
    *connected_client_count = extend_fifo_files(*connected_client_count);
    if (*connected_client_count == -1) {
        return 7;
    }

    // DEBUG: show old and new client count: printf("old_client_count = %d, *connected_client_count = %d\n", old_client_count, *connected_client_count);

    for (int i = old_client_count; i < *connected_client_count; i++) {
        printf("Trying to open reponse FIFO at file path %s\n", response_fifo_file_paths[i]);
        response_fifo_fds[i] = open(response_fifo_file_paths[i], O_WRONLY);
        if (response_fifo_fds[i] == -1) {
            fprintf(stderr, "Response FIFO could not be opened\n");
            return 8;
        }
        printf("FIFO file for response opened\n");
    }
    return 0;
}



int main(void)
{
    signal(SIGPIPE, SIG_IGN);

    printf("FIFO SERVER STARTED!\n");

    char fifo_path_c2s[256];

    snprintf(fifo_path_c2s, sizeof(fifo_path_c2s), "%sc2s", IPC_FILE_DIR);

    // init the fds to all -1 (-1 means no client found for this index)
    for (int i = 0; i < MAX_CLIENT_COUNT; i++) {
        response_fifo_fds[i] = -1;
    }

    // create FIFO dir
    if (mkdir(IPC_FILE_DIR, 0777) == -1) {
        if (errno != EEXIST) { // error is other that that the FIFO file already exists for example if added manually (or remove line removed)
            fprintf(stderr, "FIFO directory could not be created!\n");
            return 4;
        }
    }
    
    // create FIFO for requests (OLD CODE, already done with the discovery function)
    if (mkfifo(fifo_path_c2s, 0777) == -1) {
        if (errno != EEXIST) { // error is other that that the FIFO file already exists
            fprintf(stderr, "FIFO file for requests could not be created!\n");
            return 2;
        }
    }

    int request_fifo_fd = open(fifo_path_c2s, O_RDONLY);
    if (request_fifo_fd == -1) {
        fprintf(stderr, "Request FIFO could not be opened\n");
        return 6;
    }
    printf("FIFO file for request opened\n");

    int connected_client_count;
    int discovery_success = discovery(&connected_client_count);
    if (discovery_success != 0) {
        return discovery_success;
    }

    printf("\nresponse_fifo_fds = [");
    for (int i = 0; i < MAX_CLIENT_COUNT - 1; i++) {
        printf("%d, ", response_fifo_fds[i]);
    }
    printf("%d]\n\n", response_fifo_fds[MAX_CLIENT_COUNT - 1]);

    uint32_t response_msg_id = 0;

    printf("Nonblocking server with FIFO started\n");

    while (1)
    {
        int discovery_code = runtime_discovery(&connected_client_count);
        if (discovery_code != 0) {
            return discovery_code;
        }
        // read the request and write it into req_str
        char req_str[BUFFER_SIZE];
        printf("\nWaiting for a request...\n\n");
        ssize_t bytes_read = read(request_fifo_fd, req_str, BUFFER_SIZE - 1);
        if (bytes_read == -1) {
            perror("Could not read the request from the FIFO");
            continue;
        } else if (bytes_read == 0) {
            printf("Server closed the response FIFO. Waiting for client...\n");
            close(request_fifo_fd);
            request_fifo_fd = open(fifo_path_c2s, O_RDONLY);
            if (request_fifo_fd == -1) {
                perror("Response FIFO could not be reopened");
                return 3;
            }
            printf("Response FIFO reopened\n");
            continue;
        }
        req_str[bytes_read] = '\0';
        printf("Received %s\n", req_str);

        // now divide the str at the \n position(s)
        char* single_req_str = strtok(req_str, "\n");


        while (single_req_str != NULL) {
            // // json: {header: {msg_id: ..., time_stamp}, payload: {operands: ..., operation: ...}}
            // for (int i = 0; i < MAX_CLIENT_COUNT; i++) {
            //     if (client_fds[i] > 0) { // iterate over all valid connections (valid, if client_fd > 0)
            //         ssize_t n = recv(client_fds[i], buffer, sizeof(buffer)-1, 0);

            // parse json msg
            // json: buffer = "{"header": {"msg_id": ..., "time_stamp": ...}, "payload": {"operands": ..., "operation": ...}};
            // create pdu and fill it step-by-step from buffer
            JsonPdu pdu;
            pdu.json_msg = cJSON_Parse(single_req_str); // parse json msg
            if(test_json_format(pdu.json_msg, "json_msg") < 0) { // json_msg is not json conform 
                continue;
            }

            // header
            if(exists_json_node(pdu.json_msg, &pdu.header, "header") < 0) { // json_node has no header field
                continue;
            }

            // header | msg_id
            double id;
            if(exists_json_node(pdu.header, &pdu.msg_id, "msg_id") < 0) { // no msg_id field existing
                continue;
            } else if (is_json_node_number(pdu.msg_id)) {
                continue;
            } else if (is_json_node_uint(pdu.msg_id, &id, 32)) {
                continue;
            }

            // header | time_stamp
            double time_stamp;
            if(exists_json_node(pdu.header, &pdu.time_stamp, "time_stamp") < 0) { // no time_stamp field existing
                continue;
            } else if (is_json_node_number(pdu.time_stamp)) {
                continue;
            } else if (is_json_node_uint(pdu.time_stamp, &time_stamp, 64)) {
                continue;
            }

            // header | client_pid
            pid_t client_pid = -1;
            if(exists_json_node(pdu.header, &pdu.client_pid, "client_pid") < 0) { // no time_stamp field existing
                continue;
            } else if (is_json_node_number(pdu.time_stamp)) {
                continue;
            } else if (is_json_node_uint(pdu.time_stamp, &time_stamp, 64)) {
                continue;
            }

            client_pid = cJSON_GetNumberValue(pdu.client_pid);
            if (client_pid < 0) {
                printf("nvalid client fd!\n");
                continue;
            }

            // PAYLOAD
            if(exists_json_node(pdu.json_msg, &pdu.payload, "payload") < 0) { // payload field missing
                continue;
            }

            // initialize operand_values array and operand_count for later use
            double *operand_values = NULL;
            int operand_count = 0;

            // OPERANDS
            if(exists_json_node(pdu.payload, &pdu.operands, "operands")) { // operands field missing
                continue;
            }

            // json payload for operands in format "operands": [1, 2, 3, 4, ...] -> floats zulässig?
            else if (is_json_node_array(pdu.operands)) {
                continue;
            } else {
                operand_count = cJSON_GetArraySize(pdu.operands);
                if (operand_count < 2) {
                    printf("operands array has less than 2 elements\n");
                    fprintf(stderr, "operands array has less than 2 elements");
                }
                else {
                    // allocate memory for operand_values array
                    operand_values = malloc(operand_count * sizeof(double));

                    if (operand_values == NULL) {
                        printf("Failed to allocate memory for operands\n");
                        fprintf(stderr, "Failed to allocate memory for operands\n");
                    }
                    else {
                        int break_loop = 0;
                        for (int i = 0; i < operand_count; i++) {
                            cJSON* operand = cJSON_GetArrayItem(pdu.operands, i);

                            if (is_json_node_number(operand) < 0) {
                                break_loop = 1;
                                break;
                            }
                            else {
                                operand_values[i] = cJSON_GetNumberValue(operand);
                            }
                        }
                        if (break_loop) {
                            continue;
                        }
                    }
                }
            }

            // initialize result variable for later use
            double result = 0.0;
            int successful_calculation = 0;

            if(exists_json_node(pdu.payload, &pdu.operation, "operation") < 0) {
                continue;
            } else if (is_json_node_string(pdu.operation) < 0) {
                continue;
            }
            else {
                const char *operation_value = cJSON_GetStringValue(pdu.operation);

                successful_calculation = calculate_request(
                    operand_values, operand_count, operation_value, &result
                );
            }


            if (response_msg_id == UINT32_MAX) {
                response_msg_id = 0;
            } else {
                response_msg_id++;
            }

            uint64_t response_timestamp = (uint64_t)time(NULL);

            // create response json object
            cJSON *response = cJSON_CreateObject();
            cJSON *response_header = cJSON_CreateObject();
            cJSON *response_payload = cJSON_CreateObject();

            cJSON_AddItemToObject(response, "header", response_header);
            cJSON_AddNumberToObject(response_header, "msg_id", response_msg_id);
            cJSON_AddNumberToObject(response_header, "time_stamp", response_timestamp);

            cJSON_AddItemToObject(response, "payload", response_payload);
            
            if (successful_calculation == 0) {
                cJSON_AddNumberToObject(response_payload, "result", result);
            } else if (successful_calculation < 0) {
                cJSON_AddStringToObject(response_payload, "result", "NULL");
            }
                        
            
            char *response_string = cJSON_PrintUnformatted(response);
            char req_client_response_fifo_path[256];
            snprintf(req_client_response_fifo_path,
                    sizeof(req_client_response_fifo_path),
                    "%ss2c%d", IPC_FILE_DIR, (int)client_pid);

            int req_client_response_fifo_path_idx = -1;

            for (int i = 0; i < connected_client_count; i++) {
                if (strcmp(req_client_response_fifo_path,
                        response_fifo_file_paths[i]) == 0) {
                    req_client_response_fifo_path_idx = i;
                    break;
                }
            }

            if (req_client_response_fifo_path_idx == -1) {
                fprintf(stderr,
                        "Kein Response-FIFO für Client-PID %d gefunden!\n",
                        (int)client_pid);
                continue;
            }

            printf("Request von PID %d -> FIFO %s -> FD %d\n",
                (int)client_pid,
                req_client_response_fifo_path,
                response_fifo_fds[req_client_response_fifo_path_idx]);


            if (response_string == NULL) {
                printf("Failed to create response string\n");
            } else {
                printf("Writing response into response FIFO: %s\n", response_string);

                if (write(response_fifo_fds[req_client_response_fifo_path_idx], response_string, strlen(response_string)) == -1) {
                    fprintf(stderr, "Error occured during writing response into response FIFO\n");
                    continue;;
                }
            }


            free(response_string);
            free(operand_values);

            cJSON_Delete(pdu.json_msg);   // delete the msg to delete the entire tree
            cJSON_Delete(response);

            single_req_str = strtok(NULL, "\n");
        }

        // runtime discovery to enable communication with new clients
        discovery_code = runtime_discovery(&connected_client_count);
        if (discovery_code != 0) {
            return discovery_code;
        }
    }

    close(request_fifo_fd);

    int number_of_clients;
    for (int i = 0; i < MAX_CLIENT_COUNT; i++) {
        if (response_fifo_fds[i] == -1) {
            number_of_clients = i + 1;
        }
    }
    for (int i = 0; i < number_of_clients; i++) {
        close(response_fifo_fds[i]);
        remove(response_fifo_file_paths[i]);
    }

    remove(fifo_path_c2s);

    return 0;
}
