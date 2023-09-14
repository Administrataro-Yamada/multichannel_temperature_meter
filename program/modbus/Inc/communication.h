/*
 * communication.h
 *
 *  Created on: 2020/12/11
 *      Author: staff
 */

#ifndef COMMUNICATION_H_
#define COMMUNICATION_H_


#define UART_RX_LENGTH Communication_Get_RxBuffer_Count()



typedef enum Error_Code{
	Error_Code_OverLoad_CompareOutput_1			= (uint32_t)(1<<0),
	Error_Code_OverLoad_CompareOutput_2			= (uint32_t)(1<<1),
	Error_Code_SystemReset						= (uint32_t)(1<<2),
	Error_Code_WWDT_TimeOut						= (uint32_t)(1<<3),
}Error_Code;


typedef enum Parity{
	PARITY_NONE,
	PARITY_ODD,
	PARITY_EVEN
}Parity;

typedef enum Stop_Bit{
	STOP_BIT_1,
	STOP_BIT_2,
}Stop_Bit;

typedef enum Protocol{
	PROTOCOL_MODBUS_RTU,
	PROTOCOL_MODBUS_ASCII,
	PROTOCOL_IO_LINK,
	PROTOCOL_NONE
}Protocol;

/*
typedef enum Function_Mode{
	FUNCTION_MODE_WIN,
	FUNCTION_MODE_HYS,
}Function_Mode;
*/

typedef struct{
	uint16_t counter_period;
	uint8_t finished : 1;
	uint8_t enabled : 1;
	uint8_t paused : 1;
	TIM_HandleTypeDef *htim;
}Communication_Interval_Timer_TypeDef;


typedef struct{
	uint8_t rx_buffer[8];
	uint8_t rx_buffer_count;

	uint8_t tx_buffer[8];
	uint8_t tx_buffer_count;

	uint8_t interrupt_ongoing;

	Communication_Interval_Timer_TypeDef interval_timer;
} Communication_Data_TypeDef;

extern Communication_Data_TypeDef communication_data;

#endif /* COMMUNICATION_H_ */
