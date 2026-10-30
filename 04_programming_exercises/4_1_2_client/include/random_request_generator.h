#ifndef RANDOM_REQUEST_GENERATOR_H
#define RANDOM_REQUEST_GENERATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <cjson/cJSON.h>

char* generate_random_request(uint32_t msg_id);

#endif
