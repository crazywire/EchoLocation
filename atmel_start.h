#ifndef ATMEL_START_H_INCLUDED
#define ATMEL_START_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "include/driver_init.h"
#include "include/atmel_start_pins.h"

/**
 * Initializes MCU, drivers and middleware in the project
**/
void atmel_start_init(void);
extern volatile uint16_t t1, t2;
#ifdef __cplusplus
}
#endif
#endif
