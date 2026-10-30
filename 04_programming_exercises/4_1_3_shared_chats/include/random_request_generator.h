#ifndef RANDOM_REQUEST_GENERATOR_H
#define RANDOM_REQUEST_GENERATOR_H

#include <stdint.h>

char* get_chat(uint32_t request_msg_id, uint8_t chat_id);
char* get_message(uint32_t request_msg_id, uint8_t chat_id, int msg_id);
char* add_message(uint32_t request_msg_id, uint8_t chat_id);
char* edit_message(uint32_t request_msg_id, uint8_t chat_id, int msg_id);

#endif
