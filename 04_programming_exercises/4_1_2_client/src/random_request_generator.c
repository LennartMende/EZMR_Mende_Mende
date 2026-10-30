#include "random_request_generator.h"


char* generate_random_request(uint32_t msg_id) {

    cJSON *request = cJSON_CreateObject();
    cJSON *header = cJSON_CreateObject();
    cJSON *payload = cJSON_CreateObject();
    cJSON *operands = cJSON_CreateArray();

    cJSON_AddItemToObject(request, "header", header);
    cJSON_AddNumberToObject(header, "msg_id", msg_id);
    cJSON_AddNumberToObject(header, "time_stamp", (uint64_t)time(NULL));

    cJSON_AddItemToObject(request, "payload", payload);

    const char* operations[] = {"+", "-", "*", "/"};
    const char* random_operation = operations[rand() % 4];
    cJSON_AddStringToObject(payload, "operation", random_operation);

    // random number of operands between 2 and 5, because x % 4 gives 0, 1, 2, or 3
    int operand_count = (rand() % 4) + 2;

    for (int i = 0; i < operand_count; i++) {

        // generate a random double between 0 and 1000
        double random_operand = ((double)rand() / RAND_MAX) * 1000.0;

        cJSON_AddItemToArray(
            operands,
            cJSON_CreateNumber(random_operand)
        );
    }

    cJSON_AddItemToObject(payload, "operands", operands);

    char *json_request = cJSON_PrintUnformatted(request);

    cJSON_Delete(request);

    return json_request;
}
