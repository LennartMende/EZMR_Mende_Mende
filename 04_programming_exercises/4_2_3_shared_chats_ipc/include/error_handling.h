#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <cjson/cJSON.h>
#include <time.h>
#include <unistd.h>
#include <sys/socket.h>

// const uint8_t INVALID_JSON_FORMAT = 1;
// const uint8_t HEADER_MISSING = 2;
// const uint8_t PAYLOAD_MISSING = 3;
// const uint8_t REQUIRED_JSON_FIELD_MISSING = 4;
// const uint8_t WRONG_JSON_FIELD_DATATYPE = 5;
// const uint8_t INVALID_CHAT_ID = 6;
// const uint8_t INVALID_MSG_ID = 7;
// const uint8_t INVALID_CONTENT_LENGTH = 8;

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
    INVALID_CONTENT_LENGTH,
    INVALID_PID
};

void error_function_with_response(const uint8_t error_code, uint32_t *response_json_msg_id, uint8_t response_fifo_fd);
//char* error_message_from_error_code(const uint8_t error_code);

#endif