#ifndef PDU_PARSING_H
#define PDU_PARSING_H

#include <stdio.h>
#include <stdint.h>
#include <cjson/cJSON.h>

// struct for PDU
typedef struct {
    cJSON *json_msg;
    cJSON *header;
    cJSON *json_msg_id;
    cJSON *time_stamp;
    cJSON *payload;
    cJSON *operation;
    cJSON *chat_id;
    cJSON *msg_id;
    cJSON *content;
    cJSON *response_success;
} JsonPdu;


// function declarations
//general functions for arsing
int test_json_format(cJSON* json_node, const char* node_name); // 0 if json format and -1 if not json format
int exists_json_node(cJSON* parent_node, cJSON** node, const char* node_name); // 0 if node exists and -1 else
int is_json_node_number(cJSON* node); // 0 if node is a number and -1 else
int is_json_node_array(cJSON* node); // 0 if node is an array and -1 else
int is_json_node_string(cJSON* node); // 0 if node is a string and -1 else
int is_json_node_uint(cJSON* node, double* value, unsigned int bits); // 0 if node is a uint and -1 else


//functions for parsing real single json nodes
int test_json_msg(JsonPdu *pdu, uint32_t *response_json_msg_id, int client_fd);
int test_header(JsonPdu *pdu, uint32_t *response_json_msg_id, int client_fd);
int test_json_msg_id(JsonPdu *pdu, uint32_t *response_json_msg_id, int client_fd);
int test_time_stamp(JsonPdu *pdu, uint32_t *response_json_msg_id, int client_fd);
int test_payload(JsonPdu *pdu, uint32_t *response_json_msg_id, int client_fd);
int test_operation(JsonPdu *pdu, uint32_t *response_json_msg_id, int client_fd);
int test_chat_id(JsonPdu *pdu, uint32_t *response_json_msg_id, int client_fd);
int test_msg_id(JsonPdu *pdu, uint32_t *response_json_msg_id, int client_fd, const char* operation_value);
int test_and_retrieve_content(JsonPdu pdu, char chats[100][100][100]);
int test_content_and_process_sync(JsonPdu *pdu, uint32_t *response_json_msg_id, int client_fd, const char* operation_value,
            int MAX_CLIENT_COUNT, char chats[100][100][100], uint8_t chat_id, uint8_t msg_id, uint8_t* new_msg_id);

int test_response_content(JsonPdu* pdu, uint32_t *response_json_msg_id, int client_fd, size_t* content_length);
int test_response_success(JsonPdu* pdu, uint32_t *response_json_msg_id, int client_fd);


// functions for creating and filling the json response
cJSON* create_json_response(uint32_t* response_json_msg_id, uint8_t chat_id);
int create_get_msg_response(cJSON* response_payload, 
                                    uint8_t chat_id, uint8_t msg_id, char chats[100][100][100]);
int create_get_chat_response(cJSON* response_payload, 
                                    uint8_t chat_id, char chats[100][100][100], int MAX_CLIENT_COUNT);
int create_add_msg_response(cJSON* response_payload, uint8_t chat_id, uint8_t new_msg_id);
int create_edit_msg_response(cJSON* response_payload, uint8_t chat_id, uint8_t msg_id);

#endif