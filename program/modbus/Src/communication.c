/*
 * communication.c
 *
 *  Created on: 2020/12/11
 *      Author: staff
 */


#include "usart.h"
#include "tim.h"
#include "modbus.h"
#include "communication.h"
#include "global_typedef.h"



static Global_TypeDef *global_struct;
static UART_HandleTypeDef *huart_rs485;
static Communication_Interval_Timer_TypeDef interval_timer = {0};


//3.5T Counter Period
static const uint16_t _interval_time[] = {
	//0x00:1200 0x01:2400 0x02:4800 0x03:9600 0x04:14400 0x05:19200
	//0x06:38400 0x07:57600 0x08:115200 0x09:230400 0x0A:460800 0x0B:921600 baud
		29167,
		14584,
		7292,
		3646,
		2431,
		1823,
		912,
		608,
		304,
		152,
		76,
		38,
};

static const uint32_t _baudrate[] = {
		1200,
		2400,
		4800,
		9600,
		14400,
		19200,
		38400,
		57600,
		115200,
		230400,
		460800,
		921600,
};
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

void Communication_UART_Wtite(uint8_t *data, uint8_t len){
	HAL_UART_Transmit_DMA(huart_rs485, data, len);
}

Error_Code Communication_Set_ErrorCode(uint32_t error_code){
	return (Error_Code)error_code;
}

void Communication_Reflesh(){
	HAL_UART_AbortReceive_IT(huart_rs485);
	HAL_UART_Receive_DMA(huart_rs485, (uint8_t*)global_struct->ram.uart.rx_buffer, UART_BUFFER_SIZE);
}

void Communication_Reset_Error(){
	if (
		__HAL_UART_GET_FLAG(huart_rs485, UART_FLAG_ORE) ||
		__HAL_UART_GET_FLAG(huart_rs485, UART_FLAG_NE) ||
		__HAL_UART_GET_FLAG(huart_rs485, UART_FLAG_FE) ||
		__HAL_UART_GET_FLAG(huart_rs485, UART_FLAG_PE)
		){
		HAL_UART_Abort(huart_rs485);
		HAL_UART_Receive_DMA(huart_rs485, (uint8_t*)global_struct->ram.uart.rx_buffer, UART_BUFFER_SIZE);//DMA
	}
}



void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart_rs485){
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart_rs485){
	if(huart_rs485->Instance==USART1){
		Communication_Reflesh();
	}
}


//app_freertos.c(1ms)
void Communication_Get_Array_From_RingBuffer(uint8_t *array, uint8_t start_index, uint8_t size){
	uint8_t index = 0;
	for(uint8_t i = 0; i < size; i++){
		index = start_index + i;
		if(index >= UART_BUFFER_SIZE){
			index -= UART_BUFFER_SIZE;
		}
		array[i] = global_struct->ram.uart.rx_buffer[index];
	}
}

void Communication_Clear_Array_From_RingBuffer(uint8_t start_index, uint8_t size){
	uint8_t index = 0;
	for(uint8_t i = 0; i < size; i++){
		index = start_index + i;
		if(index >= UART_BUFFER_SIZE){
			index -= UART_BUFFER_SIZE;
		}
		global_struct->ram.uart.rx_buffer[index] = 0xff;//0x00で埋めるとCRC改竄に弱くなるため0xffで埋める
	}
}


static void Communication_Interval_Timer_Start(){
	if(!interval_timer.enabled){
		interval_timer.enabled = 1;
		//__HAL_TIM_SET_COUNTER(interval_timer.htim, 0);
		__HAL_TIM_CLEAR_FLAG(interval_timer.htim, TIM_FLAG_UPDATE);
		HAL_TIM_Base_Start_IT(interval_timer.htim);
	}
}

static uint8_t Communication_Interval_Timer_IsFinished(){
	uint8_t b = interval_timer.finished;
	if(b){
		interval_timer.finished = 0;
	}
	return b;
}

void Communication_Interval_Timer_Timeout(){
	interval_timer.enabled = 0;
	interval_timer.finished = 1;
	HAL_TIM_Base_Stop_IT(interval_timer.htim);
}

static void Communication_Interval_Timer_Set_Period(uint16_t counter_period){
	interval_timer.counter_period = counter_period;
	interval_timer.htim->Init.Period = counter_period;
	if (HAL_TIM_Base_Init(interval_timer.htim) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_TIM_OnePulse_Init(interval_timer.htim, TIM_OPMODE_SINGLE) != HAL_OK)
	{
		Error_Handler();
	}
	TIM_MasterConfigTypeDef sMasterConfig = {0};
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(interval_timer.htim, &sMasterConfig) != HAL_OK)
	{
		Error_Handler();
	}
}

//Check_RingBuffer
void Communication_Process_Cyclic(){

	volatile static uint8_t arranged_array[UART_BUFFER_SIZE];
	volatile static uint8_t rx_size;
	volatile static uint8_t transmit_enabled = 0;
	volatile static uint8_t process_ongoing = 0;
	volatile static uint8_t response_array[UART_BUFFER_SIZE], response_array_size;

	if( __HAL_UART_GET_FLAG(huart_rs485, UART_FLAG_ORE) ||
		__HAL_UART_GET_FLAG(huart_rs485, UART_FLAG_NE) ||
		__HAL_UART_GET_FLAG(huart_rs485, UART_FLAG_FE) ||
		__HAL_UART_GET_FLAG(huart_rs485, UART_FLAG_PE) ){
		Communication_Reflesh();
	}


	if(!process_ongoing){
		for(uint8_t i = 0; i < UART_BUFFER_SIZE; i++){
			if((global_struct->ram.uart.rx_buffer[i] == (uint8_t)global_struct->eeprom.user.communication.slave_address)
					|| (global_struct->ram.uart.rx_buffer[i] == 255)){


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
					Communication_Interval_Timer_Start();//wait 3.5T
				}

				break;
			}
		}
	}

	if(Communication_Interval_Timer_IsFinished()){
		process_ongoing = 0;
		if(transmit_enabled){
			transmit_enabled = 0;
			HAL_UART_Transmit_DMA(huart_rs485, response_array, response_array_size);
			return;
		}
	}
}


void Communication_RS485_Config_Reflesh(){


	//0x00:1200 0x01:2400 0x02:4800 0x03:9600 0x04:14400 0x05:19200
	//0x06:38600 0x07:57600 0x08:115200 0x09:230400 0x0A:460800 0x0B:921600 baud
	if(global_struct->eeprom.user.communication.baud_rate < 12){
		huart_rs485->Init.BaudRate = _baudrate[global_struct->eeprom.user.communication.baud_rate];
		Communication_Interval_Timer_Set_Period(_interval_time[global_struct->eeprom.user.communication.baud_rate]);
	}
	else{
		huart_rs485->Init.BaudRate = _baudrate[3];//9600baud
		Communication_Interval_Timer_Set_Period(_interval_time[3]);
	}


	huart_rs485->Init.WordLength = UART_WORDLENGTH_8B;

	switch(global_struct->eeprom.user.communication.stop_bit){
		case 0:
			huart_rs485->Init.StopBits = UART_STOPBITS_1;
			break;
		case 1:
			huart_rs485->Init.StopBits = UART_STOPBITS_1_5;
			break;
		case 2:
			huart_rs485->Init.StopBits = UART_STOPBITS_2;
			break;
		default:
			huart_rs485->Init.StopBits = UART_STOPBITS_1;
			break;
	}

	switch(global_struct->eeprom.user.communication.parity){
		case 0:
			huart_rs485->Init.Parity = UART_PARITY_NONE;
			break;
		case 1:
			huart_rs485->Init.WordLength = UART_PARITY_ODD;
			break;
		case 2:
			huart_rs485->Init.WordLength = UART_PARITY_EVEN;
			break;
		default:
			huart_rs485->Init.Parity = UART_PARITY_NONE;
			break;
	}

	huart_rs485->Init.Mode = UART_MODE_TX_RX;
	huart_rs485->Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart_rs485->Init.OverSampling = UART_OVERSAMPLING_16;
	huart_rs485->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart_rs485->Init.ClockPrescaler = UART_PRESCALER_DIV1;
	huart_rs485->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

	if (HAL_RS485Ex_Init(huart_rs485, UART_DE_POLARITY_HIGH, 0, 0) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_UARTEx_SetTxFifoThreshold(huart_rs485, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_UARTEx_SetRxFifoThreshold(huart_rs485, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_UARTEx_DisableFifoMode(huart_rs485) != HAL_OK)
	{
		Error_Handler();
	}
}

void Communication_Init(Global_TypeDef *_global_struct, UART_HandleTypeDef *_huart_rs485, TIM_HandleTypeDef *_htim_interval)
{
	global_struct = _global_struct;
	huart_rs485 = _huart_rs485;
	interval_timer.htim = _htim_interval;

	MODBUS_Init(global_struct);

	Communication_RS485_Config_Reflesh();

	Communication_Reflesh();

	//0x00で埋めるとCRC改竄に弱くなるため0xffで埋める
	for(uint8_t i = 0; i < global_struct->ram.uart.rx_buffer_size; i++){
		global_struct->ram.uart.rx_buffer[i] = 0xff;
	}

}
