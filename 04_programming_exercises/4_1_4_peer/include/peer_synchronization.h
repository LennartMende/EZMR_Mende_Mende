#ifndef PEER_SYNCHRONIZATION_H
#define PEER_SYNCHRONIZATION_H

#include "peer.h"
#include "peer_client_utils.h"
#include "random_request_generator.h"



////////////// ROUTINES FOR THE INITIAL SYNCHRONIZATION OF THE PEER'S CHATS /////////////////
int initialize_chats_for_new_peer(Peer *peer);

// INIT SYNC
int send_init_sync(Peer peer, InitSyncAckRequest init_sync_req);
InitSyncAckRequest create_init_sync_req(Peer peer);
cJSON* fill_init_sync_req(InitSyncAckRequest* init_sync_req);
int is_init_sync(JsonPdu pdu);

// INIT ACK
InitSyncAckRequest create_init_ack_resp(Peer peer, int dest_server_port, char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH]);
cJSON* fill_init_ack_resp(InitSyncAckRequest* init_ack_resp, char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH]);
int send_init_ack(Peer peer, InitSyncAckRequest init_ack_resp);
int is_init_ack(JsonPdu pdu);

/////////////////////////////////////////////////////////////////////////////////////////////

// SYNC
int calculate_next_peer_fd(Peer peer, int non_negative_fd_idxs[MAX_PEER_COUNT], int non_neg_outgoing_fds_count);
int create_and_send_sync_req(Peer peer, int peer_fd_idx, uint64_t request_msg_id, WorkflowConfig wf_cfg,
        int* sync_pending, int* pending_sync_peer_index);
SyncAckRequest create_sync_request(Peer peer, int peer_fd_idx, uint64_t request_msg_id);
SyncAckRequest create_random_sync_request(Peer peer, uint64_t request_msg_id, 
        int available_peers[MAX_PEER_COUNT], int available_peer_count);
int update_own_chats(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH], OwnRequest request);
int generate_str_sync_request(SyncAckRequest* sync_req, WorkflowConfig wf, char* message_content);
int update_workflow(WorkflowConfig* wf_cfg);
char* operation_from_workflow(WorkflowConfig wf_cfg);
int send_sync(Peer peer, SyncAckRequest sync_req, int* sync_ack_pending, int pending_sync_peer_indexes[MAX_PEER_COUNT]);
int is_sync(JsonPdu pdu);

// ACK
int create_and_send_ack_req(Peer peer, int peer_fd_idx, uint64_t request_msg_id, int success, int16_t src_server_port);
cJSON* create_json_ack(uint8_t success, uint16_t src_server_port);
SyncAckRequest create_ack_request(Peer peer, int peer_fd_idx, uint64_t request_msg_id, 
        uint8_t success, uint16_t src_server_port);
void create_random_chat_and_msg_id(uint8_t* chat_id, uint8_t* msg_id);
SyncAckRequest create_sync_request_from_own_request(Peer peer, uint64_t request_msg_id, 
        int available_peers[MAX_PEER_COUNT], int available_peer_count, OwnRequest own_request);
int send_ack(Peer peer, SyncAckRequest ack_req);
int is_ack(JsonPdu pdu);
int process_ack(Peer *peer, int peer_fd_idx, int peer_fd, uint8_t success);

int pdu_processing(JsonPdu pdu, Peer* peer);

// int synchronizing_peers(Peer peer);

#endif
