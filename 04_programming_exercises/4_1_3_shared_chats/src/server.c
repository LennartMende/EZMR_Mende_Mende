#include <time.h>

#include <cjson/cJSON.h>

#include "pdu_parsing.h"
#include "error_handling.h"
#include "server_utils.h"



char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH]; //first dimension: which chat, second dimension: which message, third dimension: chars


int main(void)
{
    initialize_chats();

    char buffer[1024]; // for the request content
    uint32_t response_json_msg_id = 0;
    
    // client initialization with -1 for every unused i (at initialization no client_fd used)
    int client_fds[MAX_CLIENT_COUNT];
    for (int i = 0; i < MAX_CLIENT_COUNT; i++) {
        client_fds[i] = -1;
    }

    // creates a server and returns server_fd
    int server_fd = init_server();

    while (1)
    {
        if (client_fds[0] < 0) {
            if (wait_for_client(server_fd, client_fds) < 0) {
                continue;
            }
        } else {
            connect_more_clients(server_fd, client_fds);
        }

        reorder_list(client_fds, 0);

        // json: {header: {json_msg_id: ..., time_stamp}, payload: {operands: ..., operation: ...}}
        for (int i = 0; i < MAX_CLIENT_COUNT; i++) {
            if (client_fds[i] > 0) { // iterate over all valid connections (valid, if client_fd > 0)
                ssize_t n = recv(client_fds[i], buffer, sizeof(buffer)-1, 0);

                if (n > 0)
                {
                    buffer[n] = '\0';
                    printf("Data received on client %i: %s\n", i, buffer);


                    // Json parsen
                    // json: buffer = "{"header": {"msg_id": ..., "time_stamp": ...}, "payload": {"operands": ..., "operation": ...}};
                    // create pdu and fill it step-by-step from buffer
                    JsonPdu pdu;
                    pdu.json_msg = cJSON_Parse(buffer); // Json-Message parsen


                    
                    if (test_json_msg(&pdu, &response_json_msg_id, client_fds[i]) < 0) {
                        continue;
                    }
                    if (test_header(&pdu, &response_json_msg_id, client_fds[i]) < 0) {
                        continue;
                    }
                    if (test_json_msg_id(&pdu, &response_json_msg_id, client_fds[i]) < 0) {
                        continue;
                    }
                    if (test_time_stamp(&pdu, &response_json_msg_id, client_fds[i]) < 0) {
                        continue;
                    }
                    if (test_payload(&pdu, &response_json_msg_id, client_fds[i]) < 0) {
                        continue;
                    }
                    if (test_operation(&pdu, &response_json_msg_id, client_fds[i]) < 0) {
                        continue;
                    }
                    // save the operation:
                    const char *operation_value = cJSON_GetStringValue(pdu.operation);
                    if (test_chat_id(&pdu, &response_json_msg_id, client_fds[i]) < 0) {
                        continue;
                    }
                    uint8_t chat_id = cJSON_GetNumberValue(pdu.chat_id);

                    uint8_t msg_id = 0; // either the value won't matter or will be modified

                    if (strcmp(operation_value, "get_msg") == 0 || 
                        strcmp(operation_value, "edit_msg") == 0
                    ) {
                        if (test_msg_id(&pdu, &response_json_msg_id, client_fds[i], operation_value) < 0) {
                            continue;
                        }
                        msg_id = cJSON_GetNumberValue(pdu.msg_id);
                    }
                    
                    uint8_t new_msg_id;
                    if (strcmp(operation_value, "add_msg") == 0 || 
                        strcmp(operation_value, "edit_msg") == 0
                    ) {
                        if(test_content(&pdu, &response_json_msg_id, client_fds[i], operation_value,
                                chats, chat_id, msg_id, &new_msg_id) < 0) {
                            continue;
                        }
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

                    if (response_string == NULL) {
                        printf("Failed to create response string\n");
                    } else {
                        send_json_response(response_string, client_fds[i]);
                    }


                    free(response_string);

                    cJSON_Delete(pdu.json_msg);   // just delete msg to delete the whole json tree
                    cJSON_Delete(response);
                } else if (n == 0)
                {
                    printf("Client disconnected\n");

                    close(client_fds[i]);
                    client_fds[i] = -1;
                }
                else
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        printf("No data available for client %i\n", i);
                    }
                    else
                    {
                        perror("recv");

                        close(client_fds[i]);
                        client_fds[i] = -1;
                    }
                }
            }
        }
        reorder_list(client_fds, 0);
        sleep(1);
    }

    close(server_fd);

    return 0;
}
