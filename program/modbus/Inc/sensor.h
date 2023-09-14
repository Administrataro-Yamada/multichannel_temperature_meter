/*
 * sensor.h
 *
 *  Created on: 2021/10/21
 *      Author: staff
 */

#ifndef SENSOR_H_
#define SENSOR_H_

typedef struct{
	int16_t pressure_min;
	int16_t pressure_max;
	int16_t percentage_min;
	int16_t percentage_max;
	uint8_t reset_flag_min : 1;
	uint8_t reset_flag_max : 1;
	uint8_t overflowed_min : 1;
	uint8_t overflowed_max : 1;
	uint8_t underflowed_min : 1;
	uint8_t underflowed_max : 1;
}Sensor_Store_TypeDef;

typedef struct{
	Sensor_Store_TypeDef output;
	Sensor_Store_TypeDef display;

}Sensor_TypeDef;

#endif /* SENSOR_H_ */
