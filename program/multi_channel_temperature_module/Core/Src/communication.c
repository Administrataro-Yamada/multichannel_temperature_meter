/*
 * communication.c
 *
 *  Created on: 2020/12/11
 *      Author: staff
 */


#include "modbus.h"
#include "communication.h"



//static UART_HandleTypeDef *huart;



static UART_Functions_TypeDef *UART_Fucntions;
static Interval_Timer_Functions_TypeDef *Interval_Timer_Functions;
static Communication_Data_TypeDef communication_data;

static uint8_t uart_rx_buffer[UART_BUFFER_SIZE];
static uint8_t modbus_slave_address;


void Communication_Set_SlaveAddress(uint8_t slave_address){
	modbus_slave_address = slave_address;
}

uint8_t Communication_Get_SlaveAddress(){
	return modbus_slave_address;
}




//original functions

uint8_t Communication_Get_RxBuffer_Count(){
	return communication_data.rx_buffer_count;
}

void Communication_Set_RxBuffer_Count(uint8_t len){
	communication_data.rx_buffer_count = len;
}

uint8_t Communication_Get_UART_Interrupt_Status(){
	return communication_data.interrupt_ongoing;
}
void Communication_Set_UART_Interrupt_Status(uint8_t tf){
	communication_data.interrupt_ongoing = tf;
}

ERROR_CODE Communication_Set_ErrorCode(uint32_t error_code){
	return (ERROR_CODE)error_code;
}


//app_freertos.c(1ms)
void Communication_Get_Array_From_RingBuffer(uint8_t *array, uint8_t start_index, uint8_t size){
	uint8_t index = 0;
	for(uint8_t i = 0; i < size; i++){
		index = start_index + i;
		if(index >= UART_BUFFER_SIZE){
			index -= UART_BUFFER_SIZE;
		}
		array[i] = uart_rx_buffer[index];
	}
}

void Communication_Clear_Array_From_RingBuffer(uint8_t start_index, uint8_t size){
	uint8_t index = 0;
	for(uint8_t i = 0; i < size; i++){
		index = start_index + i;
		if(index >= UART_BUFFER_SIZE){
			index -= UART_BUFFER_SIZE;
		}
		uart_rx_buffer[index] = 0xff;
	}
}


//Check_RingBuffer
void Communication_Process_Cyclic(){

	volatile static uint8_t arranged_array[UART_BUFFER_SIZE];
	volatile static uint8_t rx_size;
	volatile static uint8_t transmit_enabled = 0;
	volatile static uint8_t process_ongoing = 0;
	volatile static uint8_t response_array[UART_BUFFER_SIZE], response_array_size;


	Communication_Reset_Error();


	if(!process_ongoing){
		for(uint8_t i = 0; i < UART_BUFFER_SIZE; i++){
			if((uart_rx_buffer[i] == modbus_slave_address)
					|| (uart_rx_buffer[i] == 255)){


				//先頭にデータの頭を持ってくる
				Communication_Get_Array_From_RingBuffer(arranged_array, i, UART_BUFFER_SIZE);


				//命令とCRCだけチェック、長さを取得
				rx_size = MODBUS_Check_Data_Get_Length(arranged_array);
				if(rx_size == 0){
					continue;
				}

				process_ongoing = 1;
				Communication_Clear_Array_From_RingBuffer(i, rx_size);


				MODBUS_Branch_Process(arranged_array, rx_size, response_array, &response_array_size);

				if(response_array_size == 0){
					process_ongoing = 0;
				}
				else if(arranged_array[0] == 255){
					process_ongoing = 0;
				}
				else{
					transmit_enabled = 1;//送信フラグ
					Interval_Timer_Functions->Timer_Start();//wait 3.5T
				}

				break;
			}
		}
	}

	if(Interval_Timer_Functions->IsFinished()){
		process_ongoing = 0;
		if(transmit_enabled){
			transmit_enabled = 0;
			UART_Fucntions->Transmit(response_array, response_array_size);
			return;
		}
	}
}

void Communication_Init(UART_Functions_TypeDef *_uart_fuctions, Interval_Timer_Functions_TypeDef *_interval_timer_functions){

	UART_Fucntions = _uart_fuctions;
	Interval_Timer_Functions = _interval_timer_functions;


	MODBUS_Init();


	UART_Fucntions->Config_Reflesh(BAUDRATE_115200);
	UART_Fucntions->Reflesh();


	//0x00で埋めるとCRC改竄に弱くなるため0xffで埋める
	for(uint8_t i = 0; i < UART_BUFFER_SIZE; i++){
		uart_rx_buffer[i] = 0xff;
	}

}
