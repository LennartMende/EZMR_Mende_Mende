#include "calculator.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

int calculate_request(double operand_values[], size_t operand_count, const char* operation_value, double* result) {
    int success = 0;
    if (strcmp(operation_value, "+") == 0 ||
        strcmp(operation_value, "-") == 0 ||
        strcmp(operation_value, "*") == 0 ||
        strcmp(operation_value, "/") == 0) {

        if (operand_values != NULL) {

            *result = operand_values[0];

            for (size_t i = 1; i < operand_count; i++) {

                if (strcmp(operation_value, "+") == 0) {
                    *result += operand_values[i];
                }
                else if (strcmp(operation_value, "-") == 0) {
                    *result -= operand_values[i];
                }
                else if (strcmp(operation_value, "*") == 0) {
                    *result *= operand_values[i];
                }
                else if (strcmp(operation_value, "/") == 0) {

                    if (operand_values[i] == 0) {
                        printf("Error: division by zero\n");
                        //fprintf(stderr, "Error: division by zero\n");
                        success = -1;
                        break;
                    }

                    *result /= operand_values[i];
                }
            }
        }
        else {
            printf("Error: operand_values is NULL\n");
            //fprintf(stderr, "Error: operand_values is NULL\n");
            success = -1;
        }
    }
    else {
        printf("Invalid operation: %s\n", operation_value);
        success = -1;
    }

    return success;
}