/*
 * global_typedef.h
 *
 *  Created on: 2021/10/19
 *      Author: staff
 */

#ifndef GLOBAL_TYPEDEF_H_
#define GLOBAL_TYPEDEF_H_
#define UART_BUFFER_SIZE 20
#include "stdint.h"

typedef struct{
	uint8_t slave_address;
	uint8_t baudrate;
}Flash_TypeDef;

typedef struct{
	uint8_t tx_on_going : 1;
	uint8_t tim_on_going : 1;
	uint8_t backup_request : 1;
	uint8_t load_request : 1;
	uint8_t uart_rx_buffer[UART_BUFFER_SIZE];
}SRAM_TypeDef;

typedef struct{
	Flash_TypeDef flash;
	SRAM_TypeDef sram;
}Memory_TypeDef;



#endif /* GLOBAL_TYPEDEF_H_ */
