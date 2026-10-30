#ifndef RANDOM_REQUEST_GENERATOR_H
#define RANDOM_REQUEST_GENERATOR_H

#include <stdint.h>

int64_t millis(void);

char* generate_random_content(void);

char* get_chat(uint32_t request_msg_id, uint8_t chat_id);
char* get_message(uint32_t request_msg_id, uint8_t chat_id, int msg_id);
char* add_message(uint64_t request_msg_id, uint8_t chat_id);
char* edit_message(uint32_t request_msg_id, uint8_t chat_id, int msg_id);

char* get_chat_sync(uint32_t request_msg_id, uint8_t chat_id, uint16_t src_server_port);
char* get_message_sync(uint32_t request_msg_id, uint8_t chat_id, int msg_id, uint16_t src_server_port);
char* add_message_sync(uint64_t request_msg_id, uint8_t chat_id, uint16_t src_server_port, char* message_content);
char* edit_message_sync(uint32_t request_msg_id, uint8_t chat_id, int msg_id, uint16_t src_server_port, char* message_content);

#endif
