#define _POSIX_C_SOURCE 199309L

#include <poll.h>
#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <dirent.h>
#include <cjson/cJSON.h>
#include <signal.h>

#include "constants.h"
#include "pdu_parsing.h"
#include "error_handling.h"
#include "peer_utils.h"
#include "peer_server_utils.h"
#include "peer_client_utils.h"
#include "random_request_generator.h"
#include "peer_synchronization.h"


// static int sleep_ms(int ms) {
//     struct timespec ts_req, ts_rem;
//     ts_req.tv_sec = ms / MS_PER_S;
//     ts_req.tv_nsec = (ms % MS_PER_S) * NS_PER_MS; // ms % 1000 -> remaining "real" ms -> * 1000000 for nano secs

//     return (nanosleep(&ts_req, &ts_rem));
// }

volatile sig_atomic_t interrupted = 0; // to handle keyboard interrupts and manage resources

// signal handler to handle a keyboard interrupt
static void handle_sigint(int signal) {
    (void) signal;
    interrupted = 1;
}

static void cleanup_peer(Peer *peer, pid_t pid) {
    printf("\nInterrupt received. Cleaning up...\n");
    // remove peer registration file
    char own_pid_file[FIFO_PATH_LENGTH];
    snprintf(own_pid_file, sizeof(own_pid_file),
             "%s%d", PEERS_DIR, (int)pid);
    printf("Removing %s\n", own_pid_file);
    if (remove(own_pid_file) == -1 && errno != ENOENT) {
        perror("Could not remove peer registration file");
    }

    // close opened FIFOs
    for (int i = 0; i < MAX_PEER_COUNT; i++) {
        if (peer->incoming_fifo_fds[i] >= 0) {
            close(peer->incoming_fifo_fds[i]);
            peer->incoming_fifo_fds[i] = -1;
        }
        if (peer->outgoing_fifo_fds[i] >= 0) {
            close(peer->outgoing_fifo_fds[i]);
            peer->outgoing_fifo_fds[i] = -1;
        }
    }

    printf("Cleanup completed.\n");
}

// global vars for outgoing_fifos and incoming fifos
int outgoing_fifo_fds[MAX_PEER_COUNT];
char outgoing_fifo_file_paths[MAX_PEER_COUNT][FIFO_PATH_LENGTH];
int incoming_fifo_fds[MAX_PEER_COUNT];
char incoming_fifo_file_paths[MAX_PEER_COUNT][FIFO_PATH_LENGTH];

int main() {
    // register the interrupt handle to react on interrupts (e. g. closing resources)
    struct sigaction sa = {0};
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    // similar for sigpipe
    struct sigaction sa_pipe = {0};
    sa_pipe.sa_handler = SIG_IGN;
    sigemptyset(&sa_pipe.sa_mask);
    sa_pipe.sa_flags = 0;

    if (sigaction(SIGPIPE, &sa_pipe, NULL) == -1) {
        perror("sigaction SIGPIPE");
        return 1;
    }

    pid_t pid = getpid();

    // create directories and ignore if already exists
    // could be packed into a function
    if (mkdir(IPC_FILES_DIR, 0777) == -1 && errno != EEXIST) {
        fprintf(stderr, "Error occured during creating the IPC dir\n");
        return 1;
    }

    if (mkdir(PEERS_DIR, 0777) == -1 && errno != EEXIST) {
        fprintf(stderr, "Error occured during creating the peer dir\n");
        return 2;
    }

    if (mkdir(FIFOS_DIR, 0777) == -1 && errno != EEXIST) {
        fprintf(stderr, "Error occured during creating the FIFO dir\n");
        return 3;
    }

    // create a file in the peers folder to register the peer
    char peer_path[256];
    snprintf(peer_path, sizeof(peer_path), "%s%d", PEERS_DIR, (int)pid);
    FILE *file = fopen(peer_path, "w");
    if (file == NULL) {
        fprintf(stderr, "Error occured during opening a peer registration file\n");
        return 4;
    }
    fclose(file);
    printf("Registered peer with PID %d at %s\n", (int)pid, peer_path);

    // create the peer
    Peer peer = create_first_peer(pid);

    // discovery:
    int available_peer_count = register_fifo_files(&peer, PEERS_DIR, 1);
    int previous_available_peer_count = available_peer_count;


    // Create all FIFOs and save the paths to the FIFO files
    for (int peer_idx = 0; peer_idx < available_peer_count; peer_idx++) {
        char *outgoing_path = peer.outgoing_fifo_file_paths[peer_idx];
        char *incoming_path = peer.incoming_fifo_file_paths[peer_idx];

        snprintf(outgoing_path, FIFO_PATH_LENGTH, "%s%d_2_%d",
                FIFOS_DIR, (int)peer.pid,
                (int)peer.available_peer_pids[peer_idx]);

        if (mkfifo(outgoing_path, 0777) == -1 && errno != EEXIST) {
            perror("mkfifo outgoing FIFO");
            return 5;
        }

        snprintf(incoming_path, FIFO_PATH_LENGTH, "%s%d_2_%d",
                FIFOS_DIR, (int)peer.available_peer_pids[peer_idx],
                (int)peer.pid);

        if (mkfifo(incoming_path, 0777) == -1 && errno != EEXIST) {
            perror("mkfifo incoming FIFO");
            return 6;
        }
    }


    // Read the incoming FIFO fds non blocking so the programm
    // doesn't block because all peers only open and there is no writing FIFO end
    for (int peer_idx = 0; peer_idx < available_peer_count; peer_idx++) {
        int fd = open(peer.incoming_fifo_file_paths[peer_idx],
                    O_RDONLY | O_NONBLOCK);

        if (fd == -1) {
            perror("open incoming FIFO");
            return 7;
        }

        peer.incoming_fifo_fds[peer_idx] = fd;

        printf("Incoming FIFO opened: %s\n",
            peer.incoming_fifo_file_paths[peer_idx]);
    }


    // open the outgoing FIFOs writing
    for (int peer_idx = 0; peer_idx < available_peer_count; peer_idx++) {
        int fd = open(peer.outgoing_fifo_file_paths[peer_idx], O_WRONLY);

        if (fd == -1) {
            perror("open outgoing FIFO");
            return 8;
        }

        peer.outgoing_fifo_fds[peer_idx] = fd;
        (peer.connected_peer_pids)[peer_idx] = (peer.available_peer_pids)[peer_idx];

        printf("Outgoing FIFO opened: %s\n",
            peer.outgoing_fifo_file_paths[peer_idx]);
    }

    // synchronization can be done later
    if (peer.outgoing_fifo_fds[0] != -1) {
        initialize_chats_for_new_peer(&peer);
    }

    char buffer[BUFFER_SIZE]; // for the request content

    // JSON msg ids
    uint32_t response_json_msg_id = 0;
    uint32_t request_msg_id = 0;

    // time stamps for regularly timed prints (init to 0 so first time printed)
    int64_t last_message_sent_time = 0;
    int64_t last_peer_list = 0;
    int64_t last_available_peer_count_time = 0;
    int64_t last_no_peer_time = 0;
    int64_t last_rearange_time = 0;

    // counter for requests
    uint64_t received_request_counter = 0;

    int sync_ack_pending = -1; // -1 means no sync ack started, 1 means sync_ack running, 0 means sync_ack_finished
    int pending_sync_ack_peer_indexes[MAX_PEER_COUNT];
    for (int i = 0; i < MAX_PEER_COUNT; i++) {
        pending_sync_ack_peer_indexes[i] = -1;
    }

    // workflow management
    WorkflowState workflow_state = WORKFLOW_GET_CHAT;

    uint8_t workflow_chat_id = 0;
    size_t workflow_content_length = 0;

    int request_pending = 0;

    // now the creation of the peer's server is completed

    int break_flag = 0;

    WorkflowConfig wf_cfg;
    wf_cfg.wf_state = workflow_state;
    wf_cfg.wf_chat_id = workflow_chat_id;
    wf_cfg.wf_content_length = workflow_content_length;

    while (!interrupted)
    {
        if (millis() - last_rearange_time > REARANGE_PRINT_RATE) {
            reorder_peer_lists_and_print(&peer, &last_rearange_time);
        } else {
            reorder_peer_lists(&peer);
        }
        
        sync_ack_pending = -1;
        for (int i = 0; i < MAX_PEER_COUNT; i++) {
            peer.contacted_peer_pids[i] = -1;
        }

        if (peer.incoming_fifo_fds[0] < 0) {
            int print = (millis() - last_no_peer_time >= NO_PEER_AVAILABLE_PRINT_RATE);
            if (print) {
                last_no_peer_time = millis();
            }
            int wait_for_peer_return_code = wait_for_peer(&peer, print);
            if (wait_for_peer_return_code == 8) {
                return 8;
            } else if (wait_for_peer_return_code == 4) {
                cleanup_peer(&peer, pid);
                return 4;
            }
        }
        

        int print = (millis() - last_peer_list > PEER_LISTS_PRINT_RATE);
        if (connect_more_peers(&peer, print) != 0) {
            continue;
        }

        // check the available peer count after the tried connection
        available_peer_count = register_fifo_files(&peer, PEERS_DIR, 0);

        if (millis() - last_rearange_time > REARANGE_PRINT_RATE) {
            reorder_peer_lists_and_print(&peer, &last_rearange_time);
        } else {
            reorder_peer_lists(&peer);
        }

        if (print) {
            last_peer_list = millis();
        }
        
        // choose random peer to send a request to
        if (millis() - last_available_peer_count_time > AVAILABLE_PEER_COUNT_PRINT_RATE) {
            last_available_peer_count_time = millis();
            printf("Available outgoing peers: %d\n", available_peer_count);
        }

        if (available_peer_count == 0 && previous_available_peer_count != 0) {
            previous_available_peer_count = 0;
            for (int peer_fd_idx = 0; peer_fd_idx < MAX_PEER_COUNT; peer_fd_idx++) {
                peer.incoming_fifo_fds[peer_fd_idx] = -1;
                peer.outgoing_fifo_fds[peer_fd_idx] = -1;
                peer.connected_peer_pids[peer_fd_idx] = -1;
                peer.available_peer_pids[peer_fd_idx] = -1;
                strcpy(peer.incoming_fifo_file_paths[peer_fd_idx], "");
                strcpy(peer.outgoing_fifo_file_paths[peer_fd_idx], "");
            }
        }
        

        if (available_peer_count > 0 && !request_pending && millis() - last_message_sent_time > MSG_SENDING_RATE) {

            // Select a random available peer and create a random request:
            SyncAckRequest sync_req = create_random_sync_request(peer, request_msg_id, 
                available_peer_count);

            // filling out the request to change the own chats variable and send the request to it's own
            OwnRequest own_request;
            own_request.chat_id = sync_req.chat_id; own_request.msg_id = sync_req.msg_id;
            own_request.operation = operation_from_workflow(wf_cfg);
            char *content;
            if (strcmp(own_request.operation, "add_msg") == 0 || strcmp(own_request.operation, "edit_msg") == 0) {
                content = generate_random_content();
            } else {
                content = "";
            }
            own_request.msg = content;
            
            // NOW: CALL GET METHODS IF OPERATION IS A GET OP
            // check first if get and then call if it is a get method
            // get: means print to console here
            printf("\n\n===================\nChats before own update:\n");
            for (int chat_idx = 20; chat_idx < MAX_CHAT_COUNT; chat_idx++) {
                print_chat(peer.chats, chat_idx);
            }
            printf("===================\n\n");


            if (strcmp(own_request.operation, "get_chats") == 0) {
                printf("======= GET CHATS =======\n");
                for (int chat_idx = 0; chat_idx < 10; chat_idx++) {
                    print_chat(peer.chats, chat_idx);
                }
                printf("=========================\n");
            } else if (strcmp(own_request.operation, "get_chat") == 0) {
                printf("======= GET CHAT =======\n");
                print_chat(peer.chats, own_request.chat_id);
                printf("========================\n");
            } else if (strcmp(own_request.operation, "get_msg") == 0) {
                printf("======= GET MSG =======\n");
                printf("%s\n", get_message_from_variable(peer.chats, own_request.chat_id, own_request.msg_id));
                printf("=======================\n");
            }
            // else it has to be an updating method:
            // manipulating the own chats var with my own_req and print the resulting chats
            else if (strcmp(own_request.operation, "add_msg") == 0 || strcmp(own_request.operation, "edit_msg") == 0) {
                if (update_own_chats(peer.chats, own_request) != 0) {
                    fprintf(stderr, "Cancelling sending the message because own update was unsuccessful...\n");
                    continue;
                }
                print_chat(peer.chats, own_request.chat_id);
                printf("\n\n===================\nChats after own update:\n");
                for (int chat_idx = 20; chat_idx < MAX_CHAT_COUNT; chat_idx++) {
                    print_chat(peer.chats, chat_idx);
                }
                printf("===================\n\n");
            }

            else  {
                fprintf(stderr, "Invalid command for an update!\n");
            }

            printf("AFTER OWN UPDATE:\n");
            printf("\n\n===================\nChats after pdu processing:\n");
            for (int chat_idx = 20; chat_idx < MAX_CHAT_COUNT; chat_idx++) {
                print_chat(peer.chats, chat_idx);
            }
            printf("===================\n\n");

            // Generate the request according to the current workflow state:
            wf_cfg.wf_msg_id = own_request.msg_id;
            wf_cfg.wf_chat_id = own_request.chat_id;
            generate_str_sync_request(&sync_req, wf_cfg, own_request.msg);
            update_workflow(&wf_cfg);

            // send the message
            int non_neg_fd_idxs[MAX_PEER_COUNT];
            int non_neg_fd_count = detect_gaps_in_fds(peer.outgoing_fifo_fds, non_neg_fd_idxs, 0);

            for (int j = 0; j < non_neg_fd_count; j++) {
                int fifo_idx = non_neg_fd_idxs[j];

                sync_req.fifo_index = fifo_idx;
                sync_req.fifo_fd = peer.outgoing_fifo_fds[fifo_idx];

                printf("Before sending sync\n");
                printf("fifo_idx = %d\n", fifo_idx);
                printf("sync_req.fifo_index = %d\n", sync_req.fifo_index);
                printf("sync_req.fifo_fd = %d\n", sync_req.fifo_fd);
                printf("peer.outgoing_fifo_fds[%d] = %d\n",
                    fifo_idx, peer.outgoing_fifo_fds[fifo_idx]);

                if (send_sync(peer, sync_req, &sync_ack_pending,
                        pending_sync_ack_peer_indexes) < 0) {
                            printf("errno = %d\n", errno);
                            if (errno == EPIPE) {
                                int old_pid_idx = sync_req.fifo_index;
                                pid_t dest_pid = (peer.connected_peer_pids)[sync_req.fifo_index];
                                int dest_pid_available = is_dest_pid_available(
                                    PEERS_DIR, dest_pid
                                );
                                printf("dest_pid_available = %d\n", dest_pid_available);
                                if (!dest_pid_available) {
                                    restructure_peer(&peer, old_pid_idx, &last_rearange_time);
                                    fprintf(stderr, 
                                        "WARNING: Message could not be sent because the pid doesn't exist anymore!"
                                    );
                                    break;
                                } else if (dest_pid_available == 1) {
                                    continue;
                                } else {
                                    break_flag = 1;
                                    break;
                                }
                            } else {
                                cleanup_peer(&peer, pid);
                                return 2;
                            }
                } else {
                    last_message_sent_time = millis();
                    peer.contacted_peer_pids[fifo_idx] = peer.connected_peer_pids[fifo_idx];
                }
            }

            free(sync_req.request_str);
        }

        if (break_flag) {
            break;
        }

        // receive requests
        // polling the receiving fds bcause reading FIFOs
        // in default way is blocking and polling can comfortably
        // read multiple FIFOs one after the other

        // poll initialization for all available peers
        struct pollfd poll_fds[MAX_PEER_COUNT];
        int peer_indices[MAX_PEER_COUNT];
        nfds_t num_fds = 0;

        for (int i = 0; i < MAX_PEER_COUNT; i++) {
            if (peer.incoming_fifo_fds[i] < 0) {
                continue;
            }

            poll_fds[num_fds].fd = peer.incoming_fifo_fds[i];
            poll_fds[num_fds].events = POLLIN;
            poll_fds[num_fds].revents = 0;

            peer_indices[num_fds] = i;
            num_fds++;
        }
        int64_t recv_time;

        int ret = poll(poll_fds, num_fds, 1);

        if (ret < 0) {
            if (errno != EINTR) {
                fprintf(stderr, "Error occured during polling the incoming FIFOs\n");
            }
        } else if (ret > 0) {
            for (nfds_t j = 0; j < num_fds; j++) {
                if (poll_fds[j].revents & POLLIN) {
                    int peer_idx = peer_indices[j];

                    ssize_t n = read(
                        poll_fds[j].fd,
                        buffer,
                        sizeof(buffer) - 1
                    );

                    if (n < 0) {
                        if (errno != EAGAIN &&
                            errno != EWOULDBLOCK &&
                            errno != EINTR) {
                            fprintf(stderr, "Error occured during processing the read message\n");
                        }
                        continue;
                    }
                    if (n == 0) {
                        // No data read
                        // maybe no writing end opened
                        continue;
                    }
                    if (n > 0) { // actually always fulfilled as the loop wasn't restarted with continue
                        recv_time = millis();
                        buffer[n] = '\0';
                        printf("Data package NO. %ld received from peer %i at time %ld: %s \n\n", received_request_counter, peer_idx, millis(), buffer);
                        
                        received_request_counter++;

                        // parse JSON
                        // json: buffer = "{"header": {"msg_id": ..., "time_stamp": ...}, "payload": {"operands": ..., "operation": ...}};
                        // create pdu and fill it step-by-step from buffer
                        JsonPdu pdu;
                        pdu.json_msg = cJSON_Parse(buffer); // parse JSON msg

                        
                        if (test_json_msg(&pdu, &response_json_msg_id, peer.outgoing_fifo_fds[peer_idx]) < 0) {
                            cJSON_Delete(pdu.json_msg);
                            continue;
                        }
                        if (test_header(&pdu, &response_json_msg_id, peer.outgoing_fifo_fds[peer_idx]) < 0) {
                            cJSON_Delete(pdu.json_msg);
                            continue;
                        }
                        if (test_json_msg_id(&pdu, &response_json_msg_id, peer.outgoing_fifo_fds[peer_idx]) < 0) {
                            cJSON_Delete(pdu.json_msg);
                            continue;
                        }
                        if (test_time_stamp(&pdu, &response_json_msg_id, peer.outgoing_fifo_fds[peer_idx]) < 0) {
                            cJSON_Delete(pdu.json_msg);
                            continue;
                        }
                        if (test_payload(&pdu, &response_json_msg_id, peer.outgoing_fifo_fds[peer_idx]) < 0) {
                            cJSON_Delete(pdu.json_msg);
                            continue;
                        }
                        
                        int init_sync_sending_peer_pid = is_init_sync(pdu);
                        if (init_sync_sending_peer_pid > 0) {
                            printf("init_sync_sending_peer_pid = %d\n", init_sync_sending_peer_pid);
                            printf("\n\n\nINIT_SYNC RECEIVED!!!\n\n\n\n");
                            printf("Received init sync request: init_ack_sending_peer_pid = %d\n", init_sync_sending_peer_pid);
                            InitSyncAckRequest init_ack_resp = create_init_ack_resp(peer, init_sync_sending_peer_pid, peer.chats);
                            
                            if (send_init_ack(peer, init_ack_resp) < 0) {
                                printf("errno = %d\n", errno);
                                if (errno == EPIPE) {
                                    int old_pid_idx = init_ack_resp.fifo_index;
                                    pid_t dest_pid = (peer.connected_peer_pids)[init_ack_resp.fifo_index];
                                    int dest_pid_available = is_dest_pid_available(
                                        PEERS_DIR, dest_pid
                                    );
                                    printf("dest_pid_available = %d\n", dest_pid_available);
                                    if (!dest_pid_available) {
                                        restructure_peer(&peer, old_pid_idx, &last_rearange_time);
                                        fprintf(stderr, 
                                            "WARNING: Message could not be sent because the pid doesn't exist anymore!"
                                        );
                                        break;
                                    } else if (dest_pid_available == 1) {
                                        continue;
                                    } else {
                                        break_flag = 1;
                                        break;
                                    }
                                } else {
                                    cleanup_peer(&peer, pid);
                                    return 2;
                                }
                            }

                            free(init_ack_resp.request_str);
                            cJSON_Delete(pdu.json_msg);

                            if (break_flag) {
                                break;
                            }
                            continue;
                        }
                        int ack_sending_peer_pid = is_ack(pdu);
                        printf("ack_sending_peer_pid = %d\n", ack_sending_peer_pid);
                        if (ack_sending_peer_pid > 0) {

                            sync_ack_pending = 0; // if not later in this part changed it means all peers are synchronized

                            // get the peer_fd_idx from the server port to set it as successfully synchronized
                            for (int peer_idx = 0; peer_idx < MAX_PEER_COUNT; peer_idx++) {
                                if (ack_sending_peer_pid == peer.connected_peer_pids[peer_idx]) {
                                    peer.successfully_synchronized_peer_pids[peer_idx] = 1;
                                    pending_sync_ack_peer_indexes[peer_idx] = 0;
                                    break;
                                }
                            }

                            // now if the first non_neg_fd_count entries in peer.successfully_synchronized_peer_pids
                            // are 1 it means that all peers that were requested to sync updated successfully
                            int non_neg_fd_idxs[MAX_PEER_COUNT];
                            int non_neg_fd_count = detect_gaps_in_fds(peer.outgoing_fifo_fds, non_neg_fd_idxs, 0);
                            for (int peer_idx = 0; peer_idx < non_neg_fd_count; peer_idx++) {
                                if (peer.successfully_synchronized_peer_pids[peer_idx] != 1) {
                                    sync_ack_pending = 1;
                                    break;
                                }
                            }

                            cJSON_Delete(pdu.json_msg);
                            continue;
                        }
                        printf("sync_ack_pending = %d\n", sync_ack_pending);
                        if (sync_ack_pending == 0) {
                            break;
                        }

                        if (test_operation(&pdu, &response_json_msg_id, peer.outgoing_fifo_fds[peer_idx]) < 0) {
                            cJSON_Delete(pdu.json_msg);
                            continue;
                        }
                        // save the operation:
                        char* operation_value = cJSON_GetStringValue(pdu.operation);
                        if (test_chat_id(&pdu, &response_json_msg_id, peer.outgoing_fifo_fds[peer_idx]) < 0) {
                            cJSON_Delete(pdu.json_msg);
                            continue;
                        }
                        uint8_t chat_id = cJSON_GetNumberValue(pdu.chat_id);

                        

                        if (test_msg_id(&pdu, &response_json_msg_id, peer.outgoing_fifo_fds[peer_idx], operation_value) < 0) {
                            cJSON_Delete(pdu.json_msg);
                            continue;
                        }
                        uint8_t msg_id = cJSON_GetNumberValue(pdu.msg_id);

                        uint8_t new_msg_id;
                        
                        int success = 0;
                        int sync_sending_peer_pid = is_sync(pdu);
                        printf("sync_sending_peer_pid = %d\n", sync_sending_peer_pid);
                        if (sync_sending_peer_pid != pid) {
                            success = test_content_and_process_sync(&pdu, &response_json_msg_id, peer.incoming_fifo_fds[peer_idx], operation_value,
                                MAX_PEER_COUNT, peer.chats, chat_id, msg_id, &new_msg_id);
                        }

                        if (sync_sending_peer_pid > 0) {
                            // processing the pdu:
                            printf("\n\n===================\nChats after pdu processing:\n");
                            for (int chat_idx = 20; chat_idx < MAX_CHAT_COUNT; chat_idx++) {
                                print_chat(peer.chats, chat_idx);
                            }
                            printf("===================\n\n");
                            
                            if (create_and_send_ack_req(peer, peer_idx, request_msg_id, success) < 0) {
                                printf("errno = %d\n", errno);
                                if (errno == EPIPE) {
                                    int old_pid_idx = peer_idx;
                                    pid_t dest_pid = (peer.connected_peer_pids)[peer_idx];
                                    int dest_pid_available = is_dest_pid_available(
                                        PEERS_DIR, dest_pid
                                    );
                                    printf("dest_pid_available = %d\n", dest_pid_available);
                                    if (!dest_pid_available) {
                                        restructure_peer(&peer, old_pid_idx, &last_rearange_time);
                                        fprintf(stderr, 
                                            "WARNING: Message could not be sent because the pid doesn't exist anymore!"
                                        );
                                        break;
                                    } else if (dest_pid_available == 1) {
                                        continue;
                                    } else {
                                        break_flag = 1;
                                        break;
                                    }
                                } else {
                                    cleanup_peer(&peer, pid);
                                    return 2;
                                }
                            }

                            cJSON_Delete(pdu.json_msg);
                            if (break_flag) {
                                break;
                            }
                            continue;
                        }
                        
                        
                        // now create the json response
                        cJSON* response = create_json_response(&response_json_msg_id, chat_id);
                        cJSON* response_payload = cJSON_GetObjectItem(response, "payload");
                        
                        if (strcmp(operation_value, "get_msg") == 0) {
                            int error_code = create_get_msg_response(response_payload, chat_id, msg_id, peer.chats);
                            if (error_code == -2) {
                                fprintf(stderr, "No valid chat id!\n\n");
                                cJSON_Delete(response);
                                cJSON_Delete(pdu.json_msg);
                                continue;
                            } else if (error_code == -1) {
                                fprintf(stderr, "No valid msg id!\n\n");
                                cJSON_Delete(response);
                                cJSON_Delete(pdu.json_msg);
                                continue;
                            }
                        } else if (strcmp(operation_value, "get_chat") == 0) {
                            int error_code = create_get_chat_response(response_payload, chat_id, peer.chats, MAX_PEER_COUNT);
                            if (error_code == -1) {
                                fprintf(stderr, "No valid chat id!\n\n");
                                cJSON_Delete(response);
                                cJSON_Delete(pdu.json_msg);
                                continue;
                            }
                        } else if (strcmp(operation_value, "add_msg") == 0) {
                            int error_code = create_add_msg_response(response_payload, chat_id, new_msg_id);
                            if (error_code == -2) {
                                fprintf(stderr, "No valid chat id!\n\n");
                                cJSON_Delete(response);
                                cJSON_Delete(pdu.json_msg);
                                continue;
                            } else if (error_code == -1) {
                                fprintf(stderr, "No valid msg id!\n\n");
                                cJSON_Delete(response);
                                cJSON_Delete(pdu.json_msg);
                                continue;
                            }
                        } else if (strcmp(operation_value, "edit_msg") == 0) {
                            int error_code = create_edit_msg_response(response_payload, chat_id, msg_id);
                            if (error_code == -2) {
                                fprintf(stderr, "No valid chat id!\n\n");
                                cJSON_Delete(response);
                                cJSON_Delete(pdu.json_msg);
                                continue;
                            } else if (error_code == -1) {
                                fprintf(stderr, "No valid msg id!\n\n");
                                cJSON_Delete(response);
                                cJSON_Delete(pdu.json_msg);
                                continue;
                            }
                        } else {
                            printf("NO LEGAL OPERATION!\n");
                            fprintf(stderr, "NO LEGAL OPERATION!\n");
                            cJSON_Delete(response);
                            cJSON_Delete(pdu.json_msg);
                            continue;
                        }
                        
                        cJSON_AddNumberToObject(response_payload, "success", 0);

                        // convert response json object to string
                        char *response_string = cJSON_PrintUnformatted(response);

                        if (response_string == NULL) {
                            printf("Failed to create response string\n");
                        } else {
                            send_json_response(response_string, peer.outgoing_fifo_fds[peer_idx]);
                        }

                        int64_t time_stamp = cJSON_GetNumberValue(pdu.time_stamp);
                        printf("time_stamp = %ld\n", (long) time_stamp);
                        printf("It took %ld ms from receiving to sending the latest data.\n\n", millis() - recv_time);


                        free(response_string);

                        cJSON_Delete(pdu.json_msg);   // just delete msg to delete the whole json tree
                        cJSON_Delete(response);
                    }
                }
            }
        }

    }

    cleanup_peer(&peer, pid);

    return 0;
}
