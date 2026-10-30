#include "server_utils.h"

#include "constants.h"

// the following routine searches in the ipc_files dir for FIFO files 
// and stores them in response_fifo_file_paths
int register_fifo_files(char response_fifo_file_paths[MAX_CLIENT_COUNT][FIFO_PATH_LENGTH]) {
    DIR *dir = opendir(IPC_FILE_DIR);

    if (dir == NULL) {
        perror("Could not open directory");
        return -1;
    }

    struct dirent *entry; // directory entry as file
    struct stat file_info;

    int connected_clients = 0;
    while ((entry = readdir(dir)) != NULL) {
        // skip first entries
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0 ||
            strstr(entry->d_name, "c2s")) {
            continue;
        }

        // read file name and path into different variables
        char file_name[256]; 
        snprintf(file_name, sizeof(file_name), "%s", entry->d_name);

        char full_file_path[256+17]; // length = 256 + strlen(IPC_FILE_DIR)
        snprintf(full_file_path, sizeof(full_file_path), "%s%s", IPC_FILE_DIR, entry->d_name);

        if (lstat(full_file_path, &file_info) == -1) {
            perror("lstat");
            continue;
        }

        // now check if the file at full_file_path is a FIFO file, if so write the file path into the paths list
        if (S_ISFIFO(file_info.st_mode)) {
            strcpy(response_fifo_file_paths[connected_clients], IPC_FILE_DIR);
            strcat(response_fifo_file_paths[connected_clients], entry->d_name);
            printf("FIFO file found: %s\n", entry->d_name);
            connected_clients++;
        }
    }

    closedir(dir);

    return connected_clients;
}


// extend the existing fifo file list during runtime for new clients
int extend_fifo_files(int connected_clients, char response_fifo_file_paths[MAX_CLIENT_COUNT][FIFO_PATH_LENGTH]) { // connected_clients means the number of already connected clients
    DIR *dir = opendir(IPC_FILE_DIR);

    if (dir == NULL) {
        perror("Could not open directory");
        return -1;
    }

    struct dirent *entry; // directory entry as file
    struct stat file_info;

    int connected_clients_new = connected_clients;
    while ((entry = readdir(dir)) != NULL) {
        // skip first entries
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0 ||
            strstr(entry->d_name, "c2s")) {
                continue;
        }

        // read file name and path into different variables
        char file_name[256]; 
        snprintf(file_name, sizeof(file_name), "%s", entry->d_name);
        char full_file_path[256+17]; // length = 256 + strlen(IPC_FILE_DIR)
        snprintf(full_file_path, sizeof(full_file_path), "%s%s", IPC_FILE_DIR, entry->d_name);

        // now check if the file path is already in the connected_clients_list:
        int client_already_connected = 0;
        for (int i = 0; i < connected_clients_new; i++) {
            if (strcmp(full_file_path, response_fifo_file_paths[i]) == 0) {
                client_already_connected = 1;
                break;
            }
        }
        if (client_already_connected) {
            continue;
        }

        if (lstat(full_file_path, &file_info) == -1) {
            perror("lstat");
            continue;
        }

        // now check if the file at full_file_path is a FIFO file, if so write the file path into the paths list
        if (S_ISFIFO(file_info.st_mode)) {
            strcpy(response_fifo_file_paths[connected_clients_new], IPC_FILE_DIR);
            strcat(response_fifo_file_paths[connected_clients_new], entry->d_name);
            printf("FIFO file found: %s\n", entry->d_name);
            connected_clients_new++;
        }
    }

    closedir(dir);

    return connected_clients_new;
}


// the following function initiates a Writing end on the s2c FIFOs
// and stores their fds into response_fifo_fd
int discovery(int* connected_client_count, int response_fifo_fds[MAX_CLIENT_COUNT], 
        char response_fifo_file_paths[MAX_CLIENT_COUNT][FIFO_PATH_LENGTH]) {
    *connected_client_count = register_fifo_files(response_fifo_file_paths);
    if (*connected_client_count == -1) {
        return 1;
    }

    for (int i = 0; i < *connected_client_count; i++) {
        printf("Trying to open reponse FIFO at file path %s\n", response_fifo_file_paths[i]);
        response_fifo_fds[i] = open(response_fifo_file_paths[i], O_WRONLY);
        if (response_fifo_fds[i] == -1) {
            fprintf(stderr, "Response FIFO could not be opened\n");
            return 2;
        }
        printf("FIFO file for response opened\n");
    }
    return 0;
}


// the following function initiates a Writing end on the s2c FIFOs
// and stores their fds into response_fifo_fd
int runtime_discovery(int* connected_client_count, int response_fifo_fds[MAX_CLIENT_COUNT], 
        char response_fifo_file_paths[MAX_CLIENT_COUNT][FIFO_PATH_LENGTH]) {
    int old_client_count = *connected_client_count;
    *connected_client_count = extend_fifo_files(*connected_client_count, response_fifo_file_paths);
    if (*connected_client_count == -1) {
        return 7;
    }

    // DEBUG: show old and new client count: printf("old_client_count = %d, *connected_client_count = %d\n", old_client_count, *connected_client_count);

    for (int i = old_client_count; i < *connected_client_count; i++) {
        printf("Trying to open reponse FIFO at file path %s\n", response_fifo_file_paths[i]);
        response_fifo_fds[i] = open(response_fifo_file_paths[i], O_WRONLY);
        if (response_fifo_fds[i] == -1) {
            fprintf(stderr, "Response FIFO could not be opened\n");
            return 8;
        }
        printf("FIFO file for response opened\n");
    }
    return 0;
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
