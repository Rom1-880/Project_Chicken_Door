#ifndef POWER_MANAGEMENT_H
#define POWER_MANAGEMENT_H

#include "main.h"


/* Handles externes (définis dans main.c) ------------------------------------*/
extern ADC_HandleTypeDef  hadc1;
extern UART_HandleTypeDef huart2;
extern RTC_HandleTypeDef  hrtc;

/* Variables externes (définies dans main.c) ---------------------------------*/
extern uint32_t         adc_buffer[2];
extern volatile uint8_t adc_ready;

/* Define batterie  ---------------------------------*/
#define OFFSET_BAT  -0.020f

/* Prototypes ----------------------------------------------------------------*/
void Aller_Au_Dodo(void);

#endif /* POWER_MANAGEMENT_H */
