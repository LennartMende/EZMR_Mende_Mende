#define _POSIX_C_SOURCE 199309L

#include "peer_server_utils.h"

#include <time.h>

#include "constants.h"
#include "peer_utils.h"



int utils_sleep_ms(int ms) {
    struct timespec ts_req, ts_rem;
    ts_req.tv_sec = ms / MS_PER_S;
    ts_req.tv_nsec = (ms % MS_PER_S) * NS_PER_MS; // ms % 1000 -> remaining "real" ms -> * 1000000 for nano secs

    return (nanosleep(&ts_req, &ts_rem));
}

// returns 0 if successful and 8 if an error occured
int wait_for_peer(Peer* peer, int print) {
    int64_t start_time = 0;
    int available_peer_count = register_fifo_files(peer, PEERS_DIR, print);
    if (available_peer_count < 0) {
        fprintf(stderr, "register_fifo_files() failed and returned %d\n", available_peer_count);
        return 8;
    }
    while (available_peer_count == 0) {
        available_peer_count = register_fifo_files(peer, PEERS_DIR, print);
        if (print && millis() - start_time > 1000) {
            printf("Available peers = %d\n", available_peer_count);
        }
        if (available_peer_count > 0) {
            break;
        }
        if (available_peer_count < 0) {
            fprintf(stderr, "register_fifo_files() failed and returned %d\n", available_peer_count);
            return 8;
        }
        if (print && millis() - start_time > 1000) {
            printf("Waiting for a peer...\n");
            start_time = millis();
        }
        if (utils_sleep_ms(1) != 0) {
            printf("Sleep was interrupted!\n");
            return 4;
        }
    }

    // Create all FIFOs and save the paths to the FIFO files
    for (int peer_idx = 0; peer_idx < available_peer_count; peer_idx++) {
        char *outgoing_path = peer->outgoing_fifo_file_paths[peer_idx];
        char *incoming_path = peer->incoming_fifo_file_paths[peer_idx];

        snprintf(outgoing_path, FIFO_PATH_LENGTH, "%s%d_2_%d",
                FIFOS_DIR, (int)peer->pid,
                (int)peer->available_peer_pids[peer_idx]);

        if (mkfifo(outgoing_path, 0777) == -1 && errno != EEXIST) {
            perror("mkfifo outgoing FIFO");
            return 5;
        }

        snprintf(incoming_path, FIFO_PATH_LENGTH, "%s%d_2_%d",
                FIFOS_DIR, (int)peer->available_peer_pids[peer_idx],
                (int)peer->pid);

        if (mkfifo(incoming_path, 0777) == -1 && errno != EEXIST) {
            perror("mkfifo incoming FIFO");
            return 6;
        }
    }


    // Read the incoming FIFO fds non blocking so the programm
    // doesn't block because all peers only open and there is no writing FIFO end
    for (int peer_idx = 0; peer_idx < available_peer_count; peer_idx++) {
        int fd = open((peer->incoming_fifo_file_paths)[peer_idx],
                    O_RDONLY | O_NONBLOCK);

        if (fd == -1) {
            perror("open incoming FIFO");
            return 7;
        }

        (peer->incoming_fifo_fds)[peer_idx] = fd;

        printf("Incoming FIFO opened: %s\n",
            (peer->incoming_fifo_file_paths)[peer_idx]);
    }


    // open the outgoing FIFOs writing
    for (int peer_idx = 0; peer_idx < available_peer_count; peer_idx++) {
        int fd = open((peer->outgoing_fifo_file_paths)[peer_idx], O_WRONLY);

        if (fd == -1) {
            perror("open outgoing FIFO");
            return 8;
        }

        (peer->outgoing_fifo_fds)[peer_idx] = fd;
        (peer->connected_peer_pids)[peer_idx] = (peer->available_peer_pids)[peer_idx];

        printf("Outgoing FIFO opened: %s\n",
            (peer->outgoing_fifo_file_paths)[peer_idx]);
    }

    return 0;
}


// returns 0 if no error occured
int connect_more_peers(Peer *peer, int print) {
    pid_t old_available_peer_pids[MAX_PEER_COUNT];
    for (int i = 0; i < MAX_PEER_COUNT; i++) {
        old_available_peer_pids[i] = peer->available_peer_pids[i];
    }

    int available_peers = register_fifo_files(peer, PEERS_DIR, print);
    if (available_peers < 0) {
        fprintf(stderr, "register_fifo_files() failed and returned %d\n", available_peers);
        return 8;
    }

    // compare the old available_peer_pids to the new ones to find 
    // possible differences for updates
    for (int peer_pid_idx = 0; peer_pid_idx < available_peers; peer_pid_idx++) {
        if (print) {
            printf("\n\n\n\nDEBUG!!!\n");
        }
        if ((peer->available_peer_pids)[peer_pid_idx] != old_available_peer_pids[peer_pid_idx]) { // meaning at same position no more same peer_pid
            int new_pid_found_in_old_pids = 0;
            for (int old_peer_pid_idx = 0; old_peer_pid_idx < MAX_PEER_COUNT; old_peer_pid_idx++) {
                if ((peer->available_peer_pids)[peer_pid_idx] == old_available_peer_pids[old_peer_pid_idx]) {
                    new_pid_found_in_old_pids = 1;
                    break;
                }
            }
            printf("new_pid_found_in_old_pids = %d\n", new_pid_found_in_old_pids);
            if (!new_pid_found_in_old_pids) {
                // the indices in available_peer_pids may be different than those in the connection arrays
                // using peer_pid_idx for the connection arrays has overwritten an existing connection in previous tests
                pid_t new_peer_pid = peer->available_peer_pids[peer_pid_idx];
                int already_connected = 0;

                for (int i = 0; i < MAX_PEER_COUNT; i++) {
                    if (peer->connected_peer_pids[i] == new_peer_pid) {
                        already_connected = 1;
                        break;
                    }
                }
                if (already_connected) {
                    continue;
                }

                int connection_idx = -1;
                for (int i = 0; i < MAX_PEER_COUNT; i++) {
                    if (peer->connected_peer_pids[i] == -1) {
                        connection_idx = i;
                        break;
                    }
                }

                if (connection_idx == -1) {
                    fprintf(stderr, "No free connection slot available!\n");
                    return 9;
                }

                
                char p2p_fifo_path[256];
                // outgoing FIFO
                snprintf(p2p_fifo_path, sizeof(p2p_fifo_path), "%s%d_2_%d", FIFOS_DIR, peer->pid, peer->available_peer_pids[peer_pid_idx]);
                if (mkfifo(p2p_fifo_path, 0777) == -1) {
                    if (errno != EEXIST) { // error is other that that the FIFO file already exists
                        fprintf(stderr, "FIFO file for the request could not be created!\n");
                        return 5;
                    }
                }
                strcpy((peer->outgoing_fifo_file_paths)[connection_idx], p2p_fifo_path); // (peer->outgoing_fifo_fd)[peer_pid_idx]
                int outgoing_fifo_fd = open(p2p_fifo_path, O_WRONLY);
                if (outgoing_fifo_fd == -1) {
                    fprintf(stderr, "FIFO for incoming data could not be opened\n");
                    return 6;
                } else {
                    (peer->outgoing_fifo_fds)[connection_idx] = outgoing_fifo_fd;
                    if (print) {
                        printf("FIFO file %s for incoming data opened\n", p2p_fifo_path);
                    }
                }
                

                // incoming FIFO
                snprintf(p2p_fifo_path, sizeof(p2p_fifo_path), "%s%d_2_%d", FIFOS_DIR, peer->available_peer_pids[peer_pid_idx], peer->pid);
                if (mkfifo(p2p_fifo_path, 0777) == -1) {
                    if (errno != EEXIST) { // error is other that that the FIFO file already exists
                        fprintf(stderr, "FIFO file for incoming data could not be created!\n");
                        return 5;
                    }
                }
                strcpy((peer->incoming_fifo_file_paths)[connection_idx], p2p_fifo_path);
                int incoming_fifo_fd = open(p2p_fifo_path, O_RDONLY | O_NONBLOCK);
                if (incoming_fifo_fd == -1) {
                    fprintf(stderr, "FIFO for incoming data could not be opened\n");
                    return 6;
                } else {
                    (peer->incoming_fifo_fds)[connection_idx] = incoming_fifo_fd;
                    (peer->connected_peer_pids)[connection_idx] = (peer->available_peer_pids)[peer_pid_idx];
                    if (print) {
                        printf("FIFO file %s for outgoing data opened\n", p2p_fifo_path);
                    }
                }
            }
        }
    }
    return 0;
}


// void connect_more_peers(Peer *peer, int print) {
//     for (int i = 1; i < MAX_PEER_COUNT; i++) {
//         if (peer->incoming_peer_fds[i] < 0) {
//             peer->incoming_peer_fds[i] = accept(peer->server_fd, NULL, NULL);
//             if (print) {
//                 if (peer->incoming_peer_fds[i] >= 0) {
//                     printf("----------\nPeer connected!\n----------\n");
//                 }
//                 printf("Oveview over connected peers:\n");
//                 printf("peer->connected_peer_server_ports[0:10] = [");
//                 for (int j = 0; j < 10; j++) {
//                     printf("%i ", peer->connected_peer_server_ports[j]);
//                 }
//                 printf("]\n");
//                 printf("peer->incoming_peer_fds[0:10] = [");
//                 for (int j = 0; j < 10; j++) {
//                     printf("%i ", peer->incoming_peer_fds[j]);
//                 }
//                 printf("]\n");
//                 printf("peer->outgoing_peer_fds[0:10] = [");
//                 for (int j = 0; j < 10; j++) {
//                     printf("%i ", peer->outgoing_peer_fds[j]);
//                 }
//                 printf("]\n");
//             }
//         }
    

//         if (peer->incoming_peer_fds[i] < 0)
//         {
//             if (errno == EAGAIN || errno == EWOULDBLOCK)
//             {
//                 int number_of_peers = 0;
//                 for (int j = 0; j < MAX_PEER_COUNT; j++) {
//                     if (peer->incoming_peer_fds[j] >= 0) {
//                         number_of_peers++;
//                     }
//                 }
//                 if (print) {
//                     printf("Still %i peers available\n", number_of_peers);
//                 }
//                 break;
//             }
//             else
//             {
//                 perror("accept");
//             }
//         }

//         // Auch Peer-Socket nonblocking
//         int flags = fcntl(peer->incoming_peer_fds[i], F_GETFL, 0);
//         if (flags < 0) {
//             perror("fcntl F_GETFL");
//             close(peer->incoming_peer_fds[i]);
//             peer->incoming_peer_fds[i] = -1;
//             continue;
//         }

//         if (fcntl(peer->incoming_peer_fds[i], F_SETFL, flags | O_NONBLOCK) < 0) {
//             perror("fcntl F_SETFL");
//             close(peer->incoming_peer_fds[i]);
//             peer->incoming_peer_fds[i] = -1;
//             continue;
//         }
//     }
// }
