#ifndef RANDOM_REQUEST_GENERATOR_H
#define RANDOM_REQUEST_GENERATOR_H

#include <stdint.h>
#include <sys/types.h>

char* get_chat(uint32_t request_msg_id, uint8_t chat_id, pid_t client_pid);
char* get_message(uint32_t request_msg_id, uint8_t chat_id, int msg_id, pid_t client_pid);
char* add_message(uint32_t request_msg_id, uint8_t chat_id, pid_t client_pid);
char* edit_message(uint32_t request_msg_id, uint8_t chat_id, int msg_id, pid_t client_pid);

#endif
