#define _POSIX_C_SOURCE 199309L

#include <time.h>

#include "peer_server_utils.h"

// #define SERVER_IP "127.0.0.1"

// #define MS_PER_S 1000
// #define NS_PER_MS 1000000

// #define MSG_SENDING_RATE 5000
// #define PEER_LISTS_PRINT_RATE 5000
// #define AVAILABLE_PEER_COUNT_PRINT_RATE 1000
// #define NO_DATA_AVAILABLE_PRINT_RATE 1000


int utils_sleep_ms(int ms) {
    struct timespec ts_req, ts_rem;
    ts_req.tv_sec = ms / MS_PER_S;
    ts_req.tv_nsec = (ms % MS_PER_S) * NS_PER_MS; // ms % 1000 -> remaining "real" ms -> * 1000000 for nano secs

    return (nanosleep(&ts_req, &ts_rem));
}

// creates a server of the peer and returns success
int init_peer_server(Peer *peer) {
    int success = 0;
    peer->server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (peer->server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int reuse = 1;
    if (setsockopt(peer->server_fd, SOL_SOCKET, SO_REUSEADDR,
                   &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        close(peer->server_fd);
        exit(EXIT_FAILURE);
    }

    memset(&(peer->addr), 0, sizeof(peer->addr));
    peer->addr.sin_family = AF_INET;
    peer->addr.sin_port = htons(peer->server_port);
    peer->addr.sin_addr.s_addr = INADDR_ANY;

    if (inet_pton(AF_INET, SERVER_IP,
              &peer->addr.sin_addr) != 1) {
        perror("inet_pton");
        close(peer->server_fd);
        exit(EXIT_FAILURE);
    }

    if (bind(peer->server_fd,
             (struct sockaddr *)&(peer->addr),
             sizeof(peer->addr)) < 0)
    {
        perror("bind");
        close(peer->server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(peer->server_fd, 5) < 0) {
        perror("listen");
        close(peer->server_fd);
        exit(EXIT_FAILURE);
    }

    // Socket auf NONBLOCKING setzen
    int flags = fcntl(peer->server_fd, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl F_GETFL");
        close(peer->server_fd);
        exit(EXIT_FAILURE);
    }

    if (fcntl(peer->server_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl F_SETFL");
        close(peer->server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Peer listening on %s:%d initialized. Discovery started\n\n", peer->server_ip, peer->server_port);

    return success;
}


// waits for the first peer to connect
// returns 0 if successful and -1 if an error occured
int wait_for_peer(Peer *peer, int print) {
    int success = 0;

    peer->incoming_peer_fds[0] = accept(peer->server_fd, NULL, NULL);

    if (peer->incoming_peer_fds[0] < 0)
    {
        if ((errno == EAGAIN || errno == EWOULDBLOCK) && print)
        {
            printf("No peer available in wait for peer loop\n");
        }
        else
        {
            if (print) {
                perror("accept");
            }
        }

        success = -1;
    } else {
        printf("Peer connected\n\n");

        // Auch Peer-Socket nonblocking
        int flags = fcntl(peer->incoming_peer_fds[0], F_GETFL, 0);
        if (flags < 0) { // error while retrieving flags
            perror("fcntl F_GETFL");
            close(peer->incoming_peer_fds[0]);
            peer->incoming_peer_fds[0] = -1;
            success = -1;
        }

        if (success == 0) {
            if (fcntl(peer->incoming_peer_fds[0], F_SETFL, flags | O_NONBLOCK) < 0) { // error while setting flags
                perror("fcntl F_SETFL");
                close(peer->incoming_peer_fds[0]);
                peer->incoming_peer_fds[0] = -1;
                success = -1;
            }
        }
    }

    return success;
}


void connect_more_peers(Peer *peer, int print) {
    for (int i = 1; i < MAX_PEER_COUNT; i++) {
        if (peer->incoming_peer_fds[i] < 0) {
            peer->incoming_peer_fds[i] = accept(peer->server_fd, NULL, NULL);
            if (print) {
                if (peer->incoming_peer_fds[i] >= 0) {
                    printf("----------\nPeer connected!\n----------\n");
                }
                printf("Overview over connected peers:\n");
                printf("peer->connected_peer_server_ports[0:10] = [");
                for (int j = 0; j < 10; j++) {
                    printf("%i ", peer->connected_peer_server_ports[j]);
                }
                printf("]\n");
                printf("peer->incoming_peer_fds[0:10] = [");
                for (int j = 0; j < 10; j++) {
                    printf("%i ", peer->incoming_peer_fds[j]);
                }
                printf("]\n");
                printf("peer->outgoing_peer_fds[0:10] = [");
                for (int j = 0; j < 10; j++) {
                    printf("%i ", peer->outgoing_peer_fds[j]);
                }
                printf("]\n");
            }
        }
    

        if (peer->incoming_peer_fds[i] < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                int number_of_peers = 0;
                for (int j = 0; j < MAX_PEER_COUNT; j++) {
                    if (peer->incoming_peer_fds[j] >= 0) {
                        number_of_peers++;
                    }
                }
                if (print) {
                    printf("Still %i peers available\n", number_of_peers);
                }
                break;
            }
            else
            {
                perror("accept");
            }
        }

        // peer socket nonblocking too
        int flags = fcntl(peer->incoming_peer_fds[i], F_GETFL, 0);
        if (flags < 0) {
            perror("fcntl F_GETFL");
            close(peer->incoming_peer_fds[i]);
            peer->incoming_peer_fds[i] = -1;
            continue;
        }

        if (fcntl(peer->incoming_peer_fds[i], F_SETFL, flags | O_NONBLOCK) < 0) {
            perror("fcntl F_SETFL");
            close(peer->incoming_peer_fds[i]);
            peer->incoming_peer_fds[i] = -1;
            continue;
        }
    }
}
