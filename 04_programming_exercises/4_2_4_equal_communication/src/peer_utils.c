#define _POSIX_C_SOURCE 200809L

#include <time.h>

#include "peer_utils.h"

int64_t millis(void) {
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return ((int64_t)ts.tv_sec) * MS_PER_S + ((int64_t)ts.tv_nsec) / NS_PER_MS;
}


// returns the number of found peer registration files if successful, otherwise -1
// searches in the PEERS_DIR for files named after their peer pids
// and stores them in response_fifo_file_paths
int register_fifo_files(Peer* peer, const char *peers_dir, int print) {
    
    DIR *dir = opendir(peers_dir);

    if (dir == NULL) {
        perror("Could not open directory");
        return -1;
    }

    struct dirent *entry; // directory entry as file
    struct stat file_info;

    int connected_peers = 0;
    while ((entry = readdir(dir)) != NULL) {
        // skip first entries
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Convert filename to PID and exclude the own peer
        errno = 0;
        char *endptr;
        long parsed_pid = strtol(entry->d_name, &endptr, 10);
        if (errno == ERANGE ||
            endptr == entry->d_name ||
            *endptr != '\0' ||
            parsed_pid <= 0) {
            continue;
        }
        pid_t discovered_pid = (pid_t) parsed_pid;
        if (discovered_pid == peer->pid) {
            continue;
        }

        if (connected_peers >= MAX_PEER_COUNT) {
            fprintf(stderr, "WARNING: The connected peers list is already full!");
            break;
        }


        // store the peer file's path
        char full_file_path[256+23]; // length = 256 + strlen(IPC_FILE_DIR)
        snprintf(full_file_path, sizeof(full_file_path), "%s%s", peers_dir, entry->d_name);

        if (lstat(full_file_path, &file_info) == -1) {
            perror("lstat");
            continue;
        }
        
        peer->available_peer_pids[connected_peers] = discovered_pid;
        if (print) {
            printf("FIFO file found: %s\n", entry->d_name);
        }
        connected_peers++;
    }

    closedir(dir);

    return connected_peers;
}


//for a peer
Peer create_first_peer(pid_t pid) {
    Peer peer;
    peer.pid = pid;
    generate_first_chats(peer.chats);
    // if (init_peer_server(&peer) < 0) {
    //     fprintf(stderr, "Peer server part initialization failed");
    //     exit(10);
    // }
    for (int peer_fd_idx = 0; peer_fd_idx < MAX_PEER_COUNT; peer_fd_idx++) {
        peer.incoming_fifo_fds[peer_fd_idx] = -1;
        peer.outgoing_fifo_fds[peer_fd_idx] = -1;
        peer.connected_peer_pids[peer_fd_idx] = -1;
        peer.available_peer_pids[peer_fd_idx] = -1;
    }

    return peer;
}


void generate_first_chats(char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH]) {
    memset(chats, 0,
           MAX_CHAT_COUNT * MAX_MSG_COUNT * MAX_MSG_LENGTH);
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


void send_json_response(char* response_string, int outgoing_fifo_fd) {
    printf("Sending response: %s\n", response_string);

    // append a newline to the JSON message so the receiver can identify single messages and divide them
    size_t len = strlen(response_string);
    char *framed_response = malloc(len + 2);
    if (framed_response == NULL) {
        fprintf(stderr, "Failed to allocate memory for framed response\n");
        return;
    }

    // create a new buffer to append the newline without modifying the original JSON string
    memcpy(framed_response, response_string, len);
    framed_response[len] = '\n';
    framed_response[len + 1] = '\0';

    size_t response_len = len + 1;
    size_t total_sent = 0;

    while (total_sent < response_len) {
        ssize_t sent = write(
            outgoing_fifo_fd,
            framed_response + total_sent,
            response_len - total_sent
        );

        if (sent < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("Error occurred during writing to FIFO\n");
            free(framed_response);
            return;
        }

        if (sent == 0) {
            fprintf(stderr, "No bytes written to FIFO!\n");
            free(framed_response);
            return;
        }
        total_sent += (size_t)sent;
    }
    free(framed_response);
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
    // printf("\n\n==============msg = %s", msg);
    // printf("\n\n=================\nBEFORE manipulation:");
    // print_chat(chats, chat_id);
    strcpy(chats[chat_id][chat_length], msg);
    // printf("\n\n=================\nAFTER manipulation:");
    // print_chat(chats, chat_id);
    // printf("\n\n\n=================\n");
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


/////////////////// FUNCTIONS TO REORDER LISTS ////////////////////////

void reorder_list(int fifo_fds[MAX_PEER_COUNT], char fifo_file_paths[MAX_PEER_COUNT][FIFO_PATH_LENGTH], int print) {
    while (1) {
        // detect non neg fd entries and their indices
        int non_negative_fd_idxs[MAX_PEER_COUNT];
        int non_neg_incoming_fds_count = detect_gaps_in_fds(fifo_fds, non_negative_fd_idxs, print);

        // detect gaps and their indices and count them
        int gap_list_idxs[non_neg_incoming_fds_count];
        int gap_count = has_gaps(fifo_fds, non_neg_incoming_fds_count, gap_list_idxs, print);
        
        if (gap_count == 0) { // no gaps -> no changes
            if (print) {
                printf("fd list is in the right order.\n");
            }
            break;
        }

        // rearange the array to get rid of the gaps
        int changes = rearange_peer_fd_list(fifo_fds, non_negative_fd_idxs, 
            non_neg_incoming_fds_count, gap_list_idxs, gap_count, fifo_file_paths, print);
        
        if (print) {
            printf("There happened %d changes when rearanging.", changes);
        }
    }
}


// returns 0 if no non negative peer_fds, 
// otherwise number of non negative peer_fds
// can be applied on any peer_fd_list (incoming or outgoing)
int detect_gaps_in_fds(int fifo_fd_list[MAX_PEER_COUNT], int non_negative_fd_idxs[MAX_PEER_COUNT], int print) {
    // check if incoming fds are sorted
    int non_negative_fd_idxs_idx = 0; // holds idxs of non negative entries from 0 to non_negative_fds_count_count-1
    for (int i = 0; i < MAX_PEER_COUNT; i++) {
        if (fifo_fd_list[i] != -1) {
            if (fifo_fd_list[i] < -1) {
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
int has_gaps(int fifo_fd_list[MAX_PEER_COUNT], 
        int non_negative_fd_count, int gap_list_idxs[non_negative_fd_count], int print) { // non_negative_fd_count entries are enough for non_negative_fd_count peers
    int gaps = 0;
    for (int fd_idx = 0; fd_idx < non_negative_fd_count; fd_idx++) {
        if (fifo_fd_list[fd_idx] < 0) {
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
int rearange_peer_fd_list(int fifo_fd_list[MAX_PEER_COUNT], int non_negative_fd_idxs[MAX_PEER_COUNT], 
        int non_negative_fd_count, int gap_list_idxs[non_negative_fd_count], int gap_count, 
        char fifo_file_paths[MAX_PEER_COUNT][FIFO_PATH_LENGTH], int print) {
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
            take it's position: gap_list_idxs[gap_number] -> the element is fifo_fd_list[gap_list_idxs[gap_number]] and element == -1
            now look for the next biggest number in non_neg_fd_idxs_idx that's a bit bigger than the gap index (gap_list_idxs[gap_number])
            save this index in next_non_neg_fd_idx: it is the index of the fd that has to be swapped into the previous gap
            */
            int next_non_neg_fd_idx = -1;
            int gap_idx = gap_list_idxs[gap_number];

            for (int non_neg_fd_idxs_idx = 0; non_neg_fd_idxs_idx < non_negative_fd_count; non_neg_fd_idxs_idx++) {
                if (non_negative_fd_idxs[non_neg_fd_idxs_idx] > gap_idx) {
                    next_non_neg_fd_idx = non_negative_fd_idxs[non_neg_fd_idxs_idx];
                    break;
                }
            }
            
            if (next_non_neg_fd_idx < 0) { // start next loop iteration if there is no valid fd provided
                continue;
            }

            fifo_fd_list[gap_idx] = fifo_fd_list[next_non_neg_fd_idx];
            fifo_fd_list[next_non_neg_fd_idx] = -1;

            // if a file paths array is provided, make the changes there too
            if (fifo_file_paths != NULL) {
                strcpy(
                    fifo_file_paths[gap_idx],
                    fifo_file_paths[next_non_neg_fd_idx]
                );

                fifo_file_paths[next_non_neg_fd_idx][0] = '\0';
            }
            changes++;
        }
    }
    return changes;
}



void reorder_peer_lists(Peer* peer) {
    reorder_list(peer->incoming_fifo_fds, peer->incoming_fifo_file_paths, 0);
    reorder_list(peer->outgoing_fifo_fds, peer->outgoing_fifo_file_paths, 0);
    reorder_list(peer->connected_peer_pids, NULL, 0);
}

void reorder_peer_lists_and_print(Peer* peer, int64_t* last_rearange_time) {
    printf("\nRestructuring the incoming fifo_fd list if required:\n");
    reorder_list(peer->incoming_fifo_fds, peer->incoming_fifo_file_paths, 1);

    printf("\nRestructuring the outgoing fifo_fd list if required:\n");
    reorder_list(peer->outgoing_fifo_fds, peer->outgoing_fifo_file_paths, 1);

    printf("\nRestructuring the connected peer pid list if required:\n");
    reorder_list(peer->connected_peer_pids, NULL, 1);
        
    printf("\n\nOverview over connected peers AFTER REORDERING:\n");
    printf("peer->connected_peer_pids[0:10] = [");
    for (int j = 0; j < 10; j++) {
        printf("%i ", peer->connected_peer_pids[j]);
    }
    printf("]\n");
    printf("peer->incoming_fifo_fds[0:10] = [");
    for (int j = 0; j < 10; j++) {
        printf("%i ", peer->incoming_fifo_fds[j]);
    }
    printf("]\n");
    printf("peer->incoming_fifo_file_paths[0:10] = [");
    for (int j = 0; j < 10; j++) {
        printf("%s ", peer->incoming_fifo_file_paths[j]);
    }
    printf("]\n");
    printf("peer->outgoing_fifo_fds[0:10] = [");
    for (int j = 0; j < 10; j++) {
        printf("%i ", peer->outgoing_fifo_fds[j]);
    }
    printf("]\n");
    printf("peer->outgoing_fifo_file_paths[0:10] = [");
    for (int j = 0; j < 10; j++) {
        printf("%s ", peer->outgoing_fifo_file_paths[j]);
    }
    printf("]\n");


    *last_rearange_time = millis();
}

// returns 1 if the dest_pid is available, 0 otherwise
int is_dest_pid_available(const char* peers_dir, pid_t dest_pid) {
    DIR *dir = opendir(peers_dir);

    printf("Checking destination PID: %d\n", (int)dest_pid);
    printf("Directory: %s\n", peers_dir);


    if (dir == NULL) {
        perror("Could not open directory");
        return -1;
    }

    struct dirent *entry; // directory entry as file

    while ((entry = readdir(dir)) != NULL) {
        // skip first entries
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Convert filename to PID and exclude the own peer
        errno = 0;
        char *endptr;
        long parsed_pid = strtol(entry->d_name, &endptr, 10);
        if (errno == ERANGE ||
            endptr == entry->d_name ||
            *endptr != '\0' ||
            parsed_pid <= 0) {
            continue;
        }

        printf("Found PID file: %s (PID %ld)\n",
               entry->d_name, parsed_pid);

        pid_t discovered_pid = (pid_t) parsed_pid;
        if (discovered_pid == dest_pid) {
            printf("MATCH: destination PID %d still has a file!\n",
                   (int)dest_pid);
            closedir(dir);
            return 1;
        }
    }
    closedir(dir);

    printf("NO MATCH: destination PID %d not found.\n",
           (int)dest_pid);

    return 0;
}


void restructure_peer(Peer* peer, int old_pid_idx, int64_t* last_rearange_time) {
    (peer->connected_peer_pids)[old_pid_idx] = -1;

    close((peer->incoming_fifo_fds)[old_pid_idx]);
    (peer->incoming_fifo_fds)[old_pid_idx] = -1;
    strcpy((peer->incoming_fifo_file_paths)[old_pid_idx], "");

    close((peer->outgoing_fifo_fds)[old_pid_idx]);
    (peer->outgoing_fifo_fds)[old_pid_idx] = -1;
    strcpy((peer->outgoing_fifo_file_paths)[old_pid_idx], "");

    reorder_peer_lists_and_print(peer, last_rearange_time);
}


