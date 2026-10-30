#define _POSIX_C_SOURCE 199309L

#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>

#include <cjson/cJSON.h>

#include "pdu_parsing.h"
#include "error_handling.h"
#include "peer_utils.h"
#include "peer_server_utils.h"
#include "peer_client_utils.h"
#include "random_request_generator.h"
#include "peer_synchronization.h"


#define MS_PER_S 1000
#define NS_PER_MS 1000000

#define MSG_SENDING_RATE 3000
#define PEER_LISTS_PRINT_RATE 5000
#define AVAILABLE_PEER_COUNT_PRINT_RATE 1000
#define NO_DATA_AVAILABLE_PRINT_RATE 1000
#define NO_PEER_AVAILABLE_PRINT_RATE 1000
#define REARANGE_PRINT_RATE 5000



int sleep_ms(int ms) {
    struct timespec ts_req, ts_rem;
    ts_req.tv_sec = ms / MS_PER_S;
    ts_req.tv_nsec = (ms % MS_PER_S) * NS_PER_MS; // ms % 1000 -> remaining "real" ms -> * 1000000 for nano secs

    return (nanosleep(&ts_req, &ts_rem));
}

int main(int argc, char* argv[]) {
    // parse peer's server port if port is given as arg
    // defaults to 0 if no number or an invalid input is used
    int peer_offset = 0;
    if (argc > 1) {
        const char* arg1 = argv[1];
        char* endptr;
        peer_offset = (int) strtol(arg1, &endptr, 10); // __base=10 for decimal
    }
    
    uint16_t server_port = DEFAULT_PORT + peer_offset;
    //printf("Peer offset %d means peer's server port = %d\n", peer_offset, server_port);

    Peer peer = create_first_peer(server_port);

    unsigned int random_seed = 42U + (unsigned int)peer_offset;
    srand(random_seed);

    printf("Peer offset: %d, random seed: %u\n", peer_offset, random_seed);


    uint16_t outgoing_peer_fds_idx = 0;
    
    for (int possible_peer_idx = 0; possible_peer_idx < MAX_PEER_COUNT; possible_peer_idx++) {
        uint16_t tried_port = DEFAULT_PORT + possible_peer_idx;
        if (server_port != tried_port) {
            int fd = init_peer_client(DEFAULT_PORT + possible_peer_idx);
            if (fd >= 0) {
                peer.outgoing_peer_fds[outgoing_peer_fds_idx] = fd;
                peer.connected_peer_server_ports[outgoing_peer_fds_idx] = tried_port;
                outgoing_peer_fds_idx++;
            } else {
                peer.connected_peer_server_ports[outgoing_peer_fds_idx] = -1;
            }
        }
    }

    char buffer[1024]; // for the request content

    // JSON msg ids
    uint32_t response_json_msg_id = 0;
    uint32_t request_msg_id = 0;

    // time stamps for regularly timed prints (init to 0 so first time printed)
    int64_t last_message_sent_time = 0;
    int64_t last_peer_list = 0;
    int64_t last_no_data_time = 0;
    int64_t last_available_peer_count_time = 0;
    int64_t last_no_peer_time = 0;
    int64_t last_rearange_time = 0;

    // counters for requests
    uint64_t sent_request_counter = 0;
    uint64_t received_request_counter = 0;

    // workflow management
    WorkflowState workflow_state = WORKFLOW_GET_CHAT;

    uint8_t workflow_chat_id = 0;
    size_t workflow_content_length = 0;
    int workflow_msg_id = -1;

    int request_pending = 0;
    int pending_peer_index = -1;

    // now the creation of the peer's server is completed

    while (1)
    {
        //int64_t start_time = millis();

        if (peer.incoming_peer_fds[0] < 0) {
            int print = (millis() - last_no_peer_time >= NO_PEER_AVAILABLE_PRINT_RATE);
            if (print) {
                last_no_peer_time = millis();
            }
            wait_for_peer(&peer, print);
        }
        

        int print = (millis() - last_peer_list > PEER_LISTS_PRINT_RATE);
        connect_more_peers(&peer, print);
        if (print) {
            last_peer_list = millis();
        }
        
        // choose random peer to send a request to
        int available_peers[MAX_PEER_COUNT];
        int available_peer_count = 0;

        for (int i = 0; i < MAX_PEER_COUNT; i++) {
            if (peer.outgoing_peer_fds[i] >= 0) {
                available_peers[available_peer_count] = i;
                available_peer_count++;
            }
        }

        if (millis() - last_available_peer_count_time > AVAILABLE_PEER_COUNT_PRINT_RATE) {
            last_available_peer_count_time = millis();
            printf("Available outgoing peers: %d\n", available_peer_count);
        }

        if (available_peer_count > 0 && !request_pending && millis() - last_message_sent_time > MSG_SENDING_RATE) {

            // Select a random available peer and create a random request:
            Request req = create_random_request(peer, request_msg_id, 
                available_peers, available_peer_count);

            // Generate the request according to the current workflow state:
            WorkflowConfig wf_cfg = {workflow_state, workflow_chat_id, workflow_content_length, workflow_msg_id};
            generate_str_request(&req, wf_cfg);
            

            if (req.request_str == NULL) {
                fprintf(stderr, "Failed to generate request\n");
            } else {
                printf("Sending workflow state %d to peer on port %u\n",
                    workflow_state,
                    peer.connected_peer_server_ports[req.peer_index]);

                if (send_request(req.peer_fd, req.request_str) == 0) {
                    request_msg_id++;
                    sent_request_counter++;

                    request_pending = 1;
                    pending_peer_index = req.peer_index;
                    last_message_sent_time = millis();
                } else {
                    fprintf(stderr, "Failed to send request to peer on port %u\n",
                            peer.connected_peer_server_ports[req.peer_index]);
                }

                free(req.request_str);
            }
        }

        int sleep_success = sleep_ms(1);

        if (sleep_success != 0) {
            fprintf(stderr, "Sleep for 1 ms was interrupted before the passage of time!");
        }


        // receive requests
        int64_t recv_time;

        // json: {header: {json_msg_id: ..., time_stamp}, payload: {operands: ..., operation: ...}}
        for (int i = 0; i < MAX_PEER_COUNT; i++) {
            if (peer.incoming_peer_fds[i] > 0) { // iterate over all valid connections (valid, if client_fd > 0)
                ssize_t n = recv(peer.incoming_peer_fds[i], buffer, sizeof(buffer)-1, 0);

                if (n > 0)
                {
                    recv_time = millis();
                    buffer[n] = '\0';
                    printf("Data package NO. %ld received from peer %i at time %ld: %s \n\n", received_request_counter, i, millis(), buffer);
                    received_request_counter++;


                    // parse JSON
                    // json: buffer = "{"header": {"msg_id": ..., "time_stamp": ...}, "payload": {"operands": ..., "operation": ...}};
                    // create pdu and fill it step-by-step from buffer
                    JsonPdu pdu;
                    pdu.json_msg = cJSON_Parse(buffer); // parse JSON msg

                    
                    if (test_json_msg(&pdu, &response_json_msg_id, peer.incoming_peer_fds[i]) < 0) {
                        continue;
                    }
                    if (test_header(&pdu, &response_json_msg_id, peer.incoming_peer_fds[i]) < 0) {
                        continue;
                    }
                    if (test_json_msg_id(&pdu, &response_json_msg_id, peer.incoming_peer_fds[i]) < 0) {
                        continue;
                    }
                    if (test_time_stamp(&pdu, &response_json_msg_id, peer.incoming_peer_fds[i]) < 0) {
                        continue;
                    }
                    if (test_payload(&pdu, &response_json_msg_id, peer.incoming_peer_fds[i]) < 0) {
                        continue;
                    }
                    if (test_operation(&pdu, &response_json_msg_id, peer.incoming_peer_fds[i]) < 0) {
                        continue;
                    }
                    // save the operation:
                    const char *operation_value = cJSON_GetStringValue(pdu.operation);
                    if (test_chat_id(&pdu, &response_json_msg_id, peer.incoming_peer_fds[i]) < 0) {
                        continue;
                    }
                    uint8_t chat_id = cJSON_GetNumberValue(pdu.chat_id);
                    if (test_msg_id(&pdu, &response_json_msg_id, peer.incoming_peer_fds[i], operation_value) < 0) {
                        continue;
                    }
                    uint8_t msg_id = cJSON_GetNumberValue(pdu.msg_id);

                    uint8_t new_msg_id;
                    if(test_content(&pdu, &response_json_msg_id, peer.incoming_peer_fds[i], operation_value,
                                MAX_PEER_COUNT, peer.chats, chat_id, msg_id, &new_msg_id) < 0) {
                                    continue;
                                }
                    
                    // TODO!
                    WorkflowConfig wf_cfg = ;
                    if (is_sync(pdu) == 0) {
                        int next_peer_fd_idx = calculate_next_peer_fd();
                        WorkflowConfig wf_cfg = ...; // + Init!!
                        int create_and_send_sync_req_success =  create_and_send_sync_req(peer, next_peer_fd_idx, request_msg_id, wf_cfg,
                            &sync_pending, int* pending_sync_peer_index);
                        continue;
                        // here process the sync according to success, server_port and fd and send an ack
                    } else if (is_ack(pdu) == 0) {
                        WorkflowConfig wf_cfg = ;
                        create_and_send_ack_req(peer, peer_fd_idx, request_msg_id, wf_cfg,
                            &ack_pending, &pending_ack_peer_index, success, src_server_port);
                        continue;
                        // here process the sync according to success, server_port and fd and update the vars for the successfully connected peers
                    }
                    
                    
                    // now create the json response
                    cJSON* response = create_json_response(&response_json_msg_id, chat_id);
                    cJSON* response_payload = cJSON_GetObjectItem(response, "payload");
                    
                    if (strcmp(operation_value, "get_msg") == 0) {
                        int error_code = create_get_msg_response(response_payload, chat_id, msg_id, peer.chats);
                        if (error_code == -2) {
                            fprintf(stderr, "No valid chat id!\n\n");
                            continue;
                        } else if (error_code == -1) {
                            fprintf(stderr, "No valid msg id!\n\n");
                            continue;
                        }
                    } else if (strcmp(operation_value, "get_chat") == 0) {
                        int error_code = create_get_chat_response(response_payload, chat_id, peer.chats, MAX_PEER_COUNT);
                        if (error_code == -1) {
                            fprintf(stderr, "No valid chat id!\n\n");
                            continue;
                        }
                    } else if (strcmp(operation_value, "add_msg") == 0) {
                        int error_code = create_add_msg_response(response_payload, chat_id, new_msg_id);
                        if (error_code == -2) {
                            fprintf(stderr, "No valid chat id!\n\n");
                            continue;
                        } else if (error_code == -1) {
                            fprintf(stderr, "No valid msg id!\n\n");
                            continue;
                        }
                    } else if (strcmp(operation_value, "edit_msg") == 0) {
                        int error_code = create_edit_msg_response(response_payload, chat_id, msg_id);
                        if (error_code == -2) {
                            fprintf(stderr, "No valid chat id!\n\n");
                            continue;
                        } else if (error_code == -1) {
                            fprintf(stderr, "No valid msg id!\n\n");
                            continue;
                        }
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
                        send_json_response(response_string, peer.incoming_peer_fds[i]);
                    }

                    int64_t time_stamp = cJSON_GetNumberValue(pdu.time_stamp);
                    printf("time_stamp = %ld\n", (long) time_stamp);
                    printf("It took %ld ms from receiving to sending the latest data.\n\n", millis() - recv_time);


                    free(response_string);

                    cJSON_Delete(pdu.json_msg);   // just delete msg to delete the whole json tree
                    cJSON_Delete(response);
                } else if (n == 0)
                {
                    printf("\n\n!!!!!!Peer disconnected!!!!!!\n\n\n");

                    close(peer.incoming_peer_fds[i]);
                    peer.incoming_peer_fds[i] = -1;
                }
                else
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        if (millis() - last_no_data_time > NO_DATA_AVAILABLE_PRINT_RATE) {
                            printf("No data available for peer %i\n\n", i);
                            last_no_data_time = millis();
                        }
                    }
                    else
                    {
                        perror("recv");

                        close(peer.incoming_peer_fds[i]);
                        peer.incoming_peer_fds[i] = -1;
                    }
                }
            }
        }


        // Receive responses on outgoing peer connections
        for (int i = 0; i < MAX_PEER_COUNT; i++) {
            if (peer.outgoing_peer_fds[i] < 0) {
                continue;
            }

            ssize_t n = recv(
                peer.outgoing_peer_fds[i],
                buffer,
                sizeof(buffer) - 1,
                MSG_DONTWAIT
            );

            if (n > 0) {
                buffer[n] = '\0';

                printf("Response received from peer on port %u: %s\n\n",
                       peer.connected_peer_server_ports[i],
                       buffer);

                // Only process the response for the pending request.
                if (!request_pending || i != pending_peer_index) {
                    continue;
                }

                cJSON *response = cJSON_Parse(buffer);

                if (response == NULL) {
                    fprintf(stderr, "Invalid JSON response\n");
                    continue;
                }

                cJSON *header = cJSON_GetObjectItem(response, "header");
                cJSON *payload = cJSON_GetObjectItem(response, "payload");

                cJSON *success = cJSON_GetObjectItem(payload, "success");
                cJSON *chat_node = cJSON_GetObjectItem(payload, "chat_id");

                ParsedReceivedResponse pr_response = {response, header, payload, success, chat_node};// pr response == parsed received response

                if (!cJSON_IsObject(header) ||
                    !cJSON_IsObject(payload) ||
                    !cJSON_IsNumber(success) ||
                    !cJSON_IsNumber(chat_node)) {

                    fprintf(stderr, "Invalid response structure\n");
                    cJSON_Delete(response);
                    continue;
                }

                double chat_id_value = cJSON_GetNumberValue(pr_response.chat_node);
                if (chat_id_value > MAX_CHAT_COUNT || chat_id_value < 0) {
                    fprintf(stderr, "Invalid chat id of %d!", (int) chat_id_value);
                    cJSON_Delete(response);
                    continue;
                }

                // Update the workflow state according to the response.
                if (update_workflow_state(&pr_response, &workflow_state, &workflow_chat_id, &workflow_content_length) < 0) {
                    fprintf(stderr, "Invalid chat id");
                    cJSON_Delete(response);
                    continue;
                }
                
                // start next loop
                if (workflow_state == WORKFLOW_EDIT_MSG &&
                    workflow_content_length == 0) {
                    workflow_state = WORKFLOW_GET_CHAT;
                }

                printf("Workflow advanced to state %d\n", workflow_state);

                request_pending = 0;
                pending_peer_index = -1;

                cJSON_Delete(response);

            } else if (n == 0) {
                printf("Outgoing connection to port %u closed by peer\n",
                       peer.connected_peer_server_ports[i]);

                close(peer.outgoing_peer_fds[i]);
                peer.outgoing_peer_fds[i] = -1;
                peer.connected_peer_server_ports[i] = -1;

                if (request_pending && pending_peer_index == i) {
                    request_pending = 0;
                    pending_peer_index = -1;
                }

            } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("recv on outgoing peer");

                close(peer.outgoing_peer_fds[i]);
                peer.outgoing_peer_fds[i] = -1;
                peer.connected_peer_server_ports[i] = -1;

                if (request_pending && pending_peer_index == i) {
                    request_pending = 0;
                    pending_peer_index = -1;
                }
            }
        }

        
        // always rearange lists if required
        print = (millis() - last_rearange_time > REARANGE_PRINT_RATE);
        if (print) {
            reorder_peer_lists_and_print(&peer, &last_rearange_time);
        } else {
            reorder_peer_lists(&peer);
        }
        


        // discovery
        for (int possible_peer_idx = 0; possible_peer_idx < MAX_PEER_COUNT; possible_peer_idx++) {

            uint16_t tried_port = DEFAULT_PORT + possible_peer_idx;

            if (server_port == tried_port) {
                continue;
            }

            // check whether this peer is already connected
            int already_connected = 0;

            for (int i = 0; i < MAX_PEER_COUNT; i++) {
                if (peer.outgoing_peer_fds[i] >= 0 &&
                    peer.connected_peer_server_ports[i] == tried_port) {
                    already_connected = 1;
                    break;
                }
            }

            if (already_connected) {
                continue;
            }

            // find a free slot.
            int free_slot = -1;

            for (int i = 0; i < MAX_PEER_COUNT; i++) {
                if (peer.outgoing_peer_fds[i] < 0) {
                    free_slot = i;
                    break;
                }
            }

            if (free_slot < 0) {
                break;
            }

            int fd = init_peer_client(tried_port);

            if (fd >= 0) {
                peer.outgoing_peer_fds[free_slot] = fd;
                peer.connected_peer_server_ports[free_slot] = tried_port;

                printf("Connected to peer on port %u in slot %d\n",
                    tried_port, free_slot);
            }
        }
    }

    close(peer.server_fd);

    return 0;
}
