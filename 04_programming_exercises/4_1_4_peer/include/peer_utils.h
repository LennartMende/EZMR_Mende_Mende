#ifndef PEER_UTILS_H
#define PEER_UTILS_H

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <cjson/cJSON.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include "constants.h"
#include "peer_server_utils.h"
#include "peer_client_utils.h"
#include "peer.h"
#include "pdu_parsing.h"
#include "request.h"



typedef struct {
    cJSON *response;
    cJSON *header;
    cJSON *payload;
    cJSON *success;
    cJSON *chat_node;
} ParsedReceivedResponse;

typedef enum {
    WORKFLOW_GET_CHAT,
    WORKFLOW_ADD_MSG,
    WORKFLOW_VERIFY_ADD,
    WORKFLOW_EDIT_MSG,
    WORKFLOW_VERIFY_EDIT
} WorkflowState;

typedef struct WfCfg{
    WorkflowState wf_state;
    uint8_t wf_chat_id;
    size_t wf_content_length;
    int wf_msg_id;
} WorkflowConfig;

int64_t millis(void);

// for a peer
Peer create_first_peer(uint16_t server_port);
void generate_first_chats(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH]);

// general utils
void get_msg_count(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH], int msg_counts[MAX_CHAT_COUNT]);
cJSON* get_json_chats(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH], int chat_count);
char* read_file();
cJSON* get_json_chats_from_file();

// chat data
void initialize_chats();
int load_chats(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH]);
int save_chats(char saved_chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH]);
int save_peer_fds(int incoming_peer_fds[MAX_PEER_COUNT]);
int load_peer_fds(int incoming_peer_fds[MAX_PEER_COUNT]);

// peer management
int connect_peer();
int accept_peer();
int disconnect_peer();
int add_peer();
int remove_peer();

// synchronization
//int get_chats();
int send_chats();



void send_json_response(char* response_string, int peer_fd);

// chat routines
char (*get_chats_from_variable(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH]))[MAX_MSG_COUNT][MAX_MSG_LENGTH];
char (*get_chat_from_variable(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH], uint8_t chat_id))[MAX_MSG_LENGTH]; // pointer to first chat containing MAX_MSG_COUNT msgs
char* get_message_from_variable(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH], uint8_t chat_id, uint8_t msg_id);
int add_message_to_variable(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH], uint8_t chat_id, char* msg);
int edit_message_from_variable(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH], uint8_t chat_id, uint8_t msg_id, char* msg);
void print_chat(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH], uint8_t chat_id);


// count non negative (used) peer places
//int detect_gaps_in_fds(int peer_fd_list[MAX_PEER_COUNT], int non_negative_fd_idxs[MAX_PEER_COUNT]);

// rearange peer_fd lists
void reorder_outgoing_connections(
    int outgoing_peer_fds[MAX_PEER_COUNT],
    uint16_t connected_peer_server_ports[MAX_PEER_COUNT]);
void reorder_list(int peer_fds[MAX_PEER_COUNT], int print);
int detect_gaps_in_fds(int peer_fd_list[MAX_PEER_COUNT], int non_negative_fd_idxs[MAX_PEER_COUNT], int print);
int has_gaps(int peer_fd_list[MAX_PEER_COUNT], 
        int non_negative_fd_count, int gap_list_idxs[non_negative_fd_count], int print);
int rearange_peer_fd_list(int peer_fd_list[MAX_PEER_COUNT], int non_negative_fd_idxs[MAX_PEER_COUNT], 
        int non_negative_fd_count, int gap_list_idxs[non_negative_fd_count], int gap_count, int print);


// reorder peer and server fd lists and print them regularly 
void reorder_peer_lists(Peer* peer);
void reorder_peer_lists_and_print(Peer* peer, int64_t* last_rearange_time);


// worflow and request management
int generate_str_request(Request* req, WorkflowState workflow_state, uint8_t *workflow_chat_id, 
        size_t workflow_content_length, int *workflow_msg_id);
int update_workflow_state(ParsedReceivedResponse* pr_response, WorkflowState* workflow_state, uint8_t *workflow_chat_id, size_t* workflow_content_length);




#endif
