#include "pdu_parsing.h"
#include <stdio.h>
#include <stdint.h>

// int parse_json_msg(cJSON* json_msg) { // for parsing the complete json_msg
//     int success = 0;
//     return success;
// }


int test_json_format(cJSON* json_node, const char* node_name) {
    int success = 0;
    if (json_node == NULL) {
        printf("%s is invalid json format\n", node_name);
        fprintf(stderr, "%s is invalid json format\n", node_name);
        success = -1;
    } else {
        printf("Valid json format\n");
    }
    return success;
}

int exists_json_node(cJSON* parent_node, cJSON** node, const char* node_name) {
    int success = 0;
    *node = cJSON_GetObjectItem(parent_node, node_name);
    if (*node == NULL) {
        printf("%s has not the required field %s\n", parent_node->string, node_name);
        fprintf(stderr, "%s has not the required field %s\n", parent_node->string, node_name);
        success = -1;
    }
    return success;
}

int is_json_node_number(cJSON* node) {
    int success = 0;
    if(!cJSON_IsNumber(node)) {
        printf("%s is no number\n", node->string);
        fprintf(stderr, "%s is no number", node->string);
        success = -1;
    }
    return success;
}

int is_json_node_array(cJSON* node) {
    int success = 0;
    if(!cJSON_IsArray(node)) {
        printf("%s is no array\n", node->string);
        fprintf(stderr, "%s is no array", node->string);
        success = -1;
    }
    return success;
}

int is_json_node_string(cJSON* node) {
    int success = 0;
    if(!cJSON_IsString(node)) {
        printf("%s is no string\n", node->string);
        fprintf(stderr, "%s is no string", node->string);
        success = -1;
    }
    return success;
}


//returns 0 if node is a uint with the specified width, -1 otherwise
int is_json_node_uint(cJSON* node, double* value, unsigned int bits) {
    int success = 0;
    if(bits == 32) {
        *value = cJSON_GetNumberValue(node);
        if (*value < 0 || *value != (uint32_t)*value) {
            printf("%s is not a valid 32 bit unsigned integer\n", node->string);
            fprintf(stderr, "%s is not a valid 32 bit unsigned integer", node->string);
            success = -1;
        }
    } else if(bits == 64) {
        *value = cJSON_GetNumberValue(node);
        if (*value < 0 || *value != (uint64_t)*value) {
            printf("%s is not a valid 64 bit unsigned integer\n", node->string);
            fprintf(stderr, "%s is not a valid 64 bit unsigned integer", node->string);
            success = -1;
        }
    }
    return success;
}