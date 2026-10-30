#ifndef PDU_PARSING_H
#define PDU_PARSING_H

#include <cjson/cJSON.h>

// struct for PDU
typedef struct {
    cJSON *json_msg;
    cJSON *header;
    cJSON *msg_id;
    cJSON *time_stamp;
    cJSON *payload;
    cJSON *operands;
    cJSON *operation;
} JsonPdu;

// function declarations
// int parse_json_msg(cJSON* json_msg);
int test_json_format(cJSON* json_node, const char* node_name);
int exists_json_node(cJSON* parent_node, cJSON** node, const char* node_name);
int is_json_node_number(cJSON* node);
int is_json_node_array(cJSON* node);
int is_json_node_string(cJSON* node);
int is_json_node_uint(cJSON* node, double* value, unsigned int bits);

#endif