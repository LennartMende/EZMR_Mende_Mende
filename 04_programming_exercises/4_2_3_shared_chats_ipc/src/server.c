#include <time.h>

#include <cjson/cJSON.h>
#include <signal.h>

#include "pdu_parsing.h"
#include "error_handling.h"
#include "server_utils.h"

#include "constants.h"

char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH]; //first dimension: which chat, second dimension: which message, third dimension: chars

// create arr for fifo ids and paths with -1 in all entries
int response_fifo_fds[MAX_CLIENT_COUNT];
char response_fifo_file_paths[MAX_CLIENT_COUNT][FIFO_PATH_LENGTH];

int main(void)
{
    signal(SIGPIPE, SIG_IGN);

    printf("FIFO SERVER STARTED!\n");

    initialize_chats();

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

    int connected_client_count = 0;
    int discovery_success = discovery(&connected_client_count, response_fifo_fds, response_fifo_file_paths);
    if (discovery_success != 0) {
        return -1;
    }

    printf("\nresponse_fifo_fds = [");
    for (int i = 0; i < MAX_CLIENT_COUNT - 1; i++) {
        printf("%d, ", response_fifo_fds[i]);
    }
    printf("%d]\n\n", response_fifo_fds[MAX_CLIENT_COUNT - 1]);

    uint32_t response_json_msg_id = 0;

    printf("Server with FIFO started\n");

    while (1) {
        int discovery_code = runtime_discovery(&connected_client_count, response_fifo_fds, 
            response_fifo_file_paths);
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
           // parse json
            // json: single_req_str = "{"header": {"msg_id": ..., "time_stamp": ...}, "payload": {"operands": ..., "operation": ...}};
            // create pdu and fill it step-by-step from single_req_str
            printf("Processing now %s\n", single_req_str);
            JsonPdu pdu;
            pdu.json_msg = cJSON_Parse(single_req_str); // Json-Message parsen
            
            if (test_json_msg(&pdu, &response_json_msg_id, 255) < 0) {
                continue;
            }
            if (test_header(&pdu, &response_json_msg_id, 255) < 0) {
                continue;
            }


            // get the pid and derive the fd from the pid to use it for the response
            // header | client_pid
            pid_t client_pid = -1;
            if(exists_json_node(pdu.header, &pdu.client_pid, "client_pid") < 0) { // no time_stamp field existing
                sleep(10);
                continue;
            } else if (is_json_node_number(pdu.client_pid)) {
                continue;
            }
            double client_pid_value;
            if (is_json_node_uint(pdu.client_pid, &client_pid_value, 32)) {
                continue;
            }
            client_pid = (pid_t)client_pid_value;
            if (client_pid < 0) {
                printf("Invalid client fd!\n");
                continue;
            }

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
                        "No Response-FIFO found for client PID %d!\n",
                        (int)client_pid);
                continue;
            }



            if (test_json_msg_id(&pdu, &response_json_msg_id, response_fifo_fds[req_client_response_fifo_path_idx]) < 0) {
                continue;
            }
            if (test_time_stamp(&pdu, &response_json_msg_id, response_fifo_fds[req_client_response_fifo_path_idx]) < 0) {
                continue;
            }
            if (test_payload(&pdu, &response_json_msg_id, response_fifo_fds[req_client_response_fifo_path_idx]) < 0) {
                continue;
            }
            if (test_operation(&pdu, &response_json_msg_id, response_fifo_fds[req_client_response_fifo_path_idx]) < 0) {
                continue;
            }
            // save the operation:
            const char *operation_value = cJSON_GetStringValue(pdu.operation);
            if (test_chat_id(&pdu, &response_json_msg_id, response_fifo_fds[req_client_response_fifo_path_idx]) < 0) {
                continue;
            }
            uint8_t chat_id = cJSON_GetNumberValue(pdu.chat_id);
            if (test_msg_id(&pdu, &response_json_msg_id, response_fifo_fds[req_client_response_fifo_path_idx], operation_value) < 0) {
                continue;
            }
            uint8_t msg_id = cJSON_GetNumberValue(pdu.msg_id);

            

            uint8_t new_msg_id;
            if(test_content(&pdu, &response_json_msg_id, response_fifo_fds[req_client_response_fifo_path_idx], operation_value,
                        chats, chat_id, msg_id, &new_msg_id) < 0) {
                            continue;
                        }
            
            
            // now create the json response
            cJSON* response = create_json_response(&response_json_msg_id, chat_id);
            cJSON* response_payload = cJSON_GetObjectItem(response, "payload");
            
            if (strcmp(operation_value, "get_msg") == 0) {
                create_get_msg_response(response_payload, chat_id, msg_id, chats);
            } else if (strcmp(operation_value, "get_chat") == 0) {
                create_get_chat_response(response_payload, chat_id, chats);
            } else if (strcmp(operation_value, "add_msg") == 0) {
                create_add_msg_response(response_payload, chat_id, new_msg_id);
            } else if (strcmp(operation_value, "edit_msg") == 0) {
                create_edit_msg_response(response_payload, chat_id, msg_id);
            } else {
                printf("NO LEGAL OPERATION!\n");
                fprintf(stderr, "NO LEGAL OPERATION!\n");
                continue;
            }
            
            cJSON_AddNumberToObject(response_payload, "success", 0);

            // convert response json object to string
            char *response_string = cJSON_PrintUnformatted(response);

            printf("Sending request from PID %d -> FIFO %s -> FD %d\n",
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

            cJSON_Delete(pdu.json_msg);   // just delete msg to delete the whole json tree
            cJSON_Delete(response);



            single_req_str = strtok(NULL, "\n");
        }

        // runtime discovery to enable communication with new clients
        discovery_code = runtime_discovery(&connected_client_count, response_fifo_fds, 
            response_fifo_file_paths);
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
