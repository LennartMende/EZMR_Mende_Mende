#ifndef REQUEST_H
#define REQUEST_H

#include <stdint.h>

typedef struct OwnGetReq {
    char* operation;
    uint8_t chat_id;
    uint8_t msg_id;
    char* msg;
} OwnRequest;



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

    int peer_index;
    int peer_fd;

    int src_peer_server_port; // server port of the sending (src) peer

    uint8_t msg_id;
    uint8_t chat_id;
    char* request_str;

} SyncAckRequest;

typedef struct InitSyncAckReq {
    int peer_index;
    int peer_fd;

    int src_peer_server_port; // server port of the sending (src) peer

    char* request_str;

} InitSyncAckRequest;

#endif
