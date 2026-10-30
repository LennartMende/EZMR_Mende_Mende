#ifndef REQUEST_H
#define REQUEST_H

#include <stdint.h>
#include <sys/types.h>

typedef struct OwnGetReq {
    char* operation;
    uint8_t chat_id;
    uint8_t msg_id;
    char* msg;
} OwnRequest;

// MAYBE FOLLOWING FOR DIVIDING GET AND SET???
// typedef struct OwnGetReq {
//     char* operation;
//     uint8_t chat_id;
//     uint8_t msg_id;
// } OwnGetRequest;

// typedef struct OwnSetReq {
//     char* operation;
//     uint8_t chat_id;
//     uint8_t msg_id;
//     char* msg;
// } OwnSetRequest;



typedef struct Req {
    uint64_t request_msg_id; // increased if message is successfully sent

    int peer_index;
    int peer_fd;

    uint8_t msg_id;
    uint8_t chat_id;
    char* request_str;

} Request;

typedef struct SyncAckReq {
    uint64_t request_msg_id; // increased if message is successfully sent

    int fifo_index;
    int fifo_fd;

    pid_t src_pid; // pid of the sending (src) peer

    uint8_t msg_id;
    uint8_t chat_id;
    char* request_str;

} SyncAckRequest;

typedef struct InitSyncAckReq {
    int fifo_index;
    int fifo_fd;

    int src_pid; // server port of the sending (src) peer

    char* request_str;

} InitSyncAckRequest;

#endif