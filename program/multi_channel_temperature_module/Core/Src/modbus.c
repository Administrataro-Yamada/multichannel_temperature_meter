/*
 * modbus.c
 *
 *  Created on: 2021/10/26
 *      Author: staff
 */


#include "modbus.h"
#include "analog.h"

//static void MODBUS_Reboot(){
//	HAL_NVIC_SystemReset();
//}

static MODBUS_Funnctions *Functions;
static ERROR_CODE error_code;

static uint8_t slave_address;
static uint8_t baudrate;
static uint16_t temperature_correction[10];

void MODBUS_Set_ErrorCode(){

}
ERROR_CODE MODBUS_Get_ErrorCode(){
	return error_code;
}

void MODBUS_Clear_ErrorCode(){

}



//fixed
static uint16_t MODBUS_CRC(uint8_t *data, uint8_t len){

	uint16_t crc = 0xFFFF;
	uint8_t i,j;
	uint8_t carryFlag;
	for (i = 0; i < len; i++) {
		crc ^= *(data+i);
		for (j = 0; j < 8; j++) {
			carryFlag = crc & 1;
			crc = crc >> 1;
			if (carryFlag) {
				crc ^= 0xA001;
			}
		}
	}
	return crc;
}


//fixed
uint8_t MODBUS_Check_Data_Get_Length(uint8_t *data){

	//device id
	if((data[0] != 255) && (data[0] != slave_address)){
		return 0;
	}

	volatile uint8_t rx_size;
	uint8_t resistor_size_0x10 = 0;

	//support instruction
	switch(data[1]){
		case 0x01:
		case 0x02:
		case 0x03:
		case 0x04:
		case 0x05:
			rx_size = 8;
			break;

		case 0x10:

			//data[6] --- size(byte)
			rx_size = 7 + data[6] + 2;
			resistor_size_0x10 = data[4]<<8 | data[5];

			if(data[6] == 0 || resistor_size_0x10 == 0)
				return 0;

			switch(resistor_size_0x10){
				case 3:
					return 0;

				case 1:
					if(data[6] > 2)
						return 0;
					else
						break;

				case 2:
				case 4:
					if(data[6] != resistor_size_0x10*2)
						return 0;
					else
						break;

				default:
					return 0;
			}

			break;
		default:
			return 0;
	}

	volatile uint16_t calc_crc = MODBUS_CRC(data, rx_size-2);
	volatile uint16_t rx_crc = data[rx_size-1]<<8 | data[rx_size-2];
	if(calc_crc != rx_crc)
		return 0;
	else
		return rx_size;
}


//不正データなどのエラーメッセージを作る関数
//メンテナンスモード以外のときにもここを使う
//
static void MODBUS_Response_Error_Message(uint8_t *original_array, ERROR_CODE error_code, uint8_t *response_array, uint8_t *response_array_size){
	*response_array_size = 5; //id, cmd, exception_code, crc16 (fixed)
	response_array[0] = original_array[0];
	response_array[1] = original_array[1] | 0x80;//instruction code | 0x80
	response_array[2] = error_code;

	if(error_code){
		MODBUS_Set_ErrorCode(error_code);
	}
}

//
//
////read
//static void MODBUS_Branch_Process_0x01(uint8_t *original_array, uint8_t size, uint8_t *response_array, uint8_t *response_array_size){
//
//	MODBUS_ADDRESS_0x01 address = original_array[2]<<8 | original_array[3];
//	ERROR_CODE error_code = 0;
//
//	switch(address){
//		case MA_COMPARISON1_OUTPUT_STATE:
//			response_array[3] = global_struct->ram.comparison1.output_transistor_state & 0b1;
//			break;
//		case MA_COMPARISON2_OUTPUT_STATE:
//			response_array[3] = global_struct->ram.comparison2.output_transistor_state & 0b1;
//			break;
//
//		default:
//			error_code = EC_ADDRESS_ERROR;
//			break;
//	}
//
//
//	if(error_code){
//		MODBUS_Response_Error_Message(original_array, error_code, response_array, response_array_size);
//	}
//	else{
//
//		*response_array_size = 6; //id, cmd, data_size, [data], crc16 (fixed)
//		response_array[0] = original_array[0];
//		response_array[1] = original_array[1];//instruction code
//		response_array[2] = 1;//1byte
//	}
//
//	uint16_t crc = MODBUS_CRC(response_array, *response_array_size-2);
//	response_array[*response_array_size-2] = (crc & 0x00ff);
//	response_array[*response_array_size-1] = (crc & 0xff00)>>8;
//	return;
//
//}
//
////read
//static void MODBUS_Branch_Process_0x02(uint8_t *original_array, uint8_t size, uint8_t *response_array, uint8_t *response_array_size){
//
//	MODBUS_ADDRESS_0x02 address = original_array[2]<<8 | original_array[3];
//	ERROR_CODE error_code = 0;
//
//	if(global_struct->eeprom.system.information.maintenance_mode){
//		switch(address){
//			case MA_BUTTON_STATUS:
//				response_array[3] = global_struct->ram.button.now.mode | global_struct->ram.button.now.up << 1 | global_struct->ram.button.now.down << 2 | global_struct->ram.button.now.ent << 3;
//				break;
//
//			default:
//				error_code = EC_ADDRESS_ERROR;
//				return;
//		}
//	}
//	else{
//		error_code = EC_ADDRESS_ERROR;
//	}
//
//	if(error_code){
//		MODBUS_Response_Error_Message(original_array, error_code, response_array, response_array_size);
//	}
//	else{
//		*response_array_size = 6; //id, cmd, data_size, [data], crc16 (fixed)
//		response_array[0] = original_array[0];
//		response_array[1] = original_array[1];//instruction code
//		response_array[2] = 1;//1byte
//	}
//
//	uint16_t crc = MODBUS_CRC(response_array, *response_array_size-2);
//	response_array[*response_array_size-2] = (crc & 0x00ff);
//	response_array[*response_array_size-1] = (crc & 0xff00)>>8;
//	return;
//}

//圧力値、ADC取得など
static void MODBUS_Branch_Process_0x04(uint8_t *original_array, uint8_t size, uint8_t *response_array, uint8_t *response_array_size){
	MODBUS_ADDRESS_0x04 address = original_array[2]<<8 | original_array[3];

	uint8_t data_size = 0;
	uint32_t data_acc = 0;

	ERROR_CODE error_code = 0;

	switch(address){
		case MA_TEMPERATURE_REFERENCE:
			data_size = 2;
			data_acc = Analog_Get_Temperature(TEMPERATURE_CHANNEL_REFERENCE);
			break;

		case MA_TEMPERATURE_CH1:
		case MA_TEMPERATURE_CH2:
		case MA_TEMPERATURE_CH3:
		case MA_TEMPERATURE_CH4:
			data_size = 2;
			data_acc = Analog_Get_Temperature(TEMPERATURE_CHANNEL_CH4 + (address - MA_TEMPERATURE_CH1));
			break;

		case MA_ERROR_CODE://error code
			data_size = sizeof(error_code);//sizeof(global_struct->ram.status_and_flags.error_code);
			data_acc = error_code;
			MODBUS_Clear_ErrorCode();
			break;


		default:
			error_code = ERROR;//address error
			break;

	}



	if(error_code){
		MODBUS_Response_Error_Message(original_array, error_code, response_array, response_array_size);
	}
	else{

		uint8_t data_array[4] = {0};
		for(uint8_t i = 0; i < data_size; i++){
			uint8_t shift_count = 8 * (data_size-i-1);
			data_array[i] = (data_acc>>shift_count) & 0xff;
		}

		*response_array_size = 3 + data_size + 2; //id, cmd, size, [data], crc16

		response_array[0] = slave_address;
		response_array[1] = original_array[1];//instruction code
		response_array[2] = data_size;

		for(uint8_t i = 0; i < data_size; i++){
			response_array[3+i] = data_array[i];
		}
	}
	uint16_t crc = MODBUS_CRC(response_array, *response_array_size-2);
	response_array[*response_array_size-2] = (crc & 0x00ff);
	response_array[*response_array_size-1] = (crc & 0xff00)>>8;
	return;
}
//
//// ゼロセットなど
//static void MODBUS_Branch_Process_0x05(uint8_t *original_array, uint8_t size, uint8_t *response_array, uint8_t *response_array_size){
//
//	MODBUS_ADDRESS_0x05 address = original_array[2]<<8 | original_array[3];
//
//	ERROR_CODE error_code = EC_NO_ERROR;
//
//	uint8_t b = (original_array[4] | original_array[5]) & 0b1;
//
//
//	switch(address){
//		case MA_ZERO_ADJUSTMENT:
//			global_struct->ram.status_and_flags.zero_adjustment_flag = b;
//			break;
//		case MA_CLEAR_MAX_MIN:
//			global_struct->ram.status_and_flags.clear_max_min_flag = b;
//			break;
//		case MA_CLEAR_MAX:
//			global_struct->ram.status_and_flags.clear_max_flag = b;
//			break;
//		case MA_CLEAR_MIN:
//			global_struct->ram.status_and_flags.clear_min_flag = b;
//			break;
//		case MA_CLEAR_ERROR:
//			global_struct->ram.status_and_flags.clear_error_flag = b;
//			break;
//
//		case MA_REBOOT:
//			global_struct->ram.status_and_flags.reboot_flag = b;
//			if(global_struct->ram.status_and_flags.reboot_flag){
//				global_struct->ram.status_and_flags.reboot_flag = 0;
//				MODBUS_Reboot();
//			}
//			break;
//
//		case MA_FACTORY_RESET:
//			global_struct->ram.status_and_flags.factory_reset_flag = b;
//			break;
//
//		default:
//			if(global_struct->eeprom.system.information.maintenance_mode){
//				switch(address){
//					//9001~
//					case MA_BACKUP_PAUSE:
//						global_struct->eeprom.backup_paused = b;
//						break;
//
//					case MA_BACKUP_FORCED_START:
//						global_struct->eeprom.backup_request_all = b;
//						global_struct->eeprom.backup_paused = 0;
//						break;
//
//					case MA_DISPLAY_TEST_MODE:
//						global_struct->ram.display_test_mode.enabled = b;
//						break;
//
//					case MA_OSC_ACTIVATED:
//						global_struct->ram.pressure.osc_activated = b;
//						break;
//
//					default:
//						error_code = EC_ADDRESS_ERROR;
//						break;
//				}
//			}
//			else{
//				error_code = EC_ADDRESS_ERROR;//address error
//			}
//			break;
//	}
//
//	if(error_code){
//		MODBUS_Response_Error_Message(original_array, error_code, response_array, response_array_size);
//	}
//	else{
//		for(uint8_t i = 0; i < size; i++){
//			response_array[i] = original_array[i];
//		}
//		*response_array_size = 8; //id, cmd, start_address, address_count, data_size, [data], crc16
//	}
//	uint16_t crc = MODBUS_CRC(response_array, *response_array_size-2);
//	response_array[*response_array_size-2] = (crc & 0x00ff);
//	response_array[*response_array_size-1] = (crc & 0xff00)>>8;
//	return;
//}
//


//EEPROM READ
static void MODBUS_Branch_Process_0x03(uint8_t *original_array, uint8_t size, uint8_t *response_array, uint8_t *response_array_size){
	MODBUS_ADDRESS_0x03_0x10 address = original_array[2]<<8 | original_array[3];

	uint8_t data_size = 2;
	uint32_t data_acc;
	ERROR_CODE error_code = NONE;

	switch(address){

		case MA_SLAVE_ADDRESS:
			data_size = 2;
			data_acc = slave_address;
			break;

		case MA_BAUDRATE:
			data_size = 2;
			data_acc = baudrate;
			break;

		case MA_TEMPERATURE_CORRECTION_00:
		case MA_TEMPERATURE_CORRECTION_01:
		case MA_TEMPERATURE_CORRECTION_02:
		case MA_TEMPERATURE_CORRECTION_03:
		case MA_TEMPERATURE_CORRECTION_04:
			data_size = 2;
			data_acc = temperature_correction[address - MA_TEMPERATURE_CORRECTION_00];
			break;

		default:
			error_code = ERROR;
			break;
	}


	if(error_code){
		MODBUS_Response_Error_Message(original_array, error_code, response_array, response_array_size);
	}
	else{
		if(data_size < 2){
			data_size = 2;
		}
		uint8_t data_array[4] = {0};
		for(uint8_t i = 0; i < data_size; i++){
			uint8_t shift_count = 8 * (data_size-i-1);
			data_array[i] = (data_acc>>shift_count) & 0xff;
		}

		*response_array_size = 3 + data_size + 2; //id, cmd, data_size, [data], crc16

		response_array[0] = original_array[0];
		response_array[1] = original_array[1];//instruction code
		response_array[2] = data_size;

		for(uint8_t i = 0; i < data_size; i++){
			response_array[3+i] = data_array[i];
		}
	}
	uint16_t crc = MODBUS_CRC(response_array, *response_array_size-2);
	response_array[*response_array_size-2] = (crc & 0x00ff);
	response_array[*response_array_size-1] = (crc & 0xff00)>>8;
	return;
}

static void MODBUS_Branch_Process_0x10(uint8_t *original_array, uint8_t size, uint8_t *response_array, uint8_t *response_array_size){

	MODBUS_ADDRESS_0x03_0x10 address = original_array[2]<<8 | original_array[3];

	uint8_t data_size = original_array[6];
	ERROR_CODE error_code = NONE;
	int32_t data_acc = 0;

	int32_t limit_bottom, limit_top;

	for (uint8_t i = 0; i < data_size; i++) {
		data_acc |= (original_array[7 + i] << ((data_size - 1) * 8 - 8 * i));
	}


	switch(address){
		//EEPROM
		case MA_SLAVE_ADDRESS:
			limit_top = 0xff;
			limit_bottom = 0;
			if(limit_bottom <= data_acc && data_acc <= limit_top){
				slave_address = data_acc;
			}
			else{
				error_code = ERROR;
			}
			break;


		case MA_BAUDRATE:
			limit_top = 12;
			limit_bottom = 0;
			if(limit_bottom <= data_acc && data_acc <= limit_top){
				baudrate = data_acc;
			}
			else{
				error_code = ERROR;
			}
			break;


		case MA_TEMPERATURE_CORRECTION_00:
		case MA_TEMPERATURE_CORRECTION_01:
		case MA_TEMPERATURE_CORRECTION_02:
		case MA_TEMPERATURE_CORRECTION_03:
		case MA_TEMPERATURE_CORRECTION_04:

			limit_top = 0xffff;
			limit_bottom = 0;
			if(limit_bottom <= data_acc && data_acc <= limit_top){
				temperature_correction[address - MA_TEMPERATURE_CORRECTION_00] = data_acc;
			}
			else{
				error_code = ERROR;
			}
			break;


		default:
			error_code = ERROR;
			break;
	}




	if(error_code){
		MODBUS_Response_Error_Message(original_array, error_code, response_array, response_array_size);
	}
	else{
		for(uint8_t i = 0; i < 6; i++){
			response_array[i] = original_array[i];
		}


		uint16_t crc = MODBUS_CRC(response_array, 6);
		response_array[6] = crc & 0xff;
		response_array[7] = (crc>>8) & 0xff;
		*response_array_size = 8; //id, cmd, start_address_H, start_address_L, address_count_H, address_count_H, crc16
	}
	return;
}


void MODBUS_Branch_Process(uint8_t *original_array, uint8_t size, uint8_t *response_array, uint8_t *response_array_size){

	switch(original_array[1]){//instruction
		case 0x01:
			break;
		case 0x02:
			break;
		case 0x03:
			MODBUS_Branch_Process_0x03(original_array, size, response_array, response_array_size);
			break;
		case 0x04:
			MODBUS_Branch_Process_0x04(original_array, size, response_array, response_array_size);
			break;
		case 0x05:
			break;
		case 0x10:
			MODBUS_Branch_Process_0x10(original_array, size, response_array, response_array_size);
			break;

		default:
			*response_array_size = 0;
			break;
	}

}
void MODBUS_Init(uint8_t _slave_address, uint8_t _baudrate){
	slave_address = _slave_address;
	baudrate = _baudrate;
}
