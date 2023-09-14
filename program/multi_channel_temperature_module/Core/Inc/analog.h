/*
 * analog.h
 *
 *  Created on: Nov 22, 2021
 *      Author: staff
 */

#ifndef INC_ANALOG_H_
#define INC_ANALOG_H_
#include "stdint.h"


typedef struct{
	int32_t temperature;



	uint16_t referece_temperature_adc;
}Analog_TypeDef;


typedef enum{
	TEMPERATURE_CHANNEL_REFERENCE,
	TEMPERATURE_CHANNEL_VREF,
	TEMPERATURE_CHANNEL_CH1,
	TEMPERATURE_CHANNEL_CH2,
	TEMPERATURE_CHANNEL_CH3,
	TEMPERATURE_CHANNEL_CH4,
	TEMPERATURE_CHANNEL_VDD,
}TEMPERATURE_CHANNEL;

#endif /* INC_ANALOG_H_ */
