#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <stddef.h>

int calculate_request(double operand_values[], size_t operand_count, const char* operation_value, double* result);

#endif