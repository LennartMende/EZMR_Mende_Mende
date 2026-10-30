#include "error_handling.h"

void error_function_with_response(const uint8_t error_code, uint32_t *response_json_msg_id, uint8_t client_fd) {

    char failure_reason[80];

    switch (error_code) {
        case INVALID_JSON_FORMAT:
            strcpy(failure_reason, "No valid json format");
            break;
        case HEADER_MISSING:
            strcpy(failure_reason, "Header missing in json message");
            break;
        case PAYLOAD_MISSING:
            strcpy(failure_reason, "Payload missing in json message");
            break;
        case REQUIRED_JSON_FIELD_MISSING:
            strcpy(failure_reason, "Required field missing in json message");
            break;
        case WRONG_JSON_FIELD_DATATYPE:
            strcpy(failure_reason, "JSON field has the wrong datatype");
            break;
        case UNSUCCESSFUL_RESPONSE:
            strcpy(failure_reason, "JSON response was not successful");
            break;
        case INVALID_CHAT_ID:
            strcpy(failure_reason, "No valid chat id (has to be between 0 and 99)");
            break;
        case INVALID_MSG_ID:
            strcpy(failure_reason, "No valid message id (has to be between 0 and 99)");
            break;
        case INVALID_MSG_ID_FOR_EDIT:
            strcpy(failure_reason, "No valid message id (has to be smaller than number_of_msgs in this chat)");
            break;
        case INVALID_CONTENT_LENGTH:
            strcpy(failure_reason, "Content is too long");
            break;
        default:
            strcpy(failure_reason, "Unknown error");
            break;
    }

    printf("%s\n", failure_reason);
    fprintf(stderr, "%s\n", failure_reason);

    if (*response_json_msg_id == UINT32_MAX) {
        *response_json_msg_id = 0;
    } else {
        (*response_json_msg_id)++;
    }

    uint64_t response_timestamp = (uint64_t)time(NULL);

    cJSON *failure_response = cJSON_CreateObject();
    cJSON *failure_response_header = cJSON_CreateObject();
    cJSON *failure_response_payload = cJSON_CreateObject();

    // header
    cJSON_AddItemToObject(failure_response, "header", failure_response_header);
    cJSON_AddNumberToObject(failure_response_header, "json_msg_id", *response_json_msg_id);
    cJSON_AddNumberToObject(failure_response_header, "time_stamp", response_timestamp);
    // payload
    cJSON_AddItemToObject(failure_response, "payload", failure_response_payload);
    cJSON_AddNumberToObject(failure_response_payload, "success", error_code);
    cJSON_AddStringToObject(failure_response_payload, "failure_reason", failure_reason);

    // convert response json object to string
    char *failure_response_string = cJSON_PrintUnformatted(failure_response);

    if (failure_response_string == NULL) {
        printf("Failed to create response string\n");
    } else {
        printf("Sending response: %s\n", failure_response_string);

        // send response to client
        ssize_t sent = send(client_fd,
                            failure_response_string,
                            strlen(failure_response_string),
                            0);

        if (sent < 0) {
            fprintf(stderr, "Error occured during sending\n");
            printf("Error occured during sending\n");
        }
    }
}
