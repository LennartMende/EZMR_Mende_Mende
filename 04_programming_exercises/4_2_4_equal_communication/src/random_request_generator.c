#include "random_request_generator.h"

#include "peer_utils.h"

#include <stdlib.h>
#include <time.h>
#include <cjson/cJSON.h>


static cJSON *create_request(uint32_t request_msg_id, const char *operation) {

    request_msg_id--;

    if (request_msg_id == UINT32_MAX) {
        request_msg_id = 0;
    }
    else {
        request_msg_id++;
    }

    cJSON *request = cJSON_CreateObject();
    cJSON *header = cJSON_CreateObject();
    cJSON *payload = cJSON_CreateObject();

    cJSON_AddItemToObject(request, "header", header);
    cJSON_AddNumberToObject(header, "json_msg_id", request_msg_id);
    cJSON_AddNumberToObject(header, "time_stamp", (uint64_t)millis());

    cJSON_AddItemToObject(request, "payload", payload);
    cJSON_AddStringToObject(payload, "operation", operation);

    return request;
}


// creates a synchronization request from peer with server on src_peer_server_port
static cJSON *create_sync_request(uint32_t request_msg_id, const char *operation, pid_t src_pid) {

    request_msg_id--;

    if (request_msg_id == UINT32_MAX) {
        request_msg_id = 0;
    }
    else {
        request_msg_id++;
    }

    cJSON *request = cJSON_CreateObject();
    cJSON *header = cJSON_CreateObject();
    cJSON *payload = cJSON_CreateObject();

    cJSON_AddItemToObject(request, "header", header);
    cJSON_AddNumberToObject(header, "json_msg_id", request_msg_id);
    cJSON_AddNumberToObject(header, "time_stamp", (uint64_t)millis());
    cJSON_AddNumberToObject(header, "sync", src_pid);

    cJSON_AddItemToObject(request, "payload", payload);
    cJSON_AddStringToObject(payload, "operation", operation);

    return request;
}



static int generate_random_value(int offset, int max) {

    int random_value = offset + (rand() % max);

    return random_value;
}



char* generate_random_content(void) {

    // generate random length for the message content between 1 and 99 characters
    int message_length = generate_random_value(1, 99);

    char *message_content = malloc(message_length + 1);

    if (message_content == NULL) {
        return NULL;
    }

    // generate random alphanumeric characters for the message content
    for (int i = 0; i < message_length; i++) {

        // 26 lower case + 26 upper case + 10 digits = 62 random characters
        int random_char = rand() % 62;

        if (random_char < 26) {
            message_content[i] = 'a' + random_char;
        }
        else if (random_char < 52) {
            message_content[i] = 'A' + (random_char - 26);
        }
        else {
            message_content[i] = '0' + (random_char - 52);
        }
    }

    message_content[message_length] = '\0';

    return message_content;
}





char* get_chat(uint32_t request_msg_id, uint8_t chat_id) {
    
    cJSON *request = create_request(request_msg_id, "get_chat");
    cJSON *payload = cJSON_GetObjectItem(request, "payload");

    cJSON_AddNumberToObject(payload, "chat_id", chat_id);

    char *json_request = cJSON_PrintUnformatted(request);

    cJSON_Delete(request);

    return json_request;
}



char* get_message(uint32_t request_msg_id, uint8_t chat_id, int msg_id) {

    cJSON *request = create_request(request_msg_id, "get_msg");
    cJSON *payload = cJSON_GetObjectItem(request, "payload");

    cJSON_AddNumberToObject(payload, "chat_id", chat_id);
    cJSON_AddNumberToObject(payload, "msg_id", msg_id);

    char *json_request = cJSON_PrintUnformatted(request);

    cJSON_Delete(request);

    return json_request;
}



char* add_message(uint64_t request_msg_id, uint8_t chat_id) {
    
    cJSON *request = create_request(request_msg_id, "add_msg");
    cJSON *payload = cJSON_GetObjectItem(request, "payload");

    char *message_content = generate_random_content();

    cJSON_AddNumberToObject(payload, "chat_id", chat_id);
    cJSON_AddStringToObject(payload, "content", message_content);

    char *json_request = cJSON_PrintUnformatted(request);

    free(message_content);
    cJSON_Delete(request);

    return json_request;
}



char* edit_message(uint32_t request_msg_id, uint8_t chat_id, int msg_id) {
    
    cJSON *request = create_request(request_msg_id, "edit_msg");
    cJSON *payload = cJSON_GetObjectItem(request, "payload");

    char *message_content = generate_random_content();

    cJSON_AddNumberToObject(payload, "chat_id", chat_id);
    cJSON_AddNumberToObject(payload, "msg_id", msg_id);
    cJSON_AddStringToObject(payload, "content", message_content);

    char *json_request = cJSON_PrintUnformatted(request);

    free(message_content);
    cJSON_Delete(request);

    return json_request;
}



// NOW METHODS FOR SYNC

char* get_chat_sync(uint32_t request_msg_id, uint8_t chat_id, pid_t src_pid) {
    
    cJSON *request = create_sync_request(request_msg_id, "get_chat", src_pid);
    cJSON *payload = cJSON_GetObjectItem(request, "payload");

    cJSON_AddNumberToObject(payload, "chat_id", chat_id);

    char *json_request = cJSON_PrintUnformatted(request);

    cJSON_Delete(request);

    return json_request;
}



char* get_message_sync(uint32_t request_msg_id, uint8_t chat_id, int msg_id, pid_t src_pid) {

    cJSON *request = create_sync_request(request_msg_id, "get_msg", src_pid);
    cJSON *payload = cJSON_GetObjectItem(request, "payload");

    cJSON_AddNumberToObject(payload, "chat_id", chat_id);
    cJSON_AddNumberToObject(payload, "msg_id", msg_id);

    char *json_request = cJSON_PrintUnformatted(request);

    cJSON_Delete(request);

    return json_request;
}



char* add_message_sync(uint64_t request_msg_id, uint8_t chat_id, pid_t src_pid, char* message_content) {
    
    cJSON *request = create_sync_request(request_msg_id, "add_msg", src_pid);
    cJSON *payload = cJSON_GetObjectItem(request, "payload");

    //char *message_content = generate_random_content();

    cJSON_AddNumberToObject(payload, "chat_id", chat_id);
    cJSON_AddStringToObject(payload, "content", message_content);

    char *json_request = cJSON_PrintUnformatted(request);

    free(message_content);
    cJSON_Delete(request);

    return json_request;
}



char* edit_message_sync(uint32_t request_msg_id, uint8_t chat_id, int msg_id, pid_t src_pid, char* message_content) {
    
    cJSON *request = create_sync_request(request_msg_id, "edit_msg", src_pid);
    cJSON *payload = cJSON_GetObjectItem(request, "payload");

    //char *message_content = generate_random_content();

    cJSON_AddNumberToObject(payload, "chat_id", chat_id);
    cJSON_AddNumberToObject(payload, "msg_id", msg_id);
    cJSON_AddStringToObject(payload, "content", message_content);

    char *json_request = cJSON_PrintUnformatted(request);

    free(message_content);
    cJSON_Delete(request);

    return json_request;
}