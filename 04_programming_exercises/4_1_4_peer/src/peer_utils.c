#include "peer_utils.h"
#include "peer_client_utils.h"
#include <stdlib.h>
#include <stdarg.h>


int64_t millis(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ((int64_t)ts.tv_sec) * MS_PER_S + ((int64_t)ts.tv_nsec) / NS_PER_MS;
}


//for a peer
Peer create_first_peer(uint16_t peer_server_port) {
    Peer peer;
    peer.server_ip = "127.0.0.1";
    peer.server_port = peer_server_port;
    generate_first_chats(peer.chats);
    if (init_peer_server(&peer) < 0) {
        fprintf(stderr, "Peer server part initialization failed");
        exit(10);
    }
    for (int peer_fd_idx = 0; peer_fd_idx < MAX_PEER_COUNT; peer_fd_idx++) {
        peer.incoming_peer_fds[peer_fd_idx] = -1;
        peer.outgoing_peer_fds[peer_fd_idx] = -1;
        peer.connected_peer_server_ports[peer_fd_idx] = -1;
    }

    return peer;
}


void generate_first_chats(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH]) {
    strcpy(chats[0][0], "Wer ist deutscher Bundeskanzler?");
    strcpy(chats[0][1], "Angelo Merte.");
    strcpy(chats[0][2], "Sicher?");

    strcpy(chats[1][0], "Dann haste hier die Cola");
    strcpy(chats[1][1], "Ach ne, ich kriege...");
    strcpy(chats[1][2], "Was n jetzt schon widder?");
    strcpy(chats[1][3], "Was steht da drauf?!");
    strcpy(chats[1][4], "Light");
    strcpy(chats[1][5], "Und was solltest du mitbringen??? ZERROOO. ZERRROOO");

    strcpy(chats[2][0], "Hat jemand die Aufgabe schon gemacht?");
    strcpy(chats[2][1], "Welche Aufgabe meinst du?");
    strcpy(chats[2][2], "Die mit dem Server.");
    strcpy(chats[2][3], "Noch nicht, ich bin gerade dabei.");

    strcpy(chats[3][0], "Wann treffen wir uns morgen?");
    strcpy(chats[3][1], "Ich würde sagen um 10.");
    strcpy(chats[3][2], "Passt.");
    strcpy(chats[3][3], "Dann bis morgen.");

    strcpy(chats[4][0], "Kannst du mir die Datei schicken?");
    strcpy(chats[4][1], "Welche Datei?");
    strcpy(chats[4][2], "Die aktuelle Version vom Projekt.");
    strcpy(chats[4][3], "Ja, mache ich gleich.");
    strcpy(chats[4][4], "Danke!");

    strcpy(chats[5][0], "Was gibt es heute zu essen?");
    strcpy(chats[5][1], "Keine Ahnung.");
    strcpy(chats[5][2], "Dann müssen wir wohl einkaufen.");
    strcpy(chats[5][3], "Pizza?");
    strcpy(chats[5][4], "Immer Pizza.");

    strcpy(chats[6][0], "Hast du den Film schon gesehen?");
    strcpy(chats[6][1], "Nein, noch nicht.");
    strcpy(chats[6][2], "Der ist tatsächlich ziemlich gut.");
    strcpy(chats[6][3], "Dann schaue ich ihn heute Abend.");

    strcpy(chats[7][0], "Wie war die Klausur?");
    strcpy(chats[7][1], "Die ersten Aufgaben waren einfach.");
    strcpy(chats[7][2], "Und der Rest?");
    strcpy(chats[7][3], "Der Rest war deutlich schwieriger.");
    strcpy(chats[7][4], "Na toll.");

    strcpy(chats[8][0], "Kannst du mir beim Aufbau helfen?");
    strcpy(chats[8][1], "Klar.");
    strcpy(chats[8][2], "Wann hast du Zeit?");
    strcpy(chats[8][3], "Heute Nachmittag.");
    strcpy(chats[8][4], "Perfekt.");

    strcpy(chats[9][0], "Ist der Server noch an?");
    strcpy(chats[9][1], "Ja.");
    strcpy(chats[9][2], "Kannst du eine Anfrage schicken?");
    strcpy(chats[9][3], "Moment.");
    strcpy(chats[9][4], "Hat funktioniert.");

    strcpy(chats[10][0], "Hast du die Ergebnisse bekommen?");
    strcpy(chats[10][1], "Ja, gerade eben.");
    strcpy(chats[10][2], "Und?");
    strcpy(chats[10][3], "Sieht besser aus als erwartet.");

    strcpy(chats[11][0], "Wo bist du?");
    strcpy(chats[11][1], "Noch zu Hause.");
    strcpy(chats[11][2], "Wann kommst du?");
    strcpy(chats[11][3], "In ungefähr zwanzig Minuten.");

    strcpy(chats[12][0], "Die Verbindung funktioniert nicht.");
    strcpy(chats[12][1], "Was sagt der Client?");
    strcpy(chats[12][2], "Connection refused.");
    strcpy(chats[12][3], "Dann läuft der Server wahrscheinlich nicht.");
    strcpy(chats[12][4], "Stimmt, jetzt geht es.");

    strcpy(chats[13][0], "Hast du einen USB-Stick?");
    strcpy(chats[13][1], "Ja, warum?");
    strcpy(chats[13][2], "Ich muss kurz eine Datei übertragen.");
    strcpy(chats[13][3], "Kein Problem.");

    strcpy(chats[14][0], "Was machen wir am Wochenende?");
    strcpy(chats[14][1], "Keine Ahnung.");
    strcpy(chats[14][2], "Wir könnten wandern gehen.");
    strcpy(chats[14][3], "Wenn das Wetter mitspielt.");
    strcpy(chats[14][4], "Dann schauen wir am Samstag.");

    strcpy(chats[15][0], "Hast du den Fehler gefunden?");
    strcpy(chats[15][1], "Noch nicht.");
    strcpy(chats[15][2], "Wo tritt er auf?");
    strcpy(chats[15][3], "Beim Parsen der Nachricht.");
    strcpy(chats[15][4], "Dann würde ich dort anfangen.");

    strcpy(chats[16][0], "Kannst du mir kurz etwas erklären?");
    strcpy(chats[16][1], "Klar, was denn?");
    strcpy(chats[16][2], "Warum brauchen wir hier einen Pointer?");
    strcpy(chats[16][3], "Damit die Funktion den Wert verändern kann.");
    strcpy(chats[16][4], "Ah, jetzt verstehe ich es.");

    strcpy(chats[17][0], "Wie spät ist es?");
    strcpy(chats[17][1], "Fast halb zwölf.");
    strcpy(chats[17][2], "Dann sollten wir langsam los.");
    strcpy(chats[17][3], "Ja, bin gleich fertig.");

    strcpy(chats[18][0], "Hast du noch Kaffee?");
    strcpy(chats[18][1], "Eine Tasse.");
    strcpy(chats[18][2], "Perfekt, die nehme ich.");
    strcpy(chats[18][3], "Dann muss ich wohl neuen machen.");
    strcpy(chats[18][4], "Deal.");

    strcpy(chats[19][0], "Testnachricht.");
    strcpy(chats[19][1], "Noch eine Testnachricht.");
    strcpy(chats[19][2], "Und noch eine.");
    strcpy(chats[19][3], "Das reicht für den Test.");
}


// get numbers of msgs in each chat
void get_msg_count(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH], int msg_counts[MAX_CHAT_COUNT]) {
    for (int chat_idx = 0; chat_idx < MAX_CHAT_COUNT; chat_idx++) {
        msg_counts[chat_idx] = 0;
        for (int msg_idx = 0; msg_idx < MAX_MSG_COUNT; msg_idx++) {
            if (strcmp(chats[chat_idx][msg_idx], "") != 0) {
                msg_counts[chat_idx]++;
            }
        }
    }
}


// get chats as JSON struct
cJSON* get_json_chats(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH], int chat_count) {
    cJSON *json_chats = cJSON_CreateObject();

    // get number of digits of chat_count:
    int chat_count_digits = 0;
    int temp = chat_count;
    if (temp == 0) {
        chat_count_digits = 1;
    }
    if (temp < 0) {
        return NULL;
    }
    while (temp > 0) {
        temp /= 10;
        chat_count_digits++;
    }

    for (int chat_idx = 0; chat_idx < chat_count; chat_idx++) {
        int chat_key_length = 5 + chat_count_digits;
        char chat_key[chat_key_length];
        snprintf(chat_key, chat_key_length, "chat%d", chat_idx);
        cJSON *chat = cJSON_AddArrayToObject(json_chats, chat_key);
        int msg_counts[MAX_CHAT_COUNT] = {0};
        get_msg_count(chats, msg_counts);
        for (int msg_idx = 0; msg_idx < msg_counts[chat_idx]; msg_idx++) {
            cJSON_AddItemToArray(chat, cJSON_CreateString(chats[chat_idx][msg_idx]));
        }
    }

    return json_chats;
}







// chat routines
char (*get_chats_from_variable(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH]))[MAX_MSG_COUNT][MAX_MSG_LENGTH] {
    return chats;
}

char (*get_chat_from_variable(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH], uint8_t chat_id))[MAX_MSG_LENGTH] {
    return chats[chat_id];
}

char* get_message_from_variable(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH], 
    uint8_t chat_id, uint8_t msg_id) {
        return chats[chat_id][msg_id];
}

int add_message_to_variable(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH], uint8_t chat_id, char* msg) {
    int success = 0;
    int msg_counts[MAX_CHAT_COUNT];
    get_msg_count(chats, msg_counts);
    uint8_t chat_length = msg_counts[chat_id];
    strcpy(chats[chat_id][chat_length], msg);
    return success;
}

int edit_message_from_variable(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH], uint8_t chat_id, uint8_t msg_id, char* msg) {
    int success = 0;
    strcpy(chats[chat_id][msg_id], msg);
    return success;
}

void print_chat(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH], uint8_t chat_id) {
    char (*chat)[MAX_MSG_LENGTH] = 
             get_chat_from_variable(chats, chat_id); // pointer to first chat containing MAX_MSG_COUNT msgs
    int chat_is_not_empty = 0;
    for (int msg_idx = 0; msg_idx < MAX_MSG_COUNT; msg_idx++) {
        if (strcmp(chats[chat_id][msg_idx], "") != 0) {
            chat_is_not_empty = 1;
            break;
        }
    }
    if (chat_is_not_empty) {
        printf("Chat with chat_id %d:\n", chat_id);
    }
    for (uint8_t msg_idx = 0; msg_idx < MAX_MSG_COUNT; msg_idx++) {
        if (strcmp(chat[msg_idx], "") != 0) {
            printf("%s\n", chat[msg_idx]);
        }
    }
}


// creates a server and returns server_fd
int init_server() {
    struct sockaddr_in addr;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
                   &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd,
             (struct sockaddr *)&addr,
             sizeof(addr)) < 0)
    {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Socket auf NONBLOCKING setzen
    int flags = fcntl(server_fd, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl F_GETFL");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (fcntl(server_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl F_SETFL");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server initialization finished. Server started\n");

    return server_fd;
}


// waits for the first client to connect
// returns 0 if successful and -1 if an error occured
int wait_for_client(int server_fd, int* client_fds) {
    int success = 0;

    client_fds[0] = accept(server_fd, NULL, NULL);

    if (client_fds[0] < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            printf("No client available in wait for client loop\n");
        }
        else
        {
            perror("accept");
        }

        sleep(1);
        success = -1;
    } else {
        printf("Client connected\n");

        // client socket nonblocking too
        int flags = fcntl(client_fds[0], F_GETFL, 0);
        if (flags < 0) { // error while retrieving flags
            perror("fcntl F_GETFL");
            close(client_fds[0]);
            client_fds[0] = -1;
            success = -1;
        }

        if (success == 0) {
            if (fcntl(client_fds[0], F_SETFL, flags | O_NONBLOCK) < 0) { // error while setting flags
                perror("fcntl F_SETFL");
                close(client_fds[0]);
                client_fds[0] = -1;
                success = -1;
            }
        }
    }

    return success;
}


void connect_more_clients(int server_fd, int* client_fds, int MAX_CLIENT_COUNT) {
    for (int i = 1; i < MAX_CLIENT_COUNT; i++) {
        if (client_fds[i] < 0) {
            client_fds[i] = accept(server_fd, NULL, NULL);
            if (client_fds[i] >= 0) {
                printf("----------\nClient connected!\n----------\n");
            }
            printf("client_fds[i] = [");
            for (int j = 0; j < 10; j++) {
                printf("%i ", client_fds[j]);
            }
            printf("]\n");
        }
    

        if (client_fds[i] < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                int number_of_clients = 0;
                for (int j = 0; j < MAX_CLIENT_COUNT; j++) {
                    if (client_fds[j] >= 0) {
                        number_of_clients++;
                    }
                }
                printf("Still %i clients available\n", number_of_clients);
                break;
            }
            else
            {
                perror("accept");
            }
        }

        // Auch Client-Socket nonblocking
        int flags = fcntl(client_fds[i], F_GETFL, 0);
        if (flags < 0) {
            perror("fcntl F_GETFL");
            close(client_fds[i]);
            client_fds[i] = -1;
            continue;
        }

        if (fcntl(client_fds[i], F_SETFL, flags | O_NONBLOCK) < 0) {
            perror("fcntl F_SETFL");
            close(client_fds[i]);
            client_fds[i] = -1;
            continue;
        }
    }
}

void send_json_response(char* response_string, int peer_fd) {
    printf("Sending response: %s\n\n", response_string);

    // send response to client
    ssize_t sent = send(peer_fd,
                        response_string,
                        strlen(response_string),
                        MSG_NOSIGNAL);

    if (sent < 0) {
        perror("send_json_response: send");
    }
}


// // detect gaps in the fd list 
// // and rearange the list if gaps are there
void reorder_list(int peer_fds[MAX_PEER_COUNT], int print) {
    while (1) {
        // detect non neg fd entries and their indices
        int non_negative_fd_idxs[MAX_PEER_COUNT];
        int non_neg_incoming_fds_count = detect_gaps_in_fds(peer_fds, non_negative_fd_idxs, print);

        // detect gaps and their indices and count them
        int gap_list_idxs[non_neg_incoming_fds_count];
        int gap_count = has_gaps(peer_fds, non_neg_incoming_fds_count, gap_list_idxs, print);
        
        if (gap_count == 0) { // no gaps -> no changes
            if (print) {
                printf("fd list is in the right order.\n");
            }
            break;
        }

        // rearange the array to get rid of the gaps
        int changes = rearange_peer_fd_list(peer_fds, non_negative_fd_idxs, 
            non_neg_incoming_fds_count, gap_list_idxs, gap_count, print);
        
        if (print) {
            printf("There happened %d changes when rearanging.", changes);
        }
    }
}


// returns 0 if no non negative peer_fds, 
// otherwise number of non negative peer_fds
// can be applied on any peer_fd_list (incoming or outgoing)
int detect_gaps_in_fds(int peer_fd_list[MAX_PEER_COUNT], int non_negative_fd_idxs[MAX_PEER_COUNT], int print) {
    // check if incoming fds are sorted
    int non_negative_fd_idxs_idx = 0; // holds idxs of non negative entries from 0 to non_negative_fds_count_count-1
    for (int i = 0; i < MAX_PEER_COUNT; i++) {
        if (peer_fd_list[i] != -1) {
            if (peer_fd_list[i] < -1) {
                fprintf(stderr, "FILE FD < -1 SHOULD NOT BE POSSIBLE");
            } else {
                non_negative_fd_idxs[non_negative_fd_idxs_idx] = i;
                non_negative_fd_idxs_idx++;
            }
        }
    }
    if (non_negative_fd_idxs_idx != 0) {
        if (print) {
            printf("\n%d non negative fds, their indexes in the lists are:\n[", non_negative_fd_idxs_idx);
        }
        for (int i = 0; i < non_negative_fd_idxs_idx; i++) {
            if (print) {
                printf("%d", non_negative_fd_idxs[i]);
            }
            if (i < non_negative_fd_idxs_idx - 1 && print) {
                printf(", ");
            }
        }
        if (print) {
            printf("]\n");
        }
    } else {
        if (print) {
            printf("No non negative fds available.\n");
        }
    }
    
    return non_negative_fd_idxs_idx; // returns 0 if no non negative peer_fds, otherwise number of non negative peer_fds
}


// checks if there are gaps, meaning a -1 before a non negative number
// returns if there were gaps
int has_gaps(int peer_fd_list[MAX_PEER_COUNT], 
        int non_negative_fd_count, int gap_list_idxs[non_negative_fd_count], int print) { // non_negative_fd_count entries are enough for non_negative_fd_count peers
    int gaps = 0;
    for (int fd_idx = 0; fd_idx < non_negative_fd_count; fd_idx++) {
        if (peer_fd_list[fd_idx] < 0) {
            gap_list_idxs[gaps] = fd_idx;
            gaps++;
        }
    }

    if (gaps && print) {
        printf("There are %d gaps at the following indices:\n[", gaps);
        for (int gap_idx = 0; gap_idx < gaps; gap_idx++) {
            printf("%d", gap_list_idxs[gap_idx]);
            if (gap_idx < gaps - 1) {
                printf(", ");
            }
        }
        printf("]\n");
    } else {
        if (print) {
            printf("List has no gaps\n");
        }
    }
    return gaps;
}


// returns 0 if no rearanging occured, else number of rearangements
int rearange_peer_fd_list(int peer_fd_list[MAX_PEER_COUNT], int non_negative_fd_idxs[MAX_PEER_COUNT], 
        int non_negative_fd_count, int gap_list_idxs[non_negative_fd_count], int gap_count, int print) {
    int changes = 0; 
    if (gap_count < 0) {
        if (print) {
            printf("Error: Gap count cannot be negative!\n\n");
        }
        changes = -1; // as error code
    } else if (gap_count == 0) { // else no changes were required so no rearanging occured
        if (print) {
            printf("No rearangement required because there are no gaps.\n\n");
        }
        changes = 0;
    } else { // rearanging has to take place#
        for (int gap_number = 0; gap_number < gap_count; gap_number++) {
            /*
            Algorithm to rearange:
            Where is the gap_number+1th gap? -> gap_list_idxs[gap_number]
            take it's position: gap_list_idxs[gap_number] -> the element is peer_fd_list[gap_list_idxs[gap_number]] and element == -1
            now look for the next biggest number in non_neg_fd_idxs_idx that's a bit bigger than the gap index (gap_list_idxs[gap_number])
            save this index in next_non_neg_fd_idx: it is the index of the fd that has to be swapped into the previous gap
            */
            int next_non_neg_fd_idx = -1;

            for (int non_neg_fd_idxs_idx = 0; non_neg_fd_idxs_idx < non_negative_fd_count; non_neg_fd_idxs_idx++) {
                if (non_negative_fd_idxs[non_neg_fd_idxs_idx] > gap_list_idxs[gap_number]) {
                    next_non_neg_fd_idx = non_negative_fd_idxs[non_neg_fd_idxs_idx];
                    break;
                }
            }
            peer_fd_list[gap_list_idxs[gap_number]] = peer_fd_list[next_non_neg_fd_idx];
            peer_fd_list[next_non_neg_fd_idx] = -1;
            changes++;
        }
    }
    return changes;
}



void reorder_peer_lists(Peer* peer) {
    reorder_list(peer->incoming_peer_fds, 0);
    reorder_list(peer->outgoing_peer_fds, 0);
    reorder_list(peer->connected_peer_server_ports, 0);
}

void reorder_peer_lists_and_print(Peer* peer, int64_t* last_rearange_time) {
    printf("\nRestructuring the incoming peer_fd list if required:\n");
    reorder_list(peer->incoming_peer_fds, 1);

    printf("\nRestructuring the outgoing peer_fd list if required:\n");
    reorder_list(peer->outgoing_peer_fds, 1);

    printf("\nRestructuring the server ports list if required:\n");
    reorder_list(peer->connected_peer_server_ports, 1);
        
    printf("\n\nOveview over connected peers AFTER REORDERING:\n");
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


    *last_rearange_time = millis();
}


// Generate the request according to the current workflow state.
int generate_str_request(Request* req, WorkflowState workflow_state, uint8_t *workflow_chat_id, 
        size_t workflow_content_length, int *workflow_msg_id) {
    
    int success = 0;

    if (*workflow_chat_id >= MAX_CHAT_COUNT) {
        success = -2;
    } else if ( *workflow_msg_id >= MAX_MSG_COUNT) {
        success = -1;
    } else {
        switch (workflow_state) { // AND THE PEER_FD? RANDOM? OR PRE DEFINED???
            case WORKFLOW_GET_CHAT:
                *workflow_chat_id = rand() % MAX_CHAT_COUNT;
                req->request_str = get_chat(req->request_msg_id + 1,
                                        *workflow_chat_id);
                break;

            case WORKFLOW_ADD_MSG:
                req->request_str = add_message(req->request_msg_id + 1,
                                            *workflow_chat_id);
                break;

            case WORKFLOW_VERIFY_ADD:
                req->request_str = get_chat(req->request_msg_id + 1,
                                        *workflow_chat_id);
                break;

            case WORKFLOW_EDIT_MSG:
                if (workflow_content_length > 0) {
                    *workflow_msg_id = rand() % workflow_content_length;
                    req->request_str = edit_message(req->request_msg_id + 1,
                                                *workflow_chat_id,
                                                *workflow_msg_id);
                }
                break;

            case WORKFLOW_VERIFY_EDIT:
                req->request_str = get_message(req->request_msg_id + 1,
                                            *workflow_chat_id,
                                            *workflow_msg_id);
                break;
        }

    }

    return success;
}


// Update the workflow state according to the response.
// returns 0 if fine, -1 if you have to delete the response and continue
int update_workflow_state(ParsedReceivedResponse* pr_response, WorkflowState* workflow_state, uint8_t *workflow_chat_id, size_t* workflow_content_length) {
    
    int success = 0;

    if (*workflow_chat_id >= MAX_CHAT_COUNT) {
        success = -1;
    } else {
        switch (*workflow_state) {
            case WORKFLOW_GET_CHAT: {
                *workflow_chat_id =
                    (uint8_t)cJSON_GetNumberValue(pr_response->chat_node);

                cJSON *content =
                    cJSON_GetObjectItem(pr_response->payload, "content");

                if (!cJSON_IsArray(content)) {
                    fprintf(stderr,
                            "GET_CHAT response has no content array\n");
                    return -1;
                }

                *workflow_content_length =
                    (size_t)cJSON_GetArraySize(content);

                *workflow_state = WORKFLOW_ADD_MSG;
                break;
            }

            case WORKFLOW_ADD_MSG:
                *workflow_state = WORKFLOW_VERIFY_ADD;
                break;

            case WORKFLOW_VERIFY_ADD: {
                cJSON *content =
                    cJSON_GetObjectItem(pr_response->payload, "content");

                if (!cJSON_IsArray(content)) {
                    fprintf(stderr,
                            "GET_CHAT verification has no content array\n");
                    success = -1;
                    break;
                }

                *workflow_content_length =
                    (size_t)cJSON_GetArraySize(content);

                if (*workflow_content_length > 0) {
                    *workflow_state = WORKFLOW_EDIT_MSG;
                } else {
                    *workflow_state = WORKFLOW_GET_CHAT;
                }
                break;
            }

            case WORKFLOW_EDIT_MSG:
                *workflow_state = WORKFLOW_VERIFY_EDIT;
                break;

            case WORKFLOW_VERIFY_EDIT:
                *workflow_state = WORKFLOW_GET_CHAT;
                break;
        }
    
    }

    return success;
}
