#ifndef PEER_UTILS_H
#define PEER_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <signal.h>

#include "constants.h"
#include "peer.h"
#include "peer_utils.h"
#include "peer_client_utils.h" // for Request struct


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

int register_fifo_files(Peer* peer, const char *peers_dir, int print);

Peer create_first_peer(pid_t pid);
void generate_first_chats(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH]);
void get_msg_count(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH], int msg_counts[MAX_CHAT_COUNT]);
void send_json_response(char* response_string, int outgoing_fifo_fd);

// chat routines
char (*get_chats_from_variable(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH]))[MAX_MSG_COUNT][MAX_MSG_LENGTH];
char (*get_chat_from_variable(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH], uint8_t chat_id))[MAX_MSG_LENGTH]; // pointer to first chat containing MAX_MSG_COUNT msgs
char* get_message_from_variable(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH], uint8_t chat_id, uint8_t msg_id);
int add_message_to_variable(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH], uint8_t chat_id, char* msg);
int edit_message_from_variable(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH], uint8_t chat_id, uint8_t msg_id, char* msg);
void print_chat(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH], uint8_t chat_id);

int update_workflow_state(ParsedReceivedResponse* pr_response, WorkflowState* workflow_state, uint8_t *workflow_chat_id, size_t* workflow_content_length);

/////////////////// FUNCTIONS TO REORDER LISTS ////////////////////////
void reorder_list(int fifo_fds[MAX_PEER_COUNT], char fifo_file_paths[MAX_PEER_COUNT][FIFO_PATH_LENGTH], int print);
int detect_gaps_in_fds(int fifo_fd_list[MAX_PEER_COUNT], int non_negative_fd_idxs[MAX_PEER_COUNT], int print);
int has_gaps(int fifo_fd_list[MAX_PEER_COUNT], 
        int non_negative_fd_count, int gap_list_idxs[non_negative_fd_count], int print);
int rearange_peer_fd_list(int fifo_fd_list[MAX_PEER_COUNT], int non_negative_fd_idxs[MAX_PEER_COUNT], 
        int non_negative_fd_count, int gap_list_idxs[non_negative_fd_count], int gap_count, 
        char fifo_file_paths[MAX_PEER_COUNT][FIFO_PATH_LENGTH], int print);
void reorder_peer_lists(Peer* peer);
void reorder_peer_lists_and_print(Peer* peer, int64_t* last_rearange_time);

int is_dest_pid_available(const char* peers_dir, pid_t dest_pid);
void restructure_peer(Peer* peer, int old_pid_idx, int64_t* last_rearange_time);

#endif