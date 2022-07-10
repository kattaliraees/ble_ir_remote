#pragma once

#include "stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

void ir_init(void);
void ir_send_nec(uint16_t addr, uint16_t cmd);


#ifdef __cplusplus
}
#endif