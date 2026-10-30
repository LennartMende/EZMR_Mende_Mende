#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <time.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#include <cjson/cJSON.h>

#include "pdu_parsing.h"
#include "calculator.h"
#include "server_utils.h"



int main(void)
{
    int server_fd;
    int client_fds[MAX_CLIENT_COUNT];
    for (int i = 0; i < MAX_CLIENT_COUNT; i++) {
        client_fds[i] = -1;
    }
    

    struct sockaddr_in addr;

    char buffer[1024];

    uint32_t response_msg_id = 0;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

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

    printf("Nonblocking server started\n");

    while (1)
    {
        if (client_fds[0] < 0)
        {
            client_fds[0] = accept(server_fd, NULL, NULL);

            if (client_fds[0] < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    printf("No client available initial loop\n");
                }
                else
                {
                    perror("accept");
                }

                sleep(1);
                continue;
            }

            printf("Client connected\n");

            // Auch Client-Socket nonblocking
            flags = fcntl(client_fds[0], F_GETFL, 0);
            if (flags < 0) {
                perror("fcntl F_GETFL");
                close(client_fds[0]);
                client_fds[0] = -1;
                continue;
            }

            if (fcntl(client_fds[0], F_SETFL, flags | O_NONBLOCK) < 0) {
                perror("fcntl F_SETFL");
                close(client_fds[0]);
                client_fds[0] = -1;
                continue;
            }
        } else {
            int i;
            for (i = 1; i < MAX_CLIENT_COUNT; i++) {
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
                            //printf("client_fds[%i] = %i\n", j, client_fds[j]);
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
                flags = fcntl(client_fds[i], F_GETFL, 0);
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

        // json: {header: {msg_id: ..., time_stamp}, payload: {operands: ..., operation: ...}}
        for (int i = 0; i < MAX_CLIENT_COUNT; i++) {
            if (client_fds[i] > 0) { // iterate over all valid connections (valid, if client_fd > 0)
                ssize_t n = recv(client_fds[i], buffer, sizeof(buffer)-1, 0);

                if (n > 0)
                {
                    buffer[n] = '\0';
                    printf("Data received on client %i: %s\n", i, buffer);


                    // Json parsen
                    // json: buffer = "{"header": {"msg_id": ..., "time_stamp": ...}, "payload": {"operands": ..., "operation": ...}};
                    // create pdu and fill it step-by-step from buffer
                    JsonPdu pdu;
                    pdu.json_msg = cJSON_Parse(buffer); // Json-Message parsen
                    if(test_json_format(pdu.json_msg, "json_msg") < 0) { // json_msg is not json conform 
                        continue;
                    }

                    // header
                    if(exists_json_node(pdu.json_msg, &pdu.header, "header") < 0) { // json_node has no header field
                        continue;
                    }

                    // header | msg_id
                    double id;
                    if(exists_json_node(pdu.header, &pdu.msg_id, "msg_id") < 0) { // no msg_id field existing
                        continue;
                    } else if (is_json_node_number(pdu.msg_id)) {
                        continue;
                    } else if (is_json_node_uint(pdu.msg_id, &id, 32)) {
                        continue;
                    }

                    // header | time_stamp
                    double time_stamp;
                    if(exists_json_node(pdu.header, &pdu.time_stamp, "time_stamp") < 0) { // no time_stamp field existing
                        continue;
                    } else if (is_json_node_number(pdu.time_stamp)) {
                        continue;
                    } else if (is_json_node_uint(pdu.time_stamp, &time_stamp, 64)) {
                        continue;
                    }


                    // PAYLOAD
                    if(exists_json_node(pdu.json_msg, &pdu.payload, "payload") < 0) { // payload field missing
                        continue;
                    }

                    // initialize operand_values array and operand_count for later use
                    double *operand_values = NULL;
                    int operand_count = 0;

                    // OPERANDS
                    if(exists_json_node(pdu.payload, &pdu.operands, "operands")) { // operands field missing
                        continue;
                    }

                    // json payload for operands in format "operands": [1, 2, 3, 4, ...] -> floats zulässig?
                    else if (is_json_node_array(pdu.operands)) {
                        continue;
                    } else {
                        operand_count = cJSON_GetArraySize(pdu.operands);
                        if (operand_count < 2) {
                            printf("operands array has less than 2 elements\n");
                            fprintf(stderr, "operands array has less than 2 elements");
                        }
                        else {
                            // allocate memory for operand_values array
                            operand_values = malloc(operand_count * sizeof(double));

                            if (operand_values == NULL) {
                                printf("Failed to allocate memory for operands\n");
                                fprintf(stderr, "Failed to allocate memory for operands\n");
                            }
                            else {
                                int break_loop = 0;
                                for (int i = 0; i < operand_count; i++) {
                                    cJSON* operand = cJSON_GetArrayItem(pdu.operands, i);

                                    if (is_json_node_number(operand) < 0) {
                                        break_loop = 1;
                                        break;
                                    }
                                    else {
                                        operand_values[i] = cJSON_GetNumberValue(operand);
                                    }
                                }
                                if (break_loop) {
                                    continue;
                                }
                            }
                        }
                    }

                    // initialize result variable for later use
                    double result = 0.0;
                    int successful_calculation = 0;

                    if(exists_json_node(pdu.payload, &pdu.operation, "operation") < 0) {
                        continue;
                    } else if (is_json_node_string(pdu.operation) < 0) {
                        continue;
                    }
                    else {
                        const char *operation_value = cJSON_GetStringValue(pdu.operation);

                        successful_calculation = calculate_request(
                            operand_values, operand_count, operation_value, &result
                        );
                    }


                    if (response_msg_id == UINT32_MAX) {
                        response_msg_id = 0;
                    } else {
                        response_msg_id++;
                    }

                    uint64_t response_timestamp = (uint64_t)time(NULL);

                    // create response json object
                    cJSON *response = cJSON_CreateObject();
                    cJSON *response_header = cJSON_CreateObject();
                    cJSON *response_payload = cJSON_CreateObject();

                    cJSON_AddItemToObject(response, "header", response_header);
                    cJSON_AddNumberToObject(response_header, "msg_id", response_msg_id);
                    cJSON_AddNumberToObject(response_header, "time_stamp", response_timestamp);

                    cJSON_AddItemToObject(response, "payload", response_payload);
                    
                    if (successful_calculation == 0) {
                        cJSON_AddNumberToObject(response_payload, "result", result);
                    } else if (successful_calculation < 0) {
                        cJSON_AddStringToObject(response_payload, "result", "NULL");
                    }
                    

                    // convert response json object to string
                    
                    char *response_string = cJSON_PrintUnformatted(response);

                    if (response_string == NULL) {
                        printf("Failed to create response string\n");
                    } else {
                        printf("Sending response: %s\n", response_string);

                        // send response to client
                        ssize_t sent = send(client_fds[i],
                                            response_string,
                                            strlen(response_string),
                                            0);

                        if (sent < 0) {
                            fprintf(stderr, "Error occured during sending");
                            printf("Error occured during sending");
                        }
                    }


                    free(response_string);
                    free(operand_values);

                    cJSON_Delete(pdu.json_msg);   // man muss nur die msg deleten, um den ganzen Baum zu löschen
                    cJSON_Delete(response);
                } else if (n == 0)
                {
                    printf("Client disconnected\n");

                    close(client_fds[i]);
                    client_fds[i] = -1;
                }
                else
                {
                    if (errno == EAGAIN || errno == EWOULDBLOCK)
                    {
                        printf("No data available for client %i\n", i);
                    }
                    else
                    {
                        perror("recv");

                        close(client_fds[i]);
                        client_fds[i] = -1;
                    }
                }
            }
        }

        reorder_list(client_fds);

        sleep(1);
    }

    close(server_fd);

    return 0;
}
