/*
 * error.h
 *
 *  Created on: 2022/02/24
 *      Author: staff
 */

#ifndef ERROR_H_
#define ERROR_H_


typedef enum{
	SC_NO_ERROR = 0,
	SC_OVER_FLOW = 1 << 0,
	SC_UNDER_FLOW = 1 << 1,
	SC_MODBUS_ERROR = 1 << 2,
	SC_REWRITE_LIMIT_ERROR = 1 << 3,
	SC_EEPROM_ERROR = 1 << 4,
	SC_ZERO_ADJUSTMENT_ERROR = 1<<5,
}STATUS_CODE;

typedef enum{
	EC_NO_ERROR = 0,
	EC_COMMAND_ERROR = 1<<0,
	EC_ADDRESS_ERROR = 1<<1,
	EC_DATA_ERROR = 1<<2,
	EC_EXECUTE_ERROR = 1<<3,
}ERROR_CODE;


#endif /* ERROR_H_ */
