#pragma once
#include <Arduino.h> // für Serial

#define USB_DEBUG true

#define RCCHECK(fn) { rcl_ret_t rc = fn; if(rc != RCL_RET_OK){error_loop();}}
#define RCSOFTCHECK(fn) { rcl_ret_t rc = fn; (void)rc; }

void error_loop(); // nur DEKLARATION