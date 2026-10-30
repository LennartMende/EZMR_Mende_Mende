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


// #define MS_PER_S 1000
// #define NS_PER_MS 1000000

// #define MSG_SENDING_RATE 3000
// #define PEER_LISTS_PRINT_RATE 5000
// #define AVAILABLE_PEER_COUNT_PRINT_RATE 1000
// #define NO_DATA_AVAILABLE_PRINT_RATE 1000
// #define NO_PEER_AVAILABLE_PRINT_RATE 1000
// #define REARANGE_PRINT_RATE 5000



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

    Peer peer = create_first_peer(server_port);
    for (int i = 0; i < MAX_PEER_COUNT; i++) {
        peer.contacted_peer_fds[i] = -1;
    }

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

    if (peer.outgoing_peer_fds[0] != -1) {
        initialize_chats_for_new_peer(&peer);
    }

    char buffer[BUFFER_SIZE]; // for the request content

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
    uint64_t received_request_counter = 0;

    int sync_ack_pending = -1; // -1 means no sync ack started, 1 means sync_ack running, 0 means sync_ack_finished
    int pending_sync_ack_peer_indexes[MAX_PEER_COUNT];
    for (int i = 0; i < MAX_PEER_COUNT; i++) {
        pending_sync_ack_peer_indexes[i] = -1;
    }

    // workflow management
    WorkflowState workflow_state = WORKFLOW_GET_CHAT;

    uint8_t workflow_chat_id = 0;
    size_t workflow_content_length = 0;

    int request_pending = 0;

    // now the creation of the peer's server is completed

    WorkflowConfig wf_cfg;
    wf_cfg.wf_state = workflow_state;
    wf_cfg.wf_chat_id = workflow_chat_id;
    wf_cfg.wf_content_length = workflow_content_length;

    while (1)
    {
        sync_ack_pending = -1;
        for (int i = 0; i < MAX_PEER_COUNT; i++) {
            peer.contacted_peer_fds[i] = -1;
        }

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
            SyncAckRequest sync_req = create_random_sync_request(peer, request_msg_id, 
                available_peers, available_peer_count);

            // filling out the request to change the own chats variable and send the request to it's own
            OwnRequest own_request;
            own_request.chat_id = sync_req.chat_id; own_request.msg_id = sync_req.msg_id;
            own_request.operation = operation_from_workflow(wf_cfg);
            char *content;
            if (strcmp(own_request.operation, "add_msg") == 0 || strcmp(own_request.operation, "edit_msg") == 0) {
                content = generate_random_content();
            } else {
                content = "";
            }
            own_request.msg = content;

            // NOW: CALL GET METHODS IF OPERATION IS A GET OP
            // check first if get and then call if it is a get method
            // get: means print to console here
            if (strcmp(own_request.operation, "get_chats") == 0) {
                printf("======= GET CHATS =======\n");
                for (int chat_idx = 0; chat_idx < 10; chat_idx++) {
                    print_chat(peer.chats, chat_idx);
                }
                printf("=========================\n");
            } else if (strcmp(own_request.operation, "get_chat") == 0) {
                printf("======= GET CHAT =======\n");
                print_chat(peer.chats, own_request.chat_id);
                printf("========================\n");
            } else if (strcmp(own_request.operation, "get_msg") == 0) {
                printf("======= GET MSG =======\n");
                printf("%s\n", get_message_from_variable(peer.chats, own_request.chat_id, own_request.msg_id));
                printf("=======================\n");
            }
            // else it has to be an updating method:
            // manipulating the own chats var with my own_req and print the resulting chats
            else if (strcmp(own_request.operation, "add_msg") == 0 || strcmp(own_request.operation, "edit_msg") == 0) {
                if (update_own_chats(peer.chats, own_request) != 0) {
                    fprintf(stderr, "Cancelling sending the message because own update was unsuccessful...\n");
                    continue;
                }
                print_chat(peer.chats, own_request.chat_id);
                printf("\n\n===================\nChats after processing of the own request:\n");
                for (int chat_idx = 20; chat_idx < MAX_CHAT_COUNT; chat_idx++) {
                    print_chat(peer.chats, chat_idx);
                }
                printf("===================\n\n");
            }

            else  {
                printf("Invalid command for an update!\n");
                return 15;
            }

            // Generate the request according to the current workflow state:
            wf_cfg.wf_msg_id = own_request.msg_id;
            wf_cfg.wf_chat_id = own_request.chat_id;
            generate_str_sync_request(&sync_req, wf_cfg, own_request.msg);
            update_workflow(&wf_cfg);

            // send the message
            int non_neg_fd_idxs[MAX_PEER_COUNT];
            int non_neg_fd_count = detect_gaps_in_fds(peer.outgoing_peer_fds, non_neg_fd_idxs, 0);

            for (int j = 0; j < non_neg_fd_count; j++) {
                int peer_idx = non_neg_fd_idxs[j];

                sync_req.peer_index = peer_idx;
                sync_req.peer_fd = peer.outgoing_peer_fds[peer_idx];

                if (send_sync(peer, sync_req, &sync_ack_pending,
                            pending_sync_ack_peer_indexes) < 0) {
                    return -1;
                } else {
                    last_message_sent_time = millis();
                    peer.contacted_peer_fds[peer_idx] = sync_req.peer_fd;
                }
            }

            free(sync_req.request_str);
        }


        // receive requests
        int64_t recv_time;

        // json: {header: {json_msg_id: ..., time_stamp}, payload: {operands: ..., operation: ...}}
        for (int i = 0; i < MAX_PEER_COUNT; i++) {
            if (peer.incoming_peer_fds[i] > 0) { // iterate over all valid connections (valid, if client_fd > 0)
                
                ssize_t n = recv(peer.incoming_peer_fds[i], buffer, sizeof(buffer)-1, MSG_DONTWAIT);

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
                    
                    int init_sync_sending_server_port = is_init_sync(pdu);
                    if (init_sync_sending_server_port > 0) {
                        printf("Received init sync request: init_ack_sending_server_port = %d\n", init_sync_sending_server_port);
                        InitSyncAckRequest init_ack_resp = create_init_ack_resp(peer, init_sync_sending_server_port, peer.chats);
                        if (send_init_ack(peer, init_ack_resp) < 0) {
                            fprintf(stderr, "Failed to send init_ack!!!\n");
                        }
                        free(init_ack_resp.request_str);
                        cJSON_Delete(pdu.json_msg);
                        continue;
                    }
                    int ack_sending_server_port = is_ack(pdu);
                    printf("ack_sending_server_port = %d\n", ack_sending_server_port);
                    if (ack_sending_server_port > 0) {

                        sync_ack_pending = 0; // if not later in this part changed it means all peers are synchronized

                        // get the peer_fd_idx from the server port to set it as successfully synchronized
                        for (int peer_idx = 0; peer_idx < MAX_PEER_COUNT; peer_idx++) {
                            if (ack_sending_server_port == peer.connected_peer_server_ports[peer_idx]) {
                                peer.successfully_synchronized_peer_fds[peer_idx] = 1;
                                pending_sync_ack_peer_indexes[peer_idx] = 0;
                                break;
                            }
                        }

                        // now if the first non_neg_fd_count entries in peer.successfully_synchronized_peer_fds
                        // are 1 it means that all peers that were requested to sync updated successfully
                        int non_neg_fd_idxs[MAX_PEER_COUNT];
                        int non_neg_fd_count = detect_gaps_in_fds(peer.outgoing_peer_fds, non_neg_fd_idxs, 0);
                        for (int peer_idx = 0; peer_idx < non_neg_fd_count; peer_idx++) {
                            if (peer.successfully_synchronized_peer_fds[peer_idx] != 1) {
                                sync_ack_pending = 1;
                                break;
                            }
                        }

                        cJSON_Delete(pdu.json_msg);
                        continue;
                    }
                    printf("sync_ack_pending = %d", sync_ack_pending);
                    if (sync_ack_pending == 0) {
                        break;
                    }

                    if (test_operation(&pdu, &response_json_msg_id, peer.incoming_peer_fds[i]) < 0) {
                        continue;
                    }
                    // save the operation:
                    char* operation_value = cJSON_GetStringValue(pdu.operation);
                    if (test_chat_id(&pdu, &response_json_msg_id, peer.incoming_peer_fds[i]) < 0) {
                        continue;
                    }
                    uint8_t chat_id = cJSON_GetNumberValue(pdu.chat_id);

                    

                    if (test_msg_id(&pdu, &response_json_msg_id, peer.incoming_peer_fds[i], operation_value) < 0) {
                        continue;
                    }
                    uint8_t msg_id = cJSON_GetNumberValue(pdu.msg_id);

                    uint8_t new_msg_id;
                    

                    int success = test_content_and_process_sync(&pdu, &response_json_msg_id, peer.incoming_peer_fds[i], operation_value,
                                MAX_PEER_COUNT, peer.chats, chat_id, msg_id, &new_msg_id); // returns 0 in the success field of the 
                                                // ack msg if the testing and processing was successful


                    
                    int sync_sending_server_port = is_sync(pdu);
                    if (sync_sending_server_port > 0) {
                        // processing of the pdu:
                        printf("\n\n===================\nChats after pdu processing:\n");
                        for (int chat_idx = 20; chat_idx < MAX_CHAT_COUNT; chat_idx++) {
                            print_chat(peer.chats, chat_idx);
                        }
                        printf("===================\n\n");
                        sleep(1);
                        if (create_and_send_ack_req(peer, i, request_msg_id, success, peer.server_port) != 0) {
                            fprintf(stderr, "create_and_send_ack_req was not successful\n");
                        }
                        cJSON_Delete(pdu.json_msg);
                        continue;
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

                cJSON_Delete(response);

            } else if (n == 0) {
                printf("Outgoing connection to port %u closed by peer\n",
                       peer.connected_peer_server_ports[i]);

                close(peer.outgoing_peer_fds[i]);
                peer.outgoing_peer_fds[i] = -1;
                peer.connected_peer_server_ports[i] = -1;

                if (request_pending) {
                    request_pending = 0;
                }

            } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("recv on outgoing peer");

                close(peer.outgoing_peer_fds[i]);
                peer.outgoing_peer_fds[i] = -1;
                peer.connected_peer_server_ports[i] = -1;

                if (request_pending) {
                    request_pending = 0;
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

            // find a free slot
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
