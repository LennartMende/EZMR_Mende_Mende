#include "pdu_parsing.h"
#include "peer_utils.h"

#include "error_handling.h"


int test_json_format(cJSON* json_node, const char* node_name) {
    int success = 0;
    if (json_node == NULL) {
        printf("%s is invalid json format\n", node_name);
        fprintf(stderr, "%s is invalid json format\n", node_name);
        success = -1;
    }
    return success;
}

int exists_json_node(cJSON* parent_node, cJSON** node, const char* node_name) {
    int success = 0;
    *node = cJSON_GetObjectItem(parent_node, node_name);
    if (*node == NULL) {
        if (parent_node->string != NULL) {
            printf("The %s's parent node %s has not the required field %s\n", node_name, parent_node->string, node_name);
            fprintf(stderr, "%s has not the required field %s\n", parent_node->string, node_name);
        } else {
            printf("The parent node has not the required field %s\n", node_name);
            fprintf(stderr, "The parent node has not the required field %s\n", node_name);
        }
        
        success = -1;
    }
    return success;
}

int is_json_node_number(cJSON* node) {
    int success = 0;
    if(!cJSON_IsNumber(node)) {
        printf("%s is no number\n", node->string);
        fprintf(stderr, "%s is no number\n", node->string);
        success = -1;
    }
    return success;
}

int is_json_node_array(cJSON* node) {
    int success = 0;
    if(!cJSON_IsArray(node)) {
        printf("%s is no array\n", node->string);
        fprintf(stderr, "%s is no array\n", node->string);
        success = -1;
    }
    return success;
}

int is_json_node_string(cJSON* node) {
    int success = 0;
    if(!cJSON_IsString(node)) {
        printf("%s is no string\n", node->string);
        fprintf(stderr, "%s is no string\n", node->string);
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
            fprintf(stderr, "%s is not a valid 32 bit unsigned integer\n", node->string);
            success = -1;
        }
    } else if(bits == 64) {
        *value = cJSON_GetNumberValue(node);
        if (*value < 0 || *value != (uint64_t)*value) {
            printf("%s is not a valid 64 bit unsigned integer\n", node->string);
            fprintf(stderr, "%s is not a valid 64 bit unsigned integer\n", node->string);
            success = -1;
        }
    }
    return success;
}


//functions for parsing single json nodes
int test_json_msg(JsonPdu* pdu, uint32_t *response_json_msg_id, int client_fd) {
    int success = 0;
    if(test_json_format(pdu->json_msg, "json_msg") < 0) { // json_msg is not json conform
        error_function_with_response(INVALID_JSON_FORMAT, 
            response_json_msg_id, client_fd);
        success = -1;
    }
    return success;
}

int test_header(JsonPdu* pdu, uint32_t *response_json_msg_id, int client_fd) {
    int success = 0;
    if(exists_json_node(pdu->json_msg, &(pdu->header), "header") < 0) { // json_node has no header field
        error_function_with_response(HEADER_MISSING, 
            response_json_msg_id, client_fd);
        success = -1;
    }
    return success;
}
    
int test_json_msg_id(JsonPdu* pdu, uint32_t *response_json_msg_id, int client_fd) {
    int success = 0;
    double id;
    if(exists_json_node(pdu->header, &(pdu->json_msg_id), "json_msg_id") < 0) { // no json_msg_id field existing
        error_function_with_response(REQUIRED_JSON_FIELD_MISSING, 
            response_json_msg_id, client_fd);
        success = -1;
    } else if (is_json_node_number(pdu->json_msg_id) < 0) {
        error_function_with_response(WRONG_JSON_FIELD_DATATYPE, 
            response_json_msg_id, client_fd);
        success = -1;
    } else if (is_json_node_uint(pdu->json_msg_id, &id, 32) < 0) {
        error_function_with_response(WRONG_JSON_FIELD_DATATYPE, 
            response_json_msg_id, client_fd);
        success = -1;
    }
    return success;
}

int test_time_stamp(JsonPdu* pdu, uint32_t *response_json_msg_id, int client_fd) {
    int success = 0;
    double time_stamp;
    if(exists_json_node(pdu->header, &(pdu->time_stamp), "time_stamp") < 0) { // no time_stamp field existing
        error_function_with_response(REQUIRED_JSON_FIELD_MISSING, 
            response_json_msg_id, client_fd);
        success = -1;
    } else if (is_json_node_number(pdu->time_stamp) < 0) {
        error_function_with_response(WRONG_JSON_FIELD_DATATYPE, 
            response_json_msg_id, client_fd);
        success = -1;
    } else if (is_json_node_uint(pdu->time_stamp, &time_stamp, 64) < 0) {
        error_function_with_response(WRONG_JSON_FIELD_DATATYPE, 
            response_json_msg_id, client_fd);
        success = -1;
    }
    return success;
}

int test_payload(JsonPdu* pdu, uint32_t *response_json_msg_id, int client_fd) {
    int success = 0;
    if(exists_json_node(pdu->json_msg, &(pdu->payload), "payload") < 0) { // payload field missing
        error_function_with_response(PAYLOAD_MISSING, 
            response_json_msg_id, client_fd);
        success = -1;
    }
    return success;
}

int test_response_success(JsonPdu* pdu, uint32_t *response_json_msg_id, int client_fd) {
    int success = 0;
    if(exists_json_node(pdu->payload, &(pdu->response_success), "success") < 0) { // no json_msg_id field existing
        error_function_with_response(REQUIRED_JSON_FIELD_MISSING, 
            response_json_msg_id, client_fd);
        success = -1;
    } else if (is_json_node_number(pdu->response_success) != 0) {
        error_function_with_response(UNSUCCESSFUL_RESPONSE, 
            response_json_msg_id, client_fd);
        success = -1;
    }
    return success;
}

int test_operation(JsonPdu* pdu, uint32_t *response_json_msg_id, int client_fd) {
    int success = 0;
    if(exists_json_node(pdu->payload, &(pdu->operation), "operation") < 0) {
        error_function_with_response(REQUIRED_JSON_FIELD_MISSING, 
            response_json_msg_id, client_fd);
        success = -1;
    } else if (is_json_node_string(pdu->operation) < 0) {
        error_function_with_response(WRONG_JSON_FIELD_DATATYPE, 
            response_json_msg_id, client_fd);
        success = -1;
    }
    return success;
}

int test_chat_id(JsonPdu* pdu, uint32_t *response_json_msg_id, int client_fd) {
    int success = 0;
    uint8_t chat_id;
    if(exists_json_node(pdu->payload, &(pdu->chat_id), "chat_id") < 0) { // operands field missing
        error_function_with_response(REQUIRED_JSON_FIELD_MISSING, 
            response_json_msg_id, client_fd);
        success = -1;
    } else if (is_json_node_number(pdu->chat_id) < 0) {
        error_function_with_response(WRONG_JSON_FIELD_DATATYPE, 
            response_json_msg_id, client_fd);
        success = -1;
    } else {
        chat_id = cJSON_GetNumberValue(pdu->chat_id);
        if (chat_id >= MAX_CHAT_COUNT) {
            printf("chat_id cannot be > 99\n");
            fprintf(stderr, "chat_id cannot be < 0 or > 99\n");
            error_function_with_response(INVALID_CHAT_ID, 
                response_json_msg_id, client_fd);
            success = -1;
        }
    }
    return success;
}

                    

int test_msg_id(JsonPdu* pdu, uint32_t *response_json_msg_id, int client_fd, const char* operation_value) {
    int success = 0;
    uint8_t msg_id;
    if ((strcmp(operation_value, "get_msg") == 0) || (strcmp(operation_value, "edit_msg") == 0)) {
        if(exists_json_node(pdu->payload, &(pdu->msg_id), "msg_id") < 0) { // operands field missing
            error_function_with_response(REQUIRED_JSON_FIELD_MISSING, 
                response_json_msg_id, client_fd);
            success = -1;
        } else if (is_json_node_number(pdu->msg_id) < 0) {
            error_function_with_response(WRONG_JSON_FIELD_DATATYPE, 
                response_json_msg_id, client_fd);
            success = -1;
        } else {
            msg_id = cJSON_GetNumberValue(pdu->msg_id);
            if (msg_id >= MAX_MSG_COUNT) {
                printf("msg_id cannot be > %d\n", MAX_MSG_COUNT);
                fprintf(stderr, "msg_id cannot be < 0 or > %d\n", MAX_MSG_COUNT);
                error_function_with_response(INVALID_MSG_ID, 
                    response_json_msg_id, client_fd);
                success = -1;
            }
        }
    }
    return success;
}


// tests if there is a valid chats field in the ack response
// if chats valid: stores it into chats
// test_content only used for ack testing
int test_and_retrieve_content(JsonPdu pdu, char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH]) {
    if (exists_json_node(pdu.payload, &(pdu.content), "content") < 0) {
        error_function_with_response(
            REQUIRED_JSON_FIELD_MISSING, NULL, 0); // client_fd == 0 doesn't matter because *response_json_msg_id == NULL
        return -1;
    }

    if (!cJSON_IsArray(pdu.content)) {
        error_function_with_response(
            WRONG_JSON_FIELD_DATATYPE, NULL, 0); // client_fd == 0 doesn't matter because *response_json_msg_id == NULL
        return -1;
    }

    int chat_count = cJSON_GetArraySize(pdu.content);
    if (chat_count > MAX_CHAT_COUNT) {
        fprintf(stderr, "Too many chats received\n");
        return -1;
    }

    // clear old chats var
    memset(chats, 0,
           sizeof(char) * MAX_CHAT_COUNT * MAX_MSG_COUNT * MAX_MSG_LENGTH);

    //iterate over all chats
    for (int chat_idx = 0; chat_idx < chat_count; chat_idx++) {
        cJSON *chat = cJSON_GetArrayItem(pdu.content, chat_idx);
        if (!cJSON_IsArray(chat)) {
            fprintf(stderr, "Invalid chat entry\n");
            return -1;
        }

        int msg_count = cJSON_GetArraySize(chat);
        if (msg_count > MAX_MSG_COUNT) {
            fprintf(stderr, "Too many messages in chat %d\n", chat_idx);
            return -1;
        }

        for (int msg_idx = 0; msg_idx < msg_count; msg_idx++) {
            cJSON *msg = cJSON_GetArrayItem(chat, msg_idx);
            if (!cJSON_IsString(msg)) {
                fprintf(stderr, "Invalid message in chat %d\n", chat_idx);
                return -1;
            }
            const char *message = cJSON_GetStringValue(msg);
            size_t len = strlen(message);

            if (len >= MAX_MSG_LENGTH) {
                fprintf(stderr, "Message too long\n");
                return -1;
            }
            memcpy(chats[chat_idx][msg_idx], message, len + 1);
        }
    }

    return 0;
}


int test_content_and_process_sync(JsonPdu* pdu, uint32_t *response_json_msg_id, int client_fd, const char* operation_value,
            int MAX_CLIENT_COUNT, char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_PEER_COUNT], uint8_t chat_id, uint8_t msg_id, uint8_t* new_msg_id) {

    int success = 0;
    if (chat_id >= MAX_CHAT_COUNT || msg_id >= MAX_MSG_COUNT) {
        return -1;
    }
    // CONTENT
    char request_content[MAX_MSG_LENGTH];
    if (strcmp(operation_value, "add_msg") == 0) {
        if(exists_json_node(pdu->payload, &(pdu->content), "content") < 0) { // content field missing
            error_function_with_response(REQUIRED_JSON_FIELD_MISSING, 
                    response_json_msg_id, client_fd);
            success = -1;
        } else if (is_json_node_string(pdu->content) < 0) {
            error_function_with_response(WRONG_JSON_FIELD_DATATYPE, 
                    response_json_msg_id, client_fd);
            success = -1;
        } else {
            if (strlen(cJSON_GetStringValue(pdu->content)) > MAX_MSG_COUNT) {
                printf("message length cannot be < 0 or > 99\n");
                fprintf(stderr, "message length cannot be < 0 or > 99\n");
                error_function_with_response(INVALID_CONTENT_LENGTH, 
                    response_json_msg_id, client_fd);
                success = -1;
            } else {
                strcpy(request_content, cJSON_GetStringValue(pdu->content));
                uint8_t number_of_msgs = 0;
                for (int j = 0; j < MAX_CLIENT_COUNT; j++) {
                    if (strcmp(chats[chat_id][j], "") != 0) {
                        number_of_msgs++;
                    }
                }
                *new_msg_id = number_of_msgs;
                add_message_to_variable(chats, chat_id, request_content);
            }
        }

        
    } else if (strcmp(operation_value, "edit_msg") == 0) {
        if(exists_json_node(pdu->payload, &(pdu->content), "content") < 0) { // content field missing
            error_function_with_response(REQUIRED_JSON_FIELD_MISSING, 
                    response_json_msg_id, client_fd);
            success = -1;
        } else if (is_json_node_string(pdu->content) < 0) {
            error_function_with_response(WRONG_JSON_FIELD_DATATYPE, 
                    response_json_msg_id, client_fd);
            success = -1;
        } else {
            if (strlen(cJSON_GetStringValue(pdu->content)) > 99) {
                printf("message length cannot be < 0 or > 99\n");
                fprintf(stderr, "message length cannot be < 0 or > 99\n");
                error_function_with_response(INVALID_CONTENT_LENGTH, 
                    response_json_msg_id, client_fd);
                success = -1;
            } else {
                strcpy(request_content, cJSON_GetStringValue(pdu->content));
                uint8_t number_of_msgs = 0;
                for (int j = 0; j < MAX_CLIENT_COUNT; j++) {
                    if (strcmp(chats[chat_id][j], "") != 0) {
                        number_of_msgs++;
                    }
                }
                if (msg_id >= number_of_msgs) { //error because non existent msg is trying to be edited
                    printf("msg id to be edited cannot be bigger than number_of_msgs\n");
                    fprintf(stderr, "msg id to be edited cannot be bigger than number_of_msgs\n");
                    error_function_with_response(INVALID_MSG_ID_FOR_EDIT, 
                        response_json_msg_id, client_fd);
                    success = -1;
                }
                if (success == 0) {
                    if (edit_message_from_variable(chats, chat_id, msg_id, request_content) < 0) {
                        fprintf(stderr, "ERROR EDITING THE MSG IN PEER SERVER!: pdu_parsing.c");
                    }
                }
            }
        }
    }

    return success;
}



//creating a json response
cJSON* create_json_response(uint32_t* response_json_msg_id, uint8_t chat_id) {
    if (*response_json_msg_id == UINT32_MAX) {
        *response_json_msg_id = 0;
    } else {
        (*response_json_msg_id)++;
    }

    uint64_t response_timestamp = (uint64_t)millis();

    uint8_t response_chat_id = chat_id;
    
    cJSON *response = cJSON_CreateObject();
    cJSON *response_header = cJSON_CreateObject();
    cJSON *response_payload = cJSON_CreateObject();

    cJSON_AddItemToObject(response, "header", response_header);
    cJSON_AddNumberToObject(response_header, "json_msg_id", *response_json_msg_id);
    cJSON_AddNumberToObject(response_header, "time_stamp", response_timestamp);

    cJSON_AddItemToObject(response, "payload", response_payload);
    cJSON_AddNumberToObject(response_payload, "chat_id", response_chat_id);

    return response;
}

int create_get_msg_response(cJSON* response_payload, 
                                    uint8_t chat_id, uint8_t msg_id, char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH]) {
    
    if (chat_id >= MAX_CHAT_COUNT) {
        return -2;
    } else if (msg_id >= MAX_MSG_COUNT) {
        return -1;
    }

    cJSON_AddNumberToObject(response_payload, "msg_id", msg_id);
    const char* response_content = chats[chat_id][msg_id];
    cJSON_AddStringToObject(response_payload, "content", response_content);
    return 0;
}

int create_get_chat_response(cJSON* response_payload, 
                                    uint8_t chat_id, char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH], int MAX_CLIENT_COUNT) {

    if (chat_id >= MAX_CHAT_COUNT) {
        return -1;
    }

    cJSON *content_array = cJSON_AddArrayToObject(response_payload, "content");
    uint8_t number_of_msgs = 0;
    for (int j = 0; j < MAX_CLIENT_COUNT; j++) {
        if (strcmp(chats[chat_id][j], "") != 0) {
            number_of_msgs++;
        }
    }
    for (int j = 0; j < number_of_msgs; j++) {
        cJSON_AddItemToArray(content_array, cJSON_CreateString(chats[chat_id][j]));
    }

    return 0;
}

int create_add_msg_response(cJSON* response_payload, uint8_t chat_id, uint8_t new_msg_id) {

    if (chat_id >= MAX_CHAT_COUNT) {
        return -2;
    } else if (new_msg_id >= MAX_MSG_COUNT) {
        return -1;
    }

    cJSON_AddNumberToObject(response_payload, "msg_id", new_msg_id);
    char remark[53];
    snprintf(remark, sizeof(remark), "Message added as msg with id %i in chat with id %i", 
        new_msg_id, chat_id);
    cJSON_AddStringToObject(response_payload, "remark", remark);

    return 0;
}

int create_edit_msg_response(cJSON* response_payload, uint8_t chat_id, uint8_t msg_id) {

    if (chat_id >= MAX_CHAT_COUNT) {
        return -2;
    } else if (msg_id >= MAX_MSG_COUNT) {
        return -1;
    }

    cJSON_AddNumberToObject(response_payload, "msg_id", msg_id);
    char remark[53];
    snprintf(remark, sizeof(remark), "Edited msg with id %i in chat with id %i", 
        msg_id, chat_id);
    cJSON_AddStringToObject(response_payload, "remark", remark);

    return 0;
}

int test_response_content(JsonPdu* pdu, uint32_t *response_json_msg_id, int client_fd, size_t* content_length) {
    int success = 0;
    if(exists_json_node(pdu->payload, &(pdu->content), "content") < 0) { // operands field missing
        error_function_with_response(REQUIRED_JSON_FIELD_MISSING, 
            response_json_msg_id, client_fd);
        success = -1;
    } else if (is_json_node_array(pdu->content) < 0) {
        error_function_with_response(WRONG_JSON_FIELD_DATATYPE, 
            response_json_msg_id, client_fd);
        success = -1;
    } else {
        *content_length = cJSON_GetArraySize(pdu->content);
        if (*content_length > 99) {
            printf("content cannot be < 0 or > 99\n");
            fprintf(stderr, "content cannot be < 0 or > 99\n");
            error_function_with_response(INVALID_CONTENT_LENGTH, 
                response_json_msg_id, client_fd);
            success = -1;
        }
    }
    return success;
}
