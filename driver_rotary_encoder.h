
#ifndef _DRIVER_ROTARY_ENCODER_H
#define _DRIVER_ROTARY_ENCODER_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"

#define ROTARY_QUEUE_LEN 10

struct rotary_data {
	int32_t cnt;
	int32_t speed;
};
 

#include "stm32f1xx_hal.h"

extern int16_t Encoder_Count;

void Encoder_Init(void);
int16_t Encoder_Get(void);
  
#endif /* _DRIVER_ROTARY_ENCODER_H */

