/*
 * communication.h
 *
 *  Created on: 2020/12/11
 *      Author: staff
 */

#ifndef COMMUNICATION_H_
#define COMMUNICATION_H_

#define UART_BUFFER_SIZE 32
#define UART_RX_LENGTH 32
#include "stdint.h"

//#define UART_RX_LENGTH Communication_Get_RxBuffer_Count()
#define BAUDRATE_115200 8

typedef struct{
	void(* Reflesh)(void);
	void(* Transmit)(uint8_t*, uint8_t);//(uint8_t *)array, (uint8_t) array_size
	void(* Reset_Error)(void);
	void(* Config_Reflesh)(uint8_t);//(uint8_t)baudrate_index

}UART_Functions_TypeDef;

typedef struct{
	void(* Timer_Start)(void);
	void(* Timeout)(void);
	void(* Set_Period)(uint16_t);//(uint8_t)baudrate_index
	uint8_t(* IsFinished)(void);//(uint8_t *)array, (uint8_t) array_size

}Interval_Timer_Functions_TypeDef;


//
//
//typedef enum Error_Code{
//	Error_Code_OverLoad_CompareOutput_1			= (uint32_t)(1<<0),
//	Error_Code_OverLoad_CompareOutput_2			= (uint32_t)(1<<1),
//	Error_Code_SystemReset						= (uint32_t)(1<<2),
//	Error_Code_WWDT_TimeOut						= (uint32_t)(1<<3),
//}Error_Code;

//
//typedef enum Parity{
//	PARITY_NONE,
//	PARITY_ODD,
//	PARITY_EVEN
//}Parity;
//
//typedef enum Stop_Bit{
//	STOP_BIT_1,
//	STOP_BIT_2,
//}Stop_Bit;
//
//typedef enum Protocol{
//	PROTOCOL_MODBUS_RTU,
//	PROTOCOL_MODBUS_ASCII,
//	PROTOCOL_IO_LINK,
//	PROTOCOL_NONE
//}Protocol;

/*
typedef enum Function_Mode{
	FUNCTION_MODE_WIN,
	FUNCTION_MODE_HYS,
}Function_Mode;
*/


typedef struct{
	uint8_t rx_buffer[8];
	uint8_t rx_buffer_count;

	uint8_t tx_buffer[8];
	uint8_t tx_buffer_count;

	uint8_t interrupt_ongoing;
//
//	Communication_Interval_Timer_TypeDef interval_timer;
} Communication_Data_TypeDef;
//


#endif /* COMMUNICATION_H_ */
