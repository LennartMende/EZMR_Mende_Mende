#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <cjson/cJSON.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>


enum ErrorCode {
    INVALID_JSON_FORMAT = 1,
    HEADER_MISSING,
    PAYLOAD_MISSING,
    REQUIRED_JSON_FIELD_MISSING,
    WRONG_JSON_FIELD_DATATYPE,
    UNSUCCESSFUL_RESPONSE,
    INVALID_CHAT_ID,
    INVALID_MSG_ID,
    INVALID_MSG_ID_FOR_EDIT,
    INVALID_CONTENT_LENGTH
};

void error_function_with_response(const uint8_t error_code, uint32_t *response_json_msg_id, uint8_t client_fd);

#endif
