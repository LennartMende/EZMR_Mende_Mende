#include "server_utils.h"

//#include "constants.h"

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
    addr.sin_port = htons(SERVER_PORT);
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

        // Auch Client-Socket nonblocking
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


void connect_more_clients(int server_fd, int* client_fds) {
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

void send_json_response(char* response_string, int client_fd) {
    printf("Sending response: %s\n", response_string);

    // send response to client
    ssize_t sent = send(client_fd,
                        response_string,
                        strlen(response_string),
                        0);

    if (sent < 0) {
        fprintf(stderr, "Error occured during sending");
        printf("Error occured during sending");
    }
}


extern char chats[MAX_CHAT_COUNT][MAX_MSG_COUNT][MAX_MSG_LENGTH];

void initialize_chats(void) {
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



// // detect gaps in the fd list 
// // and rearange the list if gaps are there
void reorder_list(int client_fds[MAX_CLIENT_COUNT], int print) {
    while (1) {
        // detect non neg fd entries and their indices
        int non_negative_fd_idxs[MAX_CLIENT_COUNT];
        int non_neg_incoming_fds_count = detect_gaps_in_fds(client_fds, non_negative_fd_idxs, print);

        // detect gaps and their indices and count them
        int gap_list_idxs[non_neg_incoming_fds_count];
        int gap_count = has_gaps(client_fds, non_neg_incoming_fds_count, gap_list_idxs, print);
        
        if (gap_count == 0) { // no gaps -> no changes
            if (print) {
                printf("fd list is in the right order.\n");
            }
            break;
        }

        // rearange the array to get rid of the gaps
        int changes = rearange_client_fd_list(client_fds, non_negative_fd_idxs, 
            non_neg_incoming_fds_count, gap_list_idxs, gap_count, print);
        
        if (print) {
            printf("There happened %d changes when rearanging.", changes);
        }
    }
}


// returns 0 if no non negative client_fds, 
// otherwise number of non negative client_fds
// can be applied on any client_fd_list (incoming or outgoing)
int detect_gaps_in_fds(int client_fd_list[MAX_CLIENT_COUNT], int non_negative_fd_idxs[MAX_CLIENT_COUNT], int print) {
    // check if incoming fds are sorted
    int non_negative_fd_idxs_idx = 0; // holds idxs of non negative entries from 0 to non_negative_fds_count_count-1
    for (int i = 0; i < MAX_CLIENT_COUNT; i++) {
        if (client_fd_list[i] != -1) {
            if (client_fd_list[i] < -1) {
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
    
    return non_negative_fd_idxs_idx; // returns 0 if no non negative client_fds, otherwise number of non negative client_fds
}


// checks if there are gaps, meaning a -1 before a non negative number
// returns if there were gaps
int has_gaps(int client_fd_list[MAX_CLIENT_COUNT], 
        int non_negative_fd_count, int gap_list_idxs[non_negative_fd_count], int print) { // non_negative_fd_count entries are enough for non_negative_fd_count clients
    int gaps = 0;
    for (int fd_idx = 0; fd_idx < non_negative_fd_count; fd_idx++) {
        if (client_fd_list[fd_idx] < 0) {
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
int rearange_client_fd_list(int client_fd_list[MAX_CLIENT_COUNT], int non_negative_fd_idxs[MAX_CLIENT_COUNT], 
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
            take it's position: gap_list_idxs[gap_number] -> the element is client_fd_list[gap_list_idxs[gap_number]] and element == -1
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
            client_fd_list[gap_list_idxs[gap_number]] = client_fd_list[next_non_neg_fd_idx];
            client_fd_list[next_non_neg_fd_idx] = -1;
            changes++;
        }
    }
    return changes;
}
