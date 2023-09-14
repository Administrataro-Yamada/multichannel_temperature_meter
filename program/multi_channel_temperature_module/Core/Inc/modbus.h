/*
 * modbus.h
 *
 *  Created on: 2021/10/26
 *      Author: staff
 */

#ifndef MODBUS_H_
#define MODBUS_H_

#include "stdint.h"

typedef struct{
	void(*Reboot)();

}MODBUS_Funnctions;


typedef enum{
	NONE,
	ERROR,
}ERROR_CODE;

typedef enum {

	////////////////////////////////////////////////////////////
	//////////////////          ROM           //////////////////
	////////////////////////////////////////////////////////////

	MA_SLAVE_ADDRESS,
	MA_BAUDRATE,
	MA_TEMPERATURE_CORRECTION_00,
	MA_TEMPERATURE_CORRECTION_01,
	MA_TEMPERATURE_CORRECTION_02,
	MA_TEMPERATURE_CORRECTION_03,
	MA_TEMPERATURE_CORRECTION_04,



}MODBUS_ADDRESS_0x03_0x10;

typedef enum {

	////////////////////////////////////////////////////////////
	//////////////////          RAM           //////////////////
	////////////////////////////////////////////////////////////

	MA_TEMPERATURE_REFERENCE,
	MA_TEMPERATURE_CH1,
	MA_TEMPERATURE_CH2,
	MA_TEMPERATURE_CH3,
	MA_TEMPERATURE_CH4,


	MA_ERROR_CODE
}MODBUS_ADDRESS_0x04;

#endif /* MODBUS_H_ */
