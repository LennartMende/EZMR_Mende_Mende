#include "peer_synchronization.h"

/*
sync:
peer1 ---- sync_msg ----> peer2
    -> peer2 checks if it is a sync message, if it is then send a sync_ack with  success field back:
            peer2 ---- sync_ack ----> peer1
                -> peer1 checks if it is a sync ack, if it is peer2 updates it's synchronized peer list
*/


////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////// ROUTINES FOR INITIALIZATION //////////////////////////////////////////



// function for initializing chats so they are synchronous (hold the same chats var)


int initialize_chats_for_new_peer(Peer *peer) {
    // initalize and send the init sync request
    InitSyncAckRequest init_sync_req = create_init_sync_req(*peer);
    if (send_init_sync(*peer, init_sync_req) != 0) {
        return -1;
    }

    // receive the response (acknowledge), the responding *peer
    // has the same *peer_index as the one udes for sending the sync
    int64_t ack_resp_buffer_length = 
        MAX_CHAT_COUNT*MAX_MSG_COUNT*MAX_MSG_LENGTH + (MAX_CHAT_COUNT-1)*(MAX_MSG_COUNT-1) + 3*MAX_CHAT_COUNT + BUFFER_SIZE + SAFETY_BUFFER;
    char ack_resp_buffer[ack_resp_buffer_length];
    /*
    buffer size estimation:
    up to MAX_CHAT_COUNT*MAX_MSG_COUNT*MAX_MSG_LENGTH chars
    + delimiting symbols (up to MAX_MSG_COUNT-1 ',' per chat (up to
        (MAX_CHAT_COUNT-1)*(MAX_MSG_COUNT-1) ','), MAX_CHAT_COUNT '[' and ']' and ',' to
        delimit the chats)
    + additional msg head, up to BUFFER_SIZE chars 
    + SAFETY_BUFFER as safety buffer for the estimation
    -> resulting in MAX_CHAT_COUNT*MAX_MSG_COUNT*MAX_MSG_LENGTH + (MAX_CHAT_COUNT-1)*(MAX_MSG_COUNT-1) + 
        3*MAX_CHAT_COUNT + BUFFER_SIZE SAFETY_BUFFER
    */
   
    int init_ack_received = 0;
    int64_t last_no_data_time = millis();

    // receiving the ack
    while (!init_ack_received) {
        if (peer->incoming_peer_fds[0] < 0) {
            wait_for_peer(peer, 0);
        }
        ssize_t n = recv(
            peer->outgoing_peer_fds[init_sync_req.peer_index], 
            ack_resp_buffer, 
            sizeof(ack_resp_buffer)-1, 
            MSG_DONTWAIT
        );
        char* single_req_str = strtok(ack_resp_buffer, "\n");
        while (single_req_str != NULL) {
            if (n > 0) {
            // pdu parsing
            single_req_str[n] = '\0';
            printf("Received ack data package for chats initialization: %s\n\n", single_req_str);

            // parse JSON
            // json: buffer = "{"header": {"msg_id": ..., "time_stamp": ...}, "payload": {"operands": ..., "operation": ...}};
            // create pdu and fill it step-by-step from buffer
            JsonPdu pdu;
            pdu.json_msg = cJSON_Parse(single_req_str); // parse JSON msg

            if (test_json_msg(&pdu, NULL, peer->incoming_peer_fds[0]) < 0) {
                continue;
            }
            if (test_header(&pdu, NULL, peer->incoming_peer_fds[0]) < 0) {
                continue;
            }
            if (test_json_msg_id(&pdu, NULL, peer->incoming_peer_fds[0]) < 0) {
                continue;
            }
            if (test_time_stamp(&pdu, NULL, peer->incoming_peer_fds[0]) < 0) {
                continue;
            }
            if (test_payload(&pdu, NULL, peer->incoming_peer_fds[0]) < 0) {
                continue;
            }
            // printf("Before ack recognition and handling\n");
            int init_ack_sending_server_port = is_init_ack(pdu);
            printf("ack_sending_server_port = %d\n", init_ack_sending_server_port);
            if (init_ack_sending_server_port > 0) {
                // chats initialization
                if (test_and_retrieve_content(pdu, peer->chats) < 0) {
                    fprintf(stderr, "Ack doesn't have a content field in the reight format!\n");
                }
                init_ack_received = 1;
                cJSON_Delete(pdu.json_msg);
            } else {
                fprintf(stderr, "WARNING: Received a non init package before initializing the chats. This request will not be processed!");
            }
        } else if (n == 0) {
            printf("\n\n!!!!!!Peer disconnected!!!!!!\n\n\n");

            close((peer->incoming_peer_fds)[0]);
            (peer->incoming_peer_fds)[0] = -1;
        }
        else {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                if (millis() - last_no_data_time > 1000) {
                    printf("No data available for chat synchronization\n\n");
                    last_no_data_time = millis();
                }
            }
            else
            {
                perror("recv");

                close((peer->incoming_peer_fds)[0]);
                (peer->incoming_peer_fds)[0] = -1;
            }
        }

            single_req_str = strtok(NULL, "\n");
        }
        
        sleep(1);
    }

    return 0;
}


//////////////// INIT SYNC ///////////////////////
//////////////////////////////////////////////////


// send an init sync request with the json msg
int send_init_sync(Peer peer, InitSyncAckRequest init_sync_req) {
    if (send_request(init_sync_req.peer_fd, init_sync_req.request_str) == 0) {
        return 0;
    } else {
        fprintf(stderr, "Failed to send init sync to peer on port %u\n",
                peer.connected_peer_server_ports[init_sync_req.peer_index]);
        return -1;
    }
}

// create an init sync request with the json msg
InitSyncAckRequest create_init_sync_req(Peer peer) {
    InitSyncAckRequest init_sync_req;
    init_sync_req.peer_index = 0;
    init_sync_req.peer_fd = peer.outgoing_peer_fds[init_sync_req.peer_index];
    init_sync_req.src_peer_server_port = peer.server_port;
    cJSON* json_req = fill_init_sync_req(&init_sync_req);
    init_sync_req.request_str = cJSON_PrintUnformatted(json_req);
    cJSON_Delete(json_req);
    return init_sync_req;
}

cJSON* fill_init_sync_req(InitSyncAckRequest* init_sync_req) {
    uint64_t request_json_msg_id = 424242;
    uint64_t request_timestamp = (uint64_t)millis();
    
    cJSON *request = cJSON_CreateObject();
    cJSON *request_header = cJSON_CreateObject();
    cJSON *request_payload = cJSON_CreateObject();

    cJSON_AddItemToObject(request, "header", request_header);
    cJSON_AddNumberToObject(request_header, "json_msg_id", request_json_msg_id);
    cJSON_AddNumberToObject(request_header, "time_stamp", request_timestamp);
    cJSON_AddNumberToObject(request_header, "init_sync", init_sync_req->src_peer_server_port);

    cJSON_AddItemToObject(request, "payload", request_payload);

    return request;
}


// test if the mesage is a sync message for the initialization
int is_init_sync(JsonPdu pdu) {
    cJSON* node = cJSON_GetObjectItem(pdu.header, "init_sync");
    if (node == NULL) {
        return -1;
    }
    return cJSON_GetNumberValue(node);
}


//////////////// INIT ACK ////////////////////////
//////////////////////////////////////////////////

// create init ack resp
InitSyncAckRequest create_init_ack_resp(Peer peer, int dest_server_port, char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH]) {
    InitSyncAckRequest init_ack_resp;

    // determine the peer_idx and peer_fd to send the ack resp to
    for (int i = 0; i < MAX_PEER_COUNT; i++) {
        if (peer.connected_peer_server_ports[i] == dest_server_port) {
            init_ack_resp.peer_index = i;
            init_ack_resp.peer_fd = peer.incoming_peer_fds[init_ack_resp.peer_index];
        }
    }
    init_ack_resp.src_peer_server_port = peer.server_port;
    cJSON* json_resp = fill_init_ack_resp(&init_ack_resp, chats);
    init_ack_resp.request_str = cJSON_PrintUnformatted(json_resp);
    cJSON_Delete(json_resp);
    return init_ack_resp;
}

cJSON* fill_init_ack_resp(
    InitSyncAckRequest* init_ack_resp,
    char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH]) {
    uint64_t request_json_msg_id = 424243;
    uint64_t request_timestamp = (uint64_t)millis();

    cJSON *response = cJSON_CreateObject();
    cJSON *response_header = cJSON_CreateObject();
    cJSON *response_payload = cJSON_CreateObject();
    cJSON *response_content = cJSON_CreateArray();  // Änderung!

    cJSON_AddItemToObject(response, "header", response_header);
    cJSON_AddNumberToObject(response_header, "json_msg_id", request_json_msg_id);
    cJSON_AddNumberToObject(response_header, "time_stamp", request_timestamp);
    cJSON_AddNumberToObject(response_header, "init_ack", init_ack_resp->src_peer_server_port
    );
    cJSON_AddItemToObject(response, "payload", response_payload);
    cJSON_AddItemToObject(response_payload, "content", response_content);

    for (int chat_idx = 0; chat_idx < MAX_CHAT_COUNT; chat_idx++) {
        cJSON *chat = cJSON_CreateArray();
        for (int msg_idx = 0; msg_idx < MAX_MSG_COUNT; msg_idx++) {
            cJSON_AddItemToArray(
                chat,
                cJSON_CreateString(chats[chat_idx][msg_idx])
            );
        }
        cJSON_AddItemToArray(response_content, chat);
    }
    return response;
}


// send the ack from the old peer to the new one
int send_init_ack(Peer peer, InitSyncAckRequest init_ack_resp) {
    if (send_request(init_ack_resp.peer_fd, init_ack_resp.request_str) == 0) {
        if (send_request(init_ack_resp.peer_fd, "\n") != 0) {
            fprintf(stderr, "Failed to send init sync to peer on port %u\n",
                peer.connected_peer_server_ports[init_ack_resp.peer_index]);
        }
    } else {
        fprintf(stderr, "Failed to send init sync to peer on port %u\n",
                peer.connected_peer_server_ports[init_ack_resp.peer_index]);
        return -1;
    }
    return 0;
}

// test if json pdu is an init_ack
int is_init_ack(JsonPdu pdu) {
    cJSON* node = cJSON_GetObjectItem(pdu.header, "init_ack");
    if (node == NULL) {
        return -1;
    }
    return cJSON_GetNumberValue(node);
}



///////////////////////// END ROUTINES FOR INITIALIZATION ///////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////
///////////////////////// ROUTINES FOR SYNC ///////////////////////////////////////

//returns the fd_idx of the next peer that should be sent a sync msg to
//returns -1 if a sync msg was already sent to all peers
// important: non_negative_fd_idxs are the outgoing fds
int calculate_next_peer_fd(Peer peer, int non_negative_fd_idxs[MAX_PEER_COUNT], int non_neg_outgoing_fds_count) { // overthink
    for (int non_neg_peer_idxs_idx = 0; non_neg_peer_idxs_idx < non_neg_outgoing_fds_count; non_neg_peer_idxs_idx++) { // iterate over all real peer_fds_idx
        int peer_fd_idx = non_negative_fd_idxs[non_neg_peer_idxs_idx]; // pick a real peer_fd (which has to be non negative)
        int already_connected = 0;
        for (int tried_peer_fd_idx = 0; tried_peer_fd_idx < non_neg_outgoing_fds_count; tried_peer_fd_idx++) { // try to find a peer_fd (tried_peer_fd_idx) that is not in the contacted_peer_fds 
            if (peer.contacted_peer_fds[tried_peer_fd_idx] == peer.outgoing_peer_fds[peer_fd_idx]) { // if fulfilled the peer of that fd was already contacted
                already_connected = 1;
                break;
            }
        }
        if (!already_connected) { // that means the fd with index non_negative_fd_idxs[non_neg_peer_idxs_idx] is not in the contacted list
            return peer_fd_idx;
        }
    }

    return -1; // if all peers were messaged meaning none has to be messaged again
}


 // Send the sync msg to a peer at port peer.connected_peer_server_ports[peer_fd_idx]
SyncAckRequest create_sync_request(Peer peer, int peer_fd_idx, uint64_t request_msg_id) {
    SyncAckRequest sync_req;

    sync_req.request_msg_id = request_msg_id;

    sync_req.peer_index = peer_fd_idx;
    sync_req.peer_fd = peer.outgoing_peer_fds[peer_fd_idx];
    sync_req.src_peer_server_port = peer.server_port;

    sync_req.request_str = NULL;

    return sync_req;
}

SyncAckRequest create_random_sync_request(Peer peer, uint64_t request_msg_id, 
        int available_peers[MAX_PEER_COUNT], int available_peer_count) {
    
    SyncAckRequest sync_req;

    sync_req.request_msg_id = request_msg_id;

    sync_req.peer_index = available_peers[rand() % available_peer_count];
    sync_req.peer_fd = peer.outgoing_peer_fds[sync_req.peer_index];
    sync_req.src_peer_server_port = peer.server_port;

    sync_req.chat_id = rand() % MAX_CHAT_COUNT;

    sync_req.request_str = NULL;

    return sync_req;
}

void create_random_chat_and_msg_id(uint8_t* chat_id, uint8_t* msg_id) {
    *chat_id = rand() % MAX_CHAT_COUNT;
    *msg_id = rand() % MAX_MSG_COUNT;
}

SyncAckRequest create_sync_request_from_own_request(Peer peer, uint64_t request_msg_id, 
        int available_peers[MAX_PEER_COUNT], int available_peer_count, OwnRequest own_request) {
    
    SyncAckRequest sync_req;

    sync_req.request_msg_id = request_msg_id;

    sync_req.peer_index = available_peers[rand() % available_peer_count];
    sync_req.peer_fd = peer.outgoing_peer_fds[sync_req.peer_index];
    sync_req.src_peer_server_port = peer.server_port;

    sync_req.chat_id = own_request.chat_id;
    sync_req.msg_id = own_request.msg_id;

    sync_req.request_str = NULL;

    return sync_req;
}


// returns 1 if the req contains a get operation and 2 if it contains an update operation
int is_get_or_update(OwnRequest request) {
    if (strcmp(request.operation, "get_chats") == 0
        || strcmp(request.operation, "get_chat") == 0
        || strcmp(request.operation, "get_msg") == 0) {
            return 1;
        }
    return 2; 
}


// updated the own chats before sending a sync message
int update_own_chats(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH], OwnRequest request) {
    if (strcmp(request.operation, "add_msg") == 0) {
        return add_message_to_variable(chats, request.chat_id, request.msg);
    } else if (strcmp(request.operation, "edit_msg") == 0) {
        return edit_message_from_variable(chats, request.chat_id, request.msg_id, request.msg);
    }
    return -1;
}



// Generate the sync request as string according to the current workflow state:
// returns 0 if successful, -1 if not
// synq_req means it contains the sending peer's server port to evaluate it at the other peer
int generate_str_sync_request(SyncAckRequest* sync_req, WorkflowConfig wf, char* message_content) {
    
    int success = 0;

    if (wf.wf_chat_id >= MAX_CHAT_COUNT) {
        success = -2;
    } else if ( wf.wf_msg_id >= MAX_MSG_COUNT) {
        success = -1;
    } else {
        switch (wf.wf_state) { // AND THE PEER_FD? RANDOM? OR PRE DEFINED???
            case WORKFLOW_GET_CHAT:
                sync_req->request_str = get_chat_sync(sync_req->request_msg_id + 1,
                                        wf.wf_chat_id, sync_req->src_peer_server_port);
                break;

            case WORKFLOW_ADD_MSG:
                sync_req->request_str = add_message_sync(sync_req->request_msg_id + 1,
                                            wf.wf_chat_id, sync_req->src_peer_server_port, message_content);
                break;

            case WORKFLOW_VERIFY_ADD:
                sync_req->request_str = get_chat_sync(sync_req->request_msg_id + 1,
                                        wf.wf_chat_id, sync_req->src_peer_server_port);
                break;

            case WORKFLOW_EDIT_MSG:
                if (wf.wf_content_length > 0) {
                    sync_req->request_str = edit_message_sync(sync_req->request_msg_id + 1,
                                                wf.wf_chat_id,
                                                wf.wf_msg_id, sync_req->src_peer_server_port, message_content);
                }
                break;

            case WORKFLOW_VERIFY_EDIT:
                sync_req->request_str = get_message_sync(sync_req->request_msg_id + 1,
                                            wf.wf_chat_id,
                                            wf.wf_msg_id, sync_req->src_peer_server_port);
                break;
        }

    }

    printf("sync request: %s\n", sync_req->request_str);

    return success;
}


char* operation_from_workflow(WorkflowConfig wf_cfg) {
    switch (wf_cfg.wf_state) {
        case WORKFLOW_GET_CHAT:
            return "get_chat";
        case WORKFLOW_ADD_MSG:
            return "add_msg";
        case WORKFLOW_VERIFY_ADD:
            return "get_chat";
        case WORKFLOW_EDIT_MSG:
            return "edit_msg";
        case WORKFLOW_VERIFY_EDIT:
            return "get_msg";
    }
    return "ERROR!!!";
}


int update_workflow(WorkflowConfig* wf_cfg) {
    switch (wf_cfg->wf_state) {
        case WORKFLOW_GET_CHAT:
            wf_cfg->wf_state = WORKFLOW_ADD_MSG;
            break;
        case WORKFLOW_ADD_MSG:
            wf_cfg->wf_state = WORKFLOW_VERIFY_ADD;
            break;

        case WORKFLOW_VERIFY_ADD:
            wf_cfg->wf_state = WORKFLOW_GET_CHAT;
            break;
        case WORKFLOW_EDIT_MSG:
            wf_cfg->wf_state = WORKFLOW_VERIFY_EDIT;
            break;
        case WORKFLOW_VERIFY_EDIT:
            wf_cfg->wf_state = WORKFLOW_GET_CHAT;
            break;
    }
    return 0;
}




// send a sync request with the json msg
int send_sync(Peer peer, SyncAckRequest sync_req, int* sync_ack_pending, int pending_sync_peer_indexes[MAX_PEER_COUNT]) {
    if (send_request(sync_req.peer_fd, sync_req.request_str) == 0) {
        *sync_ack_pending = 1;
        pending_sync_peer_indexes[sync_req.peer_index] = 1;
    } else {
        fprintf(stderr, "Failed to send sync to peer on port %u\n",
                peer.connected_peer_server_ports[sync_req.peer_index]);
        return -1;
    }
    return 0;
}


//returns element in request["header"]["sync"] (meaning the sending peer's server port) if it is a sync msg and -1 if it is not a sync msg
int is_sync(JsonPdu pdu) {
    cJSON* node = cJSON_GetObjectItem(pdu.header, "sync");
    if (node == NULL) {
        return -1;
    }
    return cJSON_GetNumberValue(node);
}

//////////////////////// END ROUTINES FOR SYNC ////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////// ROUTINES FOR ACK ////////////////////////////////////////

// tries to send a sync req
// returns 0 if successful and -1 if an error occured
int create_and_send_ack_req(Peer peer, int peer_fd_idx, uint64_t request_msg_id,
        int success, int16_t src_server_port) {
    SyncAckRequest ack_req = create_ack_request(peer, peer_fd_idx, request_msg_id, 
        success, src_server_port);
    if (send_ack(peer, ack_req) < 0) {
        return -1;
    }
    return 0;
}

//creating a json response
cJSON* create_json_ack(uint8_t success, uint16_t src_server_port) {
    uint64_t response_json_msg_id = 424244;
    uint64_t response_timestamp = (uint64_t)millis();
    
    cJSON *response = cJSON_CreateObject();
    cJSON *response_header = cJSON_CreateObject();
    cJSON *response_payload = cJSON_CreateObject();

    cJSON_AddItemToObject(response, "header", response_header);
    cJSON_AddNumberToObject(response_header, "json_msg_id", response_json_msg_id);
    cJSON_AddNumberToObject(response_header, "time_stamp", response_timestamp);
    cJSON_AddNumberToObject(response_header, "ack", src_server_port);

    cJSON_AddItemToObject(response, "payload", response_payload);
    cJSON_AddNumberToObject(response_payload, "success", success);

    return response;
}

// returns the ack req for a peer sending ack back to peer at idx peer_fd_idx
SyncAckRequest create_ack_request(Peer peer, int peer_fd_idx, uint64_t request_msg_id, 
        uint8_t success, uint16_t src_server_port) {
    SyncAckRequest ack_req;

    ack_req.request_msg_id = request_msg_id;

    ack_req.peer_index = peer.outgoing_peer_fds[peer_fd_idx];
    ack_req.peer_fd = peer.outgoing_peer_fds[peer_fd_idx];
    ack_req.src_peer_server_port = peer.server_port;

    cJSON* json_req = create_json_ack(success, src_server_port);
    
    ack_req.request_str = cJSON_PrintUnformatted(json_req);
    printf("Ack request: %s\n", ack_req.request_str);
    cJSON_Delete(json_req);

    return ack_req;
}

int send_ack(Peer peer, SyncAckRequest ack_req) {
    if (send_request(ack_req.peer_fd, ack_req.request_str) == 0) {
        return 0;
    } else {
        fprintf(stderr, "Failed to send ack to peer on port %u\n",
                peer.connected_peer_server_ports[ack_req.peer_index]);
        return -1;
    }
}


//returns element in request["header"]["ack"] (meaning the sending peer's server port) if it is an ack msg and -1 if it is not an ack msg
int is_ack(JsonPdu pdu) {
    cJSON* node = cJSON_GetObjectItem(pdu.header, "ack");
    if (node == NULL) {
        return -1;
    }
    return cJSON_GetNumberValue(node);
}


// process the received ack response
int process_ack(Peer *peer, int peer_fd_idx, int peer_fd, uint8_t success) {
    if (success == 1) {
        printf("Synchronization with peer fd %d was successful.\n",
               peer_fd);
        (peer->successfully_synchronized_peer_fds)[peer_fd_idx] = peer_fd;
        return 0;
    }
    fprintf(stderr,
            "Synchronization with peer fd %d failed.\n",
            peer_fd);
    return -1;
}



int pdu_processing(JsonPdu pdu, Peer* peer) {
    OwnRequest own_request;
    own_request.chat_id = cJSON_GetNumberValue(pdu.chat_id);
    own_request.operation = cJSON_GetStringValue(pdu.operation);
    own_request.msg_id = 0;
    own_request.msg = "";

    printf("own_request from incoming sync request:\n"
        "own_request.chat_id = %d, own_request.msg_id = %d, own_request.operation = %s, "
        "own_request.msg = %s\n", own_request.chat_id, own_request.msg_id, own_request.operation, own_request.msg);

    // NOW: CALL GET METHODS IF OPERATION IS A GET OP
    // check first if get and then call if it is a get method
    // get: means print to console here
    if (strcmp(own_request.operation, "get_chats") == 0) {
        printf("======= GET CHATS =======\n");
        for (int chat_idx = 0; chat_idx < 10; chat_idx++) {
            print_chat(peer->chats, chat_idx);
        }
        printf("=========================\n");
        return 0;
    } else if (strcmp(own_request.operation, "get_chat") == 0) {
        printf("======= GET CHAT =======\n");
        print_chat(peer->chats, own_request.chat_id);
        printf("========================\n");
        return 0;
    } else if (strcmp(own_request.operation, "get_msg") == 0) {
        printf("======= GET MSG =======\n");
        printf("%s\n", get_message_from_variable(peer->chats, own_request.chat_id, own_request.msg_id));
        printf("=======================\n");
        return 0;
    }

    // else it has to be an updating method:
    // manipulating the own chats var with my own_req and print the resulting chats
    // both manipulating functions have the field content and only edit_msg has the field msg_id
    own_request.msg = cJSON_GetStringValue(pdu.content);
    if (strcmp(own_request.operation, "edit_msg") == 0) {
        own_request.msg_id = (uint8_t) cJSON_GetNumberValue(pdu.msg_id);
    }

    else if (strcmp(own_request.operation, "add_msg") == 0 || strcmp(own_request.operation, "edit_msg") == 0) {
        if (update_own_chats(peer->chats, own_request) != 0) {
            return 1;
        }
        print_chat(peer->chats, own_request.chat_id);
        printf("ALL CHATS AFTER SYNC:\n");
        for (int i = 0; i < MAX_CHAT_COUNT; i++) {
            print_chat(peer->chats, i);
        }
    }

    else  {
        printf("Invalid command for an update!\n");
    }

    return 0;
}

//////////////////////// END ROUTINES FOR ACK /////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
