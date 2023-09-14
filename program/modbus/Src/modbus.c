/*
 * modbus.c
 *
 *  Created on: 2021/10/26
 *      Author: staff
 */


#include <status.h>
#include "global_typedef.h"
#include "modbus.h"
#include "usart.h"
#include "communication.h"
#include "analog.h"

static Global_TypeDef *global_struct;


static void MODBUS_Reboot(){
	HAL_NVIC_SystemReset();
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
	if((data[0] != 255) && (data[0] != (uint8_t)global_struct->eeprom.user.communication.slave_address)){
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
		Status_Set_ErrorCode(error_code);
		Status_Set_StatusCode(SC_MODBUS_ERROR);
	}
}

static int16_t MODBUS_Percentage_To_Pascal(int16_t percentage){
	int32_t fullscale = global_struct->eeprom.system.information.fullscale_plus - global_struct->eeprom.system.information.fullscale_minus;
	if(global_struct->eeprom.system.information.scale_type == 2){
		fullscale = global_struct->eeprom.system.information.fullscale_minus;
	}
	int32_t pascal = (fullscale * percentage) / 10000;
	return (int16_t)pascal;
}

static int16_t MODBUS_Pascal_To_Percentage(int16_t pascal){
	int32_t fullscale = global_struct->eeprom.system.information.fullscale_plus - global_struct->eeprom.system.information.fullscale_minus;
	if(global_struct->eeprom.system.information.scale_type == 2){
		fullscale = global_struct->eeprom.system.information.fullscale_minus;
	}
	int32_t percentage = ((int32_t)pascal * 10000) / fullscale;
	return (int16_t)percentage;
}

static uint8_t MODBUS_Check_Percentage(int16_t percentage){
	/*
	 * 	NO ERROR : 0
	 * 	ERROR : 1
	 */
	uint8_t error_flag = 1;
	switch(global_struct->eeprom.system.information.scale_type){
		//片圧
		case 0:
			if(percentage < 0 || 10000 < percentage){
				error_flag = 1;
			}
			else{
				error_flag = 0;
			}
			break;

		case 1:
			if(percentage < -5000 || 5000 < percentage){
				error_flag = 1;
			}
			else{
				error_flag = 0;
			}
			break;

		case 2:
			if(percentage < -10000 || 0 < percentage){
				error_flag = 1;
			}
			else{
				error_flag = 0;
			}
			break;

		default:
			break;
	}
	return error_flag;
}

//read
static void MODBUS_Branch_Process_0x01(uint8_t *original_array, uint8_t size, uint8_t *response_array, uint8_t *response_array_size){

	MODBUS_ADDRESS_0x01 address = original_array[2]<<8 | original_array[3];
	ERROR_CODE error_code = 0;

	switch(address){
		case MA_COMPARISON1_OUTPUT_STATE:
			response_array[3] = global_struct->ram.comparison1.output_transistor_state & 0b1;
			break;
		case MA_COMPARISON2_OUTPUT_STATE:
			response_array[3] = global_struct->ram.comparison2.output_transistor_state & 0b1;
			break;

		default:
			error_code = EC_ADDRESS_ERROR;
			break;
	}


	if(error_code){
		MODBUS_Response_Error_Message(original_array, error_code, response_array, response_array_size);
	}
	else{

		*response_array_size = 6; //id, cmd, data_size, [data], crc16 (fixed)
		response_array[0] = original_array[0];
		response_array[1] = original_array[1];//instruction code
		response_array[2] = 1;//1byte
	}

	uint16_t crc = MODBUS_CRC(response_array, *response_array_size-2);
	response_array[*response_array_size-2] = (crc & 0x00ff);
	response_array[*response_array_size-1] = (crc & 0xff00)>>8;
	return;

}

//read
static void MODBUS_Branch_Process_0x02(uint8_t *original_array, uint8_t size, uint8_t *response_array, uint8_t *response_array_size){

	MODBUS_ADDRESS_0x02 address = original_array[2]<<8 | original_array[3];
	ERROR_CODE error_code = 0;

	if(global_struct->eeprom.system.information.maintenance_mode){
		switch(address){
			case MA_BUTTON_STATUS:
				response_array[3] = global_struct->ram.button.now.mode | global_struct->ram.button.now.up << 1 | global_struct->ram.button.now.down << 2 | global_struct->ram.button.now.ent << 3;
				break;

			default:
				error_code = EC_ADDRESS_ERROR;
				return;
		}
	}
	else{
		error_code = EC_ADDRESS_ERROR;
	}

	if(error_code){
		MODBUS_Response_Error_Message(original_array, error_code, response_array, response_array_size);
	}
	else{
		*response_array_size = 6; //id, cmd, data_size, [data], crc16 (fixed)
		response_array[0] = original_array[0];
		response_array[1] = original_array[1];//instruction code
		response_array[2] = 1;//1byte
	}

	uint16_t crc = MODBUS_CRC(response_array, *response_array_size-2);
	response_array[*response_array_size-2] = (crc & 0x00ff);
	response_array[*response_array_size-1] = (crc & 0xff00)>>8;
	return;
}

//圧力値、ADC取得など
static void MODBUS_Branch_Process_0x04(uint8_t *original_array, uint8_t size, uint8_t *response_array, uint8_t *response_array_size){
	MODBUS_ADDRESS_0x04 address = original_array[2]<<8 | original_array[3];

	uint8_t data_size = 0;
	uint32_t data_acc = 0;

	ERROR_CODE error_code = 0;

	switch(address){
		case MA_PRESSURE://pressure
			data_size = sizeof(global_struct->ram.pressure.pascal_output);
			data_acc = global_struct->ram.pressure.pascal_output;
			break;

		case MA_PRESSURE_PERCENTAGE://percentage
			data_size = sizeof(global_struct->ram.pressure.percentage_output_adjusted);
			data_acc = global_struct->ram.pressure.percentage_output_adjusted;
			break;

		case MA_PRESSURE_MAX://pressure max
			data_size = sizeof(global_struct->ram.pressure.pressure_max);
			data_acc = global_struct->ram.pressure.pressure_max;
			break;

		case MA_PRESSURE_MIN://pressure min
			data_size = sizeof(global_struct->ram.pressure.pressure_min);
			data_acc = global_struct->ram.pressure.pressure_min;
			break;

		case MA_ERROR_CODE://error code
			data_size = 2;//sizeof(global_struct->ram.status_and_flags.error_code);
			data_acc = global_struct->ram.status_and_flags.error_code;
			Status_Clear_ErrorCode();
			break;

		case MA_STATUS_CODE://status code
			data_size = 2;//sizeof(global_struct->ram.status_and_flags.status_code);
			data_acc = global_struct->ram.status_and_flags.status_code;
			Status_Clear_StatusCode();
			break;

		default:
			if(global_struct->eeprom.system.information.maintenance_mode){
				switch(address){
					case MA_SENSOR_ADC://ADC
						data_size = sizeof(global_struct->ram.pressure.adc_output);
						data_acc = global_struct->ram.pressure.adc_output;
						break;

					case MA_TEMPERATURE://temperature
						data_size = sizeof(global_struct->ram.temperature.temperature);
						data_acc = global_struct->ram.temperature.temperature;
						break;

					case MA_VDD://vdd
						data_size = sizeof(global_struct->ram.temperature.vdd);
						data_acc = global_struct->ram.temperature.vdd;
						break;

					default:
						error_code = EC_ADDRESS_ERROR;//address error
						break;
				}
			}
			else{
				error_code = EC_ADDRESS_ERROR;//address error
			}
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

		response_array[0] = (uint8_t)global_struct->eeprom.user.communication.slave_address;
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

// ゼロセットなど
static void MODBUS_Branch_Process_0x05(uint8_t *original_array, uint8_t size, uint8_t *response_array, uint8_t *response_array_size){

	MODBUS_ADDRESS_0x05 address = original_array[2]<<8 | original_array[3];

	ERROR_CODE error_code = EC_NO_ERROR;

	uint8_t b = (original_array[4] | original_array[5]) & 0b1;


	switch(address){
		case MA_ZERO_ADJUSTMENT:
			global_struct->ram.status_and_flags.zero_adjustment_flag = b;
			break;
		case MA_CLEAR_MAX_MIN:
			global_struct->ram.status_and_flags.clear_max_min_flag = b;
			break;
		case MA_CLEAR_MAX:
			global_struct->ram.status_and_flags.clear_max_flag = b;
			break;
		case MA_CLEAR_MIN:
			global_struct->ram.status_and_flags.clear_min_flag = b;
			break;
		case MA_CLEAR_ERROR:
			global_struct->ram.status_and_flags.clear_error_flag = b;
			break;

		case MA_REBOOT:
			global_struct->ram.status_and_flags.reboot_flag = b;
			if(global_struct->ram.status_and_flags.reboot_flag){
				global_struct->ram.status_and_flags.reboot_flag = 0;
				MODBUS_Reboot();
			}
			break;

		case MA_FACTORY_RESET:
			global_struct->ram.status_and_flags.factory_reset_flag = b;
			break;

		default:
			if(global_struct->eeprom.system.information.maintenance_mode){
				switch(address){
					//9001~
					case MA_BACKUP_PAUSE:
						global_struct->eeprom.backup_paused = b;
						break;

					case MA_BACKUP_FORCED_START:
						global_struct->eeprom.backup_request_all = b;
						global_struct->eeprom.backup_paused = 0;
						break;

					case MA_DISPLAY_TEST_MODE:
						global_struct->ram.display_test_mode.enabled = b;
						break;

					case MA_OSC_ACTIVATED:
						global_struct->ram.pressure.osc_activated = b;
						break;

					default:
						error_code = EC_ADDRESS_ERROR;
						break;
				}
			}
			else{
				error_code = EC_ADDRESS_ERROR;//address error
			}
			break;
	}

	if(error_code){
		MODBUS_Response_Error_Message(original_array, error_code, response_array, response_array_size);
	}
	else{
		for(uint8_t i = 0; i < size; i++){
			response_array[i] = original_array[i];
		}
		*response_array_size = 8; //id, cmd, start_address, address_count, data_size, [data], crc16
	}
	uint16_t crc = MODBUS_CRC(response_array, *response_array_size-2);
	response_array[*response_array_size-2] = (crc & 0x00ff);
	response_array[*response_array_size-1] = (crc & 0xff00)>>8;
	return;
}



//EEPROM READ
static void MODBUS_Branch_Process_0x03(uint8_t *original_array, uint8_t size, uint8_t *response_array, uint8_t *response_array_size){
	MODBUS_ADDRESS_0x03 address = original_array[2]<<8 | original_array[3];

	uint8_t data_size = 2;
	uint32_t data_acc;
	ERROR_CODE error_code = EC_NO_ERROR;

	switch(address){
		case MA_USER_MODE_ECO_MODE:
			data_size = sizeof(global_struct->eeprom.user.mode.eco_mode);
			data_acc = global_struct->eeprom.user.mode.eco_mode;
			break;

		case MA_USER_MODE_PROTECTED:
			data_size = sizeof(global_struct->eeprom.user.mode.protected);
			data_acc = global_struct->eeprom.user.mode.protected;
			break;

		case MA_USER_MODE_PRODUCTION_NUMBER:
			data_size = sizeof(*(global_struct->eeprom.user.user_information.production_number));
			data_acc = *global_struct->eeprom.user.user_information.production_number;
			break;

		case MA_USER_MODE_PRESSURE_RANGE_ID:
			data_size = sizeof(*(global_struct->eeprom.user.user_information.pressure_range_id));
			data_acc = *global_struct->eeprom.user.user_information.pressure_range_id;
			break;

		case MA_USER_DISPLAY_LOW_CUT:
			data_size = sizeof(global_struct->eeprom.user.display.low_cut);
			data_acc = global_struct->eeprom.user.display.low_cut;
			break;


		case MA_USER_DISPLAY_SIGN_INVERSION:
			data_size = sizeof(global_struct->eeprom.user.display.sign_inversion);
			data_acc = global_struct->eeprom.user.display.sign_inversion;
			break;

		case MA_USER_DISPLAY_AVERAGE_TIME:
			data_size = sizeof(global_struct->eeprom.user.display.average_time);
			data_acc = global_struct->eeprom.user.display.average_time;
			break;

		case MA_USER_OUTPUT_OUTPUT_ENABLED:
			data_size = sizeof(global_struct->eeprom.user.output.output_enabled);
			data_acc = global_struct->eeprom.user.output.output_enabled;
			break;

		case MA_USER_OUTPUT_AVERAGE_TIME:
			data_size = sizeof(global_struct->eeprom.user.output.average_time);
			data_acc = global_struct->eeprom.user.output.average_time;
			break;

		case MA_USER_OUTPUT_OUTPUT_INVERSION:
			data_size = sizeof(global_struct->eeprom.user.output.output_inversion);
			data_acc = global_struct->eeprom.user.output.output_inversion;
			break;

		case MA_USER_COMMUNICATION_MESSAGE_PROTOCOL:
			data_size = sizeof(global_struct->eeprom.user.communication.message_protocol);
			data_acc = global_struct->eeprom.user.communication.message_protocol;
			break;

		case MA_USER_COMMUNICATION_SLAVE_ADDRESS:
			data_size = sizeof(global_struct->eeprom.user.communication.slave_address);
			data_acc = global_struct->eeprom.user.communication.slave_address;
			break;

		case MA_USER_COMMUNICATION_BAUD_RATE:
			data_size = sizeof(global_struct->eeprom.user.communication.baud_rate);
			data_acc = global_struct->eeprom.user.communication.baud_rate;
			break;

		case MA_USER_COMMUNICATION_PARITY:
			data_size = sizeof(global_struct->eeprom.user.communication.parity);
			data_acc = global_struct->eeprom.user.communication.parity;
			break;

		case MA_USER_COMMUNICATION_STOP_BIT:
			data_size = sizeof(global_struct->eeprom.user.communication.stop_bit);
			data_acc = global_struct->eeprom.user.communication.stop_bit;
			break;

		case MA_USER_COMPARISON1_POWER_ON_DELAY:
			data_size = sizeof(global_struct->eeprom.user.comparison1.power_on_delay);
			data_acc = global_struct->eeprom.user.comparison1.power_on_delay;
			break;

		case MA_USER_COMPARISON1_OUTPUT_TYPE:
			data_size = sizeof(global_struct->eeprom.user.comparison1.output_type);
			data_acc = global_struct->eeprom.user.comparison1.output_type;
			break;

		case MA_USER_COMPARISON1_OFF_DELAY:
			data_size = sizeof(global_struct->eeprom.user.comparison1.off_delay);
			data_acc = global_struct->eeprom.user.comparison1.off_delay;
			break;

		case MA_USER_COMPARISON1_ON_DELAY:
			data_size = sizeof(global_struct->eeprom.user.comparison1.on_delay);
			data_acc = global_struct->eeprom.user.comparison1.on_delay;
			break;

		case MA_USER_COMPARISON1_MODE:
			data_size = sizeof(global_struct->eeprom.user.comparison1.mode);
			data_acc = global_struct->eeprom.user.comparison1.mode;
			break;

		case MA_USER_COMPARISON1_ENABLED:
			data_size = sizeof(global_struct->eeprom.user.comparison1.enabled);
			data_acc = global_struct->eeprom.user.comparison1.enabled;
			break;

		case MA_USER_COMPARISON1_HYS_P1:
			data_size = sizeof(global_struct->eeprom.user.comparison1.hys_p1);
			data_acc = MODBUS_Percentage_To_Pascal(global_struct->eeprom.user.comparison1.hys_p1);
			break;

		case MA_USER_COMPARISON1_HYS_P2:
			data_size = sizeof(global_struct->eeprom.user.comparison1.hys_p2);
			data_acc = MODBUS_Percentage_To_Pascal(global_struct->eeprom.user.comparison1.hys_p2);
			break;

		case MA_USER_COMPARISON1_WIN_HI:
			data_size = sizeof(global_struct->eeprom.user.comparison1.win_hi);
			data_acc = MODBUS_Percentage_To_Pascal(global_struct->eeprom.user.comparison1.win_hi);
			break;

		case MA_USER_COMPARISON1_WIN_LOW:
			data_size = sizeof(global_struct->eeprom.user.comparison1.win_low);
			data_acc = MODBUS_Percentage_To_Pascal(global_struct->eeprom.user.comparison1.win_low);
			break;

		case MA_USER_COMPARISON2_POWER_ON_DELAY:
			data_size = sizeof(global_struct->eeprom.user.comparison2.power_on_delay);
			data_acc = global_struct->eeprom.user.comparison2.power_on_delay;
			break;

		case MA_USER_COMPARISON2_OUTPUT_TYPE:
			data_size = sizeof(global_struct->eeprom.user.comparison2.output_type);
			data_acc = global_struct->eeprom.user.comparison2.output_type;
			break;

		case MA_USER_COMPARISON2_OFF_DELAY:
			data_size = sizeof(global_struct->eeprom.user.comparison2.off_delay);
			data_acc = global_struct->eeprom.user.comparison2.off_delay;
			break;

		case MA_USER_COMPARISON2_ON_DELAY:
			data_size = sizeof(global_struct->eeprom.user.comparison2.on_delay);
			data_acc = global_struct->eeprom.user.comparison2.on_delay;
			break;

		case MA_USER_COMPARISON2_MODE:
			data_size = sizeof(global_struct->eeprom.user.comparison2.mode);
			data_acc = global_struct->eeprom.user.comparison2.mode;
			break;

		case MA_USER_COMPARISON2_ENABLED:
			data_size = sizeof(global_struct->eeprom.user.comparison2.enabled);
			data_acc = global_struct->eeprom.user.comparison2.enabled;
			break;

		case MA_USER_COMPARISON2_HYS_P1:
			data_size = sizeof(global_struct->eeprom.user.comparison2.hys_p1);
			data_acc = MODBUS_Percentage_To_Pascal(global_struct->eeprom.user.comparison2.hys_p1);
			break;

		case MA_USER_COMPARISON2_HYS_P2:
			data_size = sizeof(global_struct->eeprom.user.comparison2.hys_p2);
			data_acc = MODBUS_Percentage_To_Pascal(global_struct->eeprom.user.comparison2.hys_p2);
			break;

		case MA_USER_COMPARISON2_WIN_HI:
			data_size = sizeof(global_struct->eeprom.user.comparison2.win_hi);
			data_acc = MODBUS_Percentage_To_Pascal(global_struct->eeprom.user.comparison2.win_hi);
			break;

		case MA_USER_COMPARISON2_WIN_LOW:
			data_size = sizeof(global_struct->eeprom.user.comparison2.win_low);
			data_acc = MODBUS_Percentage_To_Pascal(global_struct->eeprom.user.comparison2.win_low);
			break;

		//maintenance mode
		case MA_SYSTEM_INFORMATION_MAINTENANCE_MODE:
			data_size = sizeof(global_struct->eeprom.system.information.maintenance_mode);
			data_acc = global_struct->eeprom.system.information.maintenance_mode;
			break;

		default:
			if(global_struct->eeprom.system.information.maintenance_mode){
				switch(address){

					case MA_USER_DISPLAY_BRIGHTNESS:
						data_size = sizeof(global_struct->eeprom.user.display.brightness);
						data_acc = global_struct->eeprom.user.display.brightness;
						break;

					case MA_SENSOR_ZERO_ADJUSTMENT_COUNT:
						data_size = sizeof(global_struct->eeprom.sensor.zero_adjustment.count);
						data_acc = global_struct->eeprom.sensor.zero_adjustment.count;
						break;

					case MA_SENSOR_ZERO_ADJUSTMENT_DISPLAY:
						data_size = sizeof(global_struct->eeprom.sensor.zero_adjustment.display);
						data_acc = global_struct->eeprom.sensor.zero_adjustment.display;
						break;

					case MA_SENSOR_ZERO_ADJUSTMENT_OUTPUT:
						data_size = sizeof(global_struct->eeprom.sensor.zero_adjustment.output);
						data_acc = global_struct->eeprom.sensor.zero_adjustment.output;
						break;

					case MA_SYSTEM_INFORMATION_SETTING_STATUS_FLAG:
						data_size = sizeof(global_struct->eeprom.system.information.setting_status_flag);
						data_acc = global_struct->eeprom.system.information.setting_status_flag;
						break;

					case MA_SYSTEM_INFORMATION_SOFTWARE_VERSION:
						data_size = sizeof(global_struct->eeprom.system.information.software_version);
						data_acc = global_struct->eeprom.system.information.software_version;
						break;

					case MA_SYSTEM_INFORMATION_HARDWARE_VERSION:
						data_size = sizeof(global_struct->eeprom.system.information.hardware_version);
						data_acc = global_struct->eeprom.system.information.hardware_version;
						break;

					case MA_SYSTEM_INFORMATION_MECHANISM_VERSION:
						data_size = sizeof(global_struct->eeprom.system.information.mechanism_version);
						data_acc = global_struct->eeprom.system.information.mechanism_version;
						break;

					case MA_SYSTEM_INFORMATION_PRODUCTION_NUMBER:
						data_size = sizeof(global_struct->eeprom.system.information.production_number);
						data_acc = global_struct->eeprom.system.information.production_number;
						break;

					case MA_SYSTEM_INFORMATION_CUSTOMER_CODE:
						data_size = sizeof(global_struct->eeprom.system.information.customer_code);
						data_acc = global_struct->eeprom.system.information.customer_code;
						break;

					case MA_SYSTEM_INFORMATION_PRESSURE_RANGE_ID:
						data_size = sizeof(global_struct->eeprom.system.information.pressure_range_id);
						data_acc = global_struct->eeprom.system.information.pressure_range_id;
						break;

					case MA_SYSTEM_INFORMATION_POINT_POSITION:
						data_size = sizeof(global_struct->eeprom.system.information.point_position);
						data_acc = global_struct->eeprom.system.information.point_position;
						break;

					case MA_SYSTEM_INFORMATION_SCALE_TYPE:
						data_size = sizeof(global_struct->eeprom.system.information.scale_type);
						data_acc = global_struct->eeprom.system.information.scale_type;
						break;

					case MA_SYSTEM_INFORMATION_FULLSCALE_PLUS:
						data_size = sizeof(global_struct->eeprom.system.information.fullscale_plus);
						data_acc = global_struct->eeprom.system.information.fullscale_plus;
						break;

					case MA_SYSTEM_INFORMATION_FULLSCALE_MINUS:
						data_size = sizeof(global_struct->eeprom.system.information.fullscale_minus);
						data_acc = global_struct->eeprom.system.information.fullscale_minus;
						break;

					case MA_SYSTEM_INFORMATION_SCALE_MARGIN:
						data_size = sizeof(global_struct->eeprom.system.information.scale_margin);
						data_acc = global_struct->eeprom.system.information.scale_margin;
						break;

					case MA_SYSTEM_INFORMATION_OUTPUT_TYPE:
						data_size = sizeof(global_struct->eeprom.system.information.output_type);
						data_acc = global_struct->eeprom.system.information.output_type;
						break;

					case MA_SYSTEM_INFORMATION_OUTPUT_PHY_PROTOCOL:
						data_size = sizeof(global_struct->eeprom.system.information.output_phy_protocol);
						data_acc = global_struct->eeprom.system.information.output_phy_protocol;
						break;

					case MA_SYSTEM_INFORMATION_COMPARISON_COUNT:
						data_size = sizeof(global_struct->eeprom.system.information.comparison_count);
						data_acc = global_struct->eeprom.system.information.comparison_count;
						break;

					case MA_SYSTEM_INFORMATION_ANALOG_COUNT:
						data_size = sizeof(global_struct->eeprom.system.information.analog_count);
						data_acc = global_struct->eeprom.system.information.analog_count;
						break;

					case MA_SYSTEM_INFORMATION_DIGITAL_COUNT:
						data_size = sizeof(global_struct->eeprom.system.information.digital_count);
						data_acc = global_struct->eeprom.system.information.digital_count;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_ZERO_OFFSET_FINE:
						data_size = sizeof(global_struct->eeprom.system.correction_value.zero_offset_fine);
						data_acc = global_struct->eeprom.system.correction_value.zero_offset_fine;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_ZERO_OFFSET_ROUGH:
						data_size = sizeof(global_struct->eeprom.system.correction_value.zero_offset_rough);
						data_acc = global_struct->eeprom.system.correction_value.zero_offset_rough;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_CORRECTION_COUNT:
						data_size = sizeof(global_struct->eeprom.system.correction_value.correction_count);
						data_acc = global_struct->eeprom.system.correction_value.correction_count;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_INSPECTION_TEMPERATURE:
						data_size = sizeof(global_struct->eeprom.system.correction_value.inspection_temperature);
						data_acc = global_struct->eeprom.system.correction_value.inspection_temperature;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_INSPECTION_TEMPERATURE_OFFSET:
						data_size = sizeof(global_struct->eeprom.system.correction_value.inspection_temperature_offset);
						data_acc = global_struct->eeprom.system.correction_value.inspection_temperature_offset;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_REFERENCE:
						data_size = sizeof(global_struct->eeprom.system.correction_value.temperature_reference);
						data_acc = global_struct->eeprom.system.correction_value.temperature_reference;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_REFERENCE_ADC:
						data_size = sizeof(global_struct->eeprom.system.correction_value.temperature_reference_adc);
						data_acc = global_struct->eeprom.system.correction_value.temperature_reference_adc;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_ZERO:
						data_size = sizeof(global_struct->eeprom.system.correction_value.temperature_zero);
						data_acc = global_struct->eeprom.system.correction_value.temperature_zero;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_SPAN:
						data_size = sizeof(global_struct->eeprom.system.correction_value.temperature_span);
						data_acc = global_struct->eeprom.system.correction_value.temperature_span;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_COUNT:
						data_size = sizeof(global_struct->eeprom.system.correction_value.temperature_count);
						data_acc = global_struct->eeprom.system.correction_value.temperature_count;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_CORRECTION_00:
						data_size = sizeof(global_struct->eeprom.system.correction_value.correction[0]);
						data_acc = global_struct->eeprom.system.correction_value.correction[0];
						break;

					case MA_SYSTEM_CORRECTION_VALUE_CORRECTION_01:
						data_size = sizeof(global_struct->eeprom.system.correction_value.correction[1]);
						data_acc = global_struct->eeprom.system.correction_value.correction[1];
						break;

					case MA_SYSTEM_CORRECTION_VALUE_CORRECTION_02:
						data_size = sizeof(global_struct->eeprom.system.correction_value.correction[2]);
						data_acc = global_struct->eeprom.system.correction_value.correction[2];
						break;

					case MA_SYSTEM_CORRECTION_VALUE_CORRECTION_03:
						data_size = sizeof(global_struct->eeprom.system.correction_value.correction[3]);
						data_acc = global_struct->eeprom.system.correction_value.correction[3];
						break;

					case MA_SYSTEM_CORRECTION_VALUE_CORRECTION_04:
						data_size = sizeof(global_struct->eeprom.system.correction_value.correction[4]);
						data_acc = global_struct->eeprom.system.correction_value.correction[4];
						break;

					case MA_SYSTEM_CORRECTION_VALUE_CORRECTION_05:
						data_size = sizeof(global_struct->eeprom.system.correction_value.correction[5]);
						data_acc = global_struct->eeprom.system.correction_value.correction[5];
						break;

					case MA_SYSTEM_CORRECTION_VALUE_CORRECTION_06:
						data_size = sizeof(global_struct->eeprom.system.correction_value.correction[6]);
						data_acc = global_struct->eeprom.system.correction_value.correction[6];
						break;

					case MA_SYSTEM_CORRECTION_VALUE_CORRECTION_07:
						data_size = sizeof(global_struct->eeprom.system.correction_value.correction[7]);
						data_acc = global_struct->eeprom.system.correction_value.correction[7];
						break;

					case MA_SYSTEM_CORRECTION_VALUE_CORRECTION_08:
						data_size = sizeof(global_struct->eeprom.system.correction_value.correction[8]);
						data_acc = global_struct->eeprom.system.correction_value.correction[8];
						break;

					case MA_SYSTEM_CORRECTION_VALUE_CORRECTION_09:
						data_size = sizeof(global_struct->eeprom.system.correction_value.correction[9]);
						data_acc = global_struct->eeprom.system.correction_value.correction[9];
						break;

					case MA_SYSTEM_CORRECTION_VALUE_CORRECTION_10:
						data_size = sizeof(global_struct->eeprom.system.correction_value.correction[10]);
						data_acc = global_struct->eeprom.system.correction_value.correction[10];
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_00:
						data_size = sizeof(global_struct->eeprom.system.correction_value.temperature[0]);
						data_acc = global_struct->eeprom.system.correction_value.temperature[0];
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_01:
						data_size = sizeof(global_struct->eeprom.system.correction_value.temperature[1]);
						data_acc = global_struct->eeprom.system.correction_value.temperature[1];
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_02:
						data_size = sizeof(global_struct->eeprom.system.correction_value.temperature[2]);
						data_acc = global_struct->eeprom.system.correction_value.temperature[2];
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_03:
						data_size = sizeof(global_struct->eeprom.system.correction_value.temperature[3]);
						data_acc = global_struct->eeprom.system.correction_value.temperature[3];
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_04:
						data_size = sizeof(global_struct->eeprom.system.correction_value.temperature[4]);
						data_acc = global_struct->eeprom.system.correction_value.temperature[4];
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_05:
						data_size = sizeof(global_struct->eeprom.system.correction_value.temperature[5]);
						data_acc = global_struct->eeprom.system.correction_value.temperature[5];
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_06:
						data_size = sizeof(global_struct->eeprom.system.correction_value.temperature[6]);
						data_acc = global_struct->eeprom.system.correction_value.temperature[6];
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_07:
						data_size = sizeof(global_struct->eeprom.system.correction_value.temperature[7]);
						data_acc = global_struct->eeprom.system.correction_value.temperature[7];
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_08:
						data_size = sizeof(global_struct->eeprom.system.correction_value.temperature[8]);
						data_acc = global_struct->eeprom.system.correction_value.temperature[8];
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_09:
						data_size = sizeof(global_struct->eeprom.system.correction_value.temperature[9]);
						data_acc = global_struct->eeprom.system.correction_value.temperature[9];
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_10:
						data_size = sizeof(global_struct->eeprom.system.correction_value.temperature[10]);
						data_acc = global_struct->eeprom.system.correction_value.temperature[10];
						break;

					case MA_SYSTEM_CORRECTION_VALUE_DAC_MIN:
						data_size = sizeof(global_struct->eeprom.system.correction_value.dac_min);
						data_acc = global_struct->eeprom.system.correction_value.dac_min;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_DAC_MAX:
						data_size = sizeof(global_struct->eeprom.system.correction_value.dac_max);
						data_acc = global_struct->eeprom.system.correction_value.dac_max;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_DAC_CENTER:
						data_size = sizeof(global_struct->eeprom.system.correction_value.dac_center);
						data_acc = global_struct->eeprom.system.correction_value.dac_center;
						break;

					default:
						error_code = EC_ADDRESS_ERROR;
						break;
				}
			}
			else{
				error_code = EC_ADDRESS_ERROR;
			}
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

	MODBUS_ADDRESS_0x03 address = original_array[2]<<8 | original_array[3];

	uint8_t data_size = original_array[6];
	ERROR_CODE error_code = EC_NO_ERROR;
	int32_t data_acc = 0;

	int32_t limit_bottom, limit_top;

	for (uint8_t i = 0; i < data_size; i++) {
		data_acc |= (original_array[7 + i] << ((data_size - 1) * 8 - 8 * i));
	}


	switch(address){
		//EEPROM
		case MA_USER_MODE_ECO_MODE:
			limit_top = 1;
			limit_bottom = 0;
			if(limit_bottom <= data_acc && data_acc <= limit_top){
				global_struct->eeprom.user.mode.eco_mode = data_acc;
				global_struct->eeprom.user.backup_request = 1;
			}
			else{
				global_struct->eeprom.user.backup_request = 0;
				error_code = EC_DATA_ERROR;
			}
			break;

		case MA_USER_MODE_PROTECTED:
			limit_top = 1;
			limit_bottom = 0;
			if(limit_bottom <= data_acc && data_acc <= limit_top){
				global_struct->eeprom.user.mode.protected = data_acc;
				global_struct->eeprom.user.backup_request = 1;
			}
			else{
				global_struct->eeprom.user.backup_request = 0;
				error_code = EC_DATA_ERROR;
			}
			break;


		case MA_USER_DISPLAY_LOW_CUT:
			limit_top = 5;
			limit_bottom = 0;
			if(limit_bottom <= data_acc && data_acc <= limit_top){
				global_struct->eeprom.user.display.low_cut = data_acc;
				global_struct->eeprom.user.backup_request = 1;
			}
			else{
				global_struct->eeprom.user.backup_request = 0;
				error_code = EC_DATA_ERROR;
			}
			break;


		case MA_USER_DISPLAY_SIGN_INVERSION:
			limit_top = 1;
			limit_bottom = 0;
			if(limit_bottom <= data_acc && data_acc <= limit_top){
				global_struct->eeprom.user.display.sign_inversion = data_acc;
				global_struct->eeprom.user.backup_request = 1;
			}
			else{
				global_struct->eeprom.user.backup_request = 0;
				error_code = EC_DATA_ERROR;
			}
			break;

		case MA_USER_DISPLAY_AVERAGE_TIME:
			limit_top = 3;
			limit_bottom = 0;
			if(limit_bottom <= data_acc && data_acc <= limit_top){
				global_struct->eeprom.user.display.average_time = data_acc;
				Analog_Set_Average_Time_Display(data_acc);
				global_struct->eeprom.user.backup_request = 1;
			}
			else{
				global_struct->eeprom.user.backup_request = 0;
				error_code = EC_DATA_ERROR;
			}
			break;

		case MA_USER_OUTPUT_OUTPUT_ENABLED:
			limit_top = 1;
			limit_bottom = 0;
			if(limit_bottom <= data_acc && data_acc <= limit_top){
				global_struct->eeprom.user.output.output_enabled = data_acc;
				global_struct->eeprom.user.backup_request = 1;
			}
			else{
				global_struct->eeprom.user.backup_request = 0;
				error_code = EC_DATA_ERROR;
			}
			break;

		case MA_USER_OUTPUT_AVERAGE_TIME:
			limit_top = 3;
			limit_bottom = 0;
			if(limit_bottom <= data_acc && data_acc <= limit_top){
				global_struct->eeprom.user.output.average_time = data_acc;
				Analog_Set_Average_Time_Output(data_acc);
				global_struct->eeprom.user.backup_request = 1;
			}
			else{
				global_struct->eeprom.user.backup_request = 0;
				error_code = EC_DATA_ERROR;
			}
			break;

		case MA_USER_OUTPUT_OUTPUT_INVERSION:
			limit_top = 1;
			limit_bottom = 0;
			if(limit_bottom <= data_acc && data_acc <= limit_top){
				global_struct->eeprom.user.output.output_inversion = data_acc;
				global_struct->eeprom.user.backup_request = 1;
			}
			else{
				global_struct->eeprom.user.backup_request = 0;
				error_code = EC_DATA_ERROR;
			}
			break;


		case MA_USER_COMMUNICATION_SLAVE_ADDRESS:
			limit_top = 0xfe;
			limit_bottom = 0x01;
			if(limit_bottom <= data_acc && data_acc <= limit_top){
				global_struct->eeprom.user.communication.slave_address = data_acc;
				global_struct->eeprom.user.backup_request = 1;
			}
			else{
				global_struct->eeprom.user.backup_request = 0;
				error_code = EC_DATA_ERROR;
			}
			break;


		//あってる？0x04が設定できてしまう
		case MA_USER_COMMUNICATION_BAUD_RATE:
			if(global_struct->eeprom.system.information.maintenance_mode){
				limit_top = 0xff;
				limit_bottom = 0x00;
			}
			else{
				limit_top = 0x05;
				limit_bottom = 0x03;
			}
			if(limit_bottom <= data_acc && data_acc <= limit_top){
				global_struct->eeprom.user.communication.baud_rate = data_acc;
				Communication_RS485_Config_Reflesh();
				global_struct->eeprom.user.backup_request = 1;
			}
			else{
				global_struct->eeprom.user.backup_request = 0;
				error_code = EC_DATA_ERROR;
			}
			break;



		case MA_USER_COMPARISON1_POWER_ON_DELAY:
			limit_top = 20;
			limit_bottom = 0;
			if(limit_bottom <= data_acc && data_acc <= limit_top){
				global_struct->eeprom.user.comparison1.power_on_delay = data_acc;
				global_struct->eeprom.user.backup_request = 1;
			}
			else{
				global_struct->eeprom.user.backup_request = 0;
				error_code = EC_DATA_ERROR;
			}
			break;



		case MA_USER_COMPARISON1_OFF_DELAY:
			limit_top = 20;
			limit_bottom = 0;
			if(limit_bottom <= data_acc && data_acc <= limit_top){
				global_struct->eeprom.user.comparison1.off_delay = data_acc;
				global_struct->eeprom.user.backup_request = 1;
			}
			else{
				global_struct->eeprom.user.backup_request = 0;
				error_code = EC_DATA_ERROR;
			}
			break;

		case MA_USER_COMPARISON1_ON_DELAY:
			limit_top = 20;
			limit_bottom = 0;
			if(limit_bottom <= data_acc && data_acc <= limit_top){
				global_struct->eeprom.user.comparison1.on_delay = data_acc;
				global_struct->eeprom.user.backup_request = 1;
			}
			else{
				global_struct->eeprom.user.backup_request = 0;
				error_code = EC_DATA_ERROR;
			}
			break;

		case MA_USER_COMPARISON1_MODE:
			limit_top = 1;
			limit_bottom = 0;
			if(limit_bottom <= data_acc && data_acc <= limit_top){
				global_struct->eeprom.user.comparison1.mode = data_acc;
				global_struct->eeprom.user.backup_request = 1;
			}
			else{
				global_struct->eeprom.user.backup_request = 0;
				error_code = EC_DATA_ERROR;
			}
			break;


		case MA_USER_COMPARISON1_ENABLED:
			limit_top = 3;
			limit_bottom = 0;
			if(limit_bottom <= data_acc && data_acc <= limit_top){
				global_struct->eeprom.user.comparison1.enabled = data_acc;
				global_struct->eeprom.user.backup_request = 1;
			}
			else{
				global_struct->eeprom.user.backup_request = 0;
				error_code = EC_DATA_ERROR;
			}
			break;

		//TODO:::
		case MA_USER_COMPARISON1_HYS_P1:
			data_acc = MODBUS_Pascal_To_Percentage(data_acc);
			if(!MODBUS_Check_Percentage(data_acc)){
				global_struct->eeprom.user.comparison1.hys_p1 = data_acc;
				global_struct->eeprom.user.backup_request = 1;
			}
			else{
				global_struct->eeprom.user.backup_request = 0;
				error_code = EC_DATA_ERROR;
			}
			break;

		//TODO:::
		case MA_USER_COMPARISON1_HYS_P2:
			data_acc = MODBUS_Pascal_To_Percentage(data_acc);
			if(!MODBUS_Check_Percentage(data_acc)){
				global_struct->eeprom.user.comparison1.hys_p2 = data_acc;
				global_struct->eeprom.user.backup_request = 1;
			}
			else{
				global_struct->eeprom.user.backup_request = 0;
				error_code = EC_DATA_ERROR;
			}
			break;

		case MA_USER_COMPARISON1_WIN_HI:
			data_acc = MODBUS_Pascal_To_Percentage(data_acc);
			if(!MODBUS_Check_Percentage(data_acc)){
				global_struct->eeprom.user.comparison1.win_hi = data_acc;
				global_struct->eeprom.user.backup_request = 1;
			}
			else{
				global_struct->eeprom.user.backup_request = 0;
				error_code = EC_DATA_ERROR;
			}
			break;

		case MA_USER_COMPARISON1_WIN_LOW:
			data_acc = MODBUS_Pascal_To_Percentage(data_acc);
			if(!MODBUS_Check_Percentage(data_acc)){
				global_struct->eeprom.user.comparison1.win_low = data_acc;
				global_struct->eeprom.user.backup_request = 1;
			}
			else{
				global_struct->eeprom.user.backup_request = 0;
				error_code = EC_DATA_ERROR;
			}
			break;

		case MA_USER_COMPARISON2_POWER_ON_DELAY:
			limit_top = 20;
			limit_bottom = 0;
			if(limit_bottom <= data_acc && data_acc <= limit_top){
				global_struct->eeprom.user.comparison2.power_on_delay = data_acc;
				global_struct->eeprom.user.backup_request = 1;
			}
			else{
				global_struct->eeprom.user.backup_request = 0;
				error_code = EC_DATA_ERROR;
			}
			break;



		case MA_USER_COMPARISON2_OFF_DELAY:
			limit_top = 20;
			limit_bottom = 0;
			if(limit_bottom <= data_acc && data_acc <= limit_top){
				global_struct->eeprom.user.comparison2.off_delay = data_acc;
				global_struct->eeprom.user.backup_request = 1;
			}
			else{
				global_struct->eeprom.user.backup_request = 0;
				error_code = EC_DATA_ERROR;
			}
			break;

		case MA_USER_COMPARISON2_ON_DELAY:
			limit_top = 20;
			limit_bottom = 0;
			if(limit_bottom <= data_acc && data_acc <= limit_top){
				global_struct->eeprom.user.comparison2.on_delay = data_acc;
				global_struct->eeprom.user.backup_request = 1;
			}
			else{
				global_struct->eeprom.user.backup_request = 0;
				error_code = EC_DATA_ERROR;
			}
			break;

		case MA_USER_COMPARISON2_MODE:
			limit_top = 1;
			limit_bottom = 0;
			if(limit_bottom <= data_acc && data_acc <= limit_top){
				global_struct->eeprom.user.comparison2.mode = data_acc;
				global_struct->eeprom.user.backup_request = 1;
			}
			else{
				global_struct->eeprom.user.backup_request = 0;
				error_code = EC_DATA_ERROR;
			}
			break;

		case MA_USER_COMPARISON2_ENABLED:
			limit_top = 3;
			limit_bottom = 0;
			if(limit_bottom <= data_acc && data_acc <= limit_top){
				global_struct->eeprom.user.comparison2.enabled = data_acc;
				global_struct->eeprom.user.backup_request = 1;
			}
			else{
				global_struct->eeprom.user.backup_request = 0;
				error_code = EC_DATA_ERROR;
			}
			break;

		case MA_USER_COMPARISON2_HYS_P1:
			data_acc = MODBUS_Pascal_To_Percentage(data_acc);
			if(!MODBUS_Check_Percentage(data_acc)){
				global_struct->eeprom.user.comparison2.hys_p1 = data_acc;
				global_struct->eeprom.user.backup_request = 1;
			}
			else{
				global_struct->eeprom.user.backup_request = 0;
				error_code = EC_DATA_ERROR;
			}
			break;

		case MA_USER_COMPARISON2_HYS_P2:
			data_acc = MODBUS_Pascal_To_Percentage(data_acc);
			if(!MODBUS_Check_Percentage(data_acc)){
				global_struct->eeprom.user.comparison2.hys_p2 = data_acc;
				global_struct->eeprom.user.backup_request = 1;
			}
			else{
				global_struct->eeprom.user.backup_request = 0;
				error_code = EC_DATA_ERROR;
			}
			break;

		case MA_USER_COMPARISON2_WIN_HI:
			data_acc = MODBUS_Pascal_To_Percentage(data_acc);
			if(!MODBUS_Check_Percentage(data_acc)){
				global_struct->eeprom.user.comparison2.win_hi = data_acc;
				global_struct->eeprom.user.backup_request = 1;
			}
			else{
				global_struct->eeprom.user.backup_request = 0;
				error_code = EC_DATA_ERROR;
			}
			break;

		case MA_USER_COMPARISON2_WIN_LOW:
			data_acc = MODBUS_Pascal_To_Percentage(data_acc);
			if(!MODBUS_Check_Percentage(data_acc)){
				global_struct->eeprom.user.comparison2.win_low = data_acc;
				global_struct->eeprom.user.backup_request = 1;
			}
			else{
				global_struct->eeprom.user.backup_request = 0;
				error_code = EC_DATA_ERROR;
			}
			break;

		//maintenance mode
		case MA_SYSTEM_INFORMATION_MAINTENANCE_MODE:
			limit_top = 1;
			limit_bottom = 0;
			if(limit_bottom <= data_acc && data_acc <= limit_top){
				global_struct->eeprom.system.information.maintenance_mode = data_acc;
				global_struct->eeprom.system.backup_request = 1;
			}
			else{
				global_struct->eeprom.user.backup_request = 0;
				error_code = EC_DATA_ERROR;
			}
			break;

		default:
			if(global_struct->eeprom.system.information.maintenance_mode){
				switch(address){

				//これらは下に別途ある : SYSTEM_INFORMATION
//					case MA_USER_MODE_PRODUCTION_NUMBER:
//						*global_struct->eeprom.user.user_information.production_number = data_acc;
//						global_struct->eeprom.user.backup_request = 1;
//						break;
//
//					case MA_USER_MODE_PRESSURE_RANGE_ID:
//						*global_struct->eeprom.user.user_information.pressure_range_id = data_acc;
//						global_struct->eeprom.user.backup_request = 1;
//						limit_top = 0xff;
//						limit_bottom = 0;
//						break;

					case MA_USER_DISPLAY_BRIGHTNESS:
						global_struct->eeprom.user.display.brightness = data_acc;
						global_struct->eeprom.user.backup_request = 1;
						break;

					case MA_USER_COMMUNICATION_MESSAGE_PROTOCOL:
						global_struct->eeprom.user.communication.message_protocol = data_acc;
						global_struct->eeprom.user.backup_request = 1;
						break;

					case MA_USER_COMMUNICATION_PARITY:
						global_struct->eeprom.user.communication.parity = data_acc;
						Communication_RS485_Config_Reflesh();
						global_struct->eeprom.user.backup_request = 1;
						break;

					case MA_USER_COMMUNICATION_STOP_BIT:
						global_struct->eeprom.user.communication.stop_bit = data_acc;
						Communication_RS485_Config_Reflesh();
						global_struct->eeprom.user.backup_request = 1;
						break;

					case MA_USER_COMPARISON1_OUTPUT_TYPE:
						global_struct->eeprom.user.comparison1.output_type = data_acc;
						global_struct->eeprom.user.backup_request = 1;
						break;

					case MA_USER_COMPARISON2_OUTPUT_TYPE:
						global_struct->eeprom.user.comparison2.output_type = data_acc;
						global_struct->eeprom.user.backup_request = 1;
						break;

					case MA_SENSOR_ZERO_ADJUSTMENT_COUNT:
						global_struct->eeprom.sensor.zero_adjustment.count = data_acc;
						global_struct->eeprom.sensor.backup_request = 1;
						break;

					case MA_SENSOR_ZERO_ADJUSTMENT_DISPLAY:
						global_struct->eeprom.sensor.zero_adjustment.display = data_acc;
						global_struct->eeprom.sensor.backup_request = 1;
						break;

					case MA_SENSOR_ZERO_ADJUSTMENT_OUTPUT:
						global_struct->eeprom.sensor.zero_adjustment.output = data_acc;
						global_struct->eeprom.sensor.backup_request = 1;
						break;

					case MA_SYSTEM_INFORMATION_SETTING_STATUS_FLAG:
						global_struct->eeprom.system.information.setting_status_flag = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_INFORMATION_SOFTWARE_VERSION:
						global_struct->eeprom.system.information.software_version = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_INFORMATION_HARDWARE_VERSION:
						global_struct->eeprom.system.information.hardware_version = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_INFORMATION_MECHANISM_VERSION:
						global_struct->eeprom.system.information.mechanism_version = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_INFORMATION_PRODUCTION_NUMBER:
						global_struct->eeprom.system.information.production_number = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_INFORMATION_CUSTOMER_CODE:
						global_struct->eeprom.system.information.customer_code = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_INFORMATION_PRESSURE_RANGE_ID:
						global_struct->eeprom.system.information.pressure_range_id = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_INFORMATION_POINT_POSITION:
						global_struct->eeprom.system.information.point_position = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_INFORMATION_SCALE_TYPE:
						global_struct->eeprom.system.information.scale_type = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_INFORMATION_FULLSCALE_PLUS:
						global_struct->eeprom.system.information.fullscale_plus = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_INFORMATION_FULLSCALE_MINUS:
						global_struct->eeprom.system.information.fullscale_minus = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_INFORMATION_SCALE_MARGIN:
						global_struct->eeprom.system.information.scale_margin = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_INFORMATION_OUTPUT_TYPE:
						global_struct->eeprom.system.information.output_type = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_INFORMATION_OUTPUT_PHY_PROTOCOL:
						global_struct->eeprom.system.information.output_phy_protocol = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_INFORMATION_COMPARISON_COUNT:
						global_struct->eeprom.system.information.comparison_count = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_INFORMATION_ANALOG_COUNT:
						global_struct->eeprom.system.information.analog_count = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_INFORMATION_DIGITAL_COUNT:
						global_struct->eeprom.system.information.digital_count = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_ZERO_OFFSET_FINE:
						global_struct->eeprom.system.correction_value.zero_offset_fine = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_ZERO_OFFSET_ROUGH:
						global_struct->eeprom.system.correction_value.zero_offset_rough = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_CORRECTION_COUNT:
						global_struct->eeprom.system.correction_value.correction_count = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_INSPECTION_TEMPERATURE:
						global_struct->eeprom.system.correction_value.inspection_temperature = data_acc;
						global_struct->ram.temperature.flag_calibration = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_INSPECTION_TEMPERATURE_OFFSET:
						global_struct->eeprom.system.correction_value.inspection_temperature_offset = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_REFERENCE:
						global_struct->eeprom.system.correction_value.temperature_reference = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_REFERENCE_ADC:
						global_struct->eeprom.system.correction_value.temperature_reference_adc = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_ZERO:
						global_struct->eeprom.system.correction_value.temperature_zero = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_SPAN:
						global_struct->eeprom.system.correction_value.temperature_span = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_COUNT:
						global_struct->eeprom.system.correction_value.temperature_count = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_CORRECTION_00:
						global_struct->eeprom.system.correction_value.correction[0] = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_CORRECTION_01:
						global_struct->eeprom.system.correction_value.correction[1] = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_CORRECTION_02:
						global_struct->eeprom.system.correction_value.correction[2] = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_CORRECTION_03:
						global_struct->eeprom.system.correction_value.correction[3] = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_CORRECTION_04:
						global_struct->eeprom.system.correction_value.correction[4] = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_CORRECTION_05:
						global_struct->eeprom.system.correction_value.correction[5] = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_CORRECTION_06:
						global_struct->eeprom.system.correction_value.correction[6] = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_CORRECTION_07:
						global_struct->eeprom.system.correction_value.correction[7] = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_CORRECTION_08:
						global_struct->eeprom.system.correction_value.correction[8] = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_CORRECTION_09:
						global_struct->eeprom.system.correction_value.correction[9] = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_CORRECTION_10:
						global_struct->eeprom.system.correction_value.correction[10] = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_00:
						global_struct->eeprom.system.correction_value.temperature[0] = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_01:
						global_struct->eeprom.system.correction_value.temperature[1] = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_02:
						global_struct->eeprom.system.correction_value.temperature[2] = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_03:
						global_struct->eeprom.system.correction_value.temperature[3] = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_04:
						global_struct->eeprom.system.correction_value.temperature[4] = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_05:
						global_struct->eeprom.system.correction_value.temperature[5] = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_06:
						global_struct->eeprom.system.correction_value.temperature[6] = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_07:
						global_struct->eeprom.system.correction_value.temperature[7] = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_08:
						global_struct->eeprom.system.correction_value.temperature[8] = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_09:
						global_struct->eeprom.system.correction_value.temperature[9] = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_10:
						global_struct->eeprom.system.correction_value.temperature[10] = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_DAC_MIN:
						global_struct->eeprom.system.correction_value.dac_min = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_DAC_MAX:
						global_struct->eeprom.system.correction_value.dac_max = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

					case MA_SYSTEM_CORRECTION_VALUE_DAC_CENTER:
						global_struct->eeprom.system.correction_value.dac_center = data_acc;
						global_struct->eeprom.system.backup_request = 1;
						break;

						//RAM
					case MA_OSC_PERIOD:
						global_struct->ram.pressure.osc_period = data_acc;
						break;
					case MA_OSC_PWM_VALUE:
						global_struct->ram.pressure.osc_pwm_value = data_acc;
						break;
					case MA_DISPLAY_TEST_DIGIT_00:
						global_struct->ram.display_test_mode.segments[0] = data_acc;
						break;
					case MA_DISPLAY_TEST_DIGIT_01:
						global_struct->ram.display_test_mode.segments[1] = data_acc;
						break;
					case MA_DISPLAY_TEST_DIGIT_02:
						global_struct->ram.display_test_mode.segments[2] = data_acc;
						break;
					case MA_DISPLAY_TEST_DIGIT_03:
						global_struct->ram.display_test_mode.segments[3] = data_acc;
						break;
					case MA_DISPLAY_TEST_LED:
						global_struct->ram.display_test_mode.leds[0] = data_acc & 0x01;
						global_struct->ram.display_test_mode.leds[1] = (data_acc >> 1) & 0x01;
							break;

					default:
						error_code = EC_ADDRESS_ERROR;
						break;
				}
			}
			else{
				error_code = EC_ADDRESS_ERROR;
			}
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
			MODBUS_Branch_Process_0x01(original_array, size, response_array, response_array_size);
			break;
		case 0x02:
			MODBUS_Branch_Process_0x02(original_array, size, response_array, response_array_size);
			break;
		case 0x03:
			MODBUS_Branch_Process_0x03(original_array, size, response_array, response_array_size);
			break;
		case 0x04:
			MODBUS_Branch_Process_0x04(original_array, size, response_array, response_array_size);
			break;
		case 0x05:
			MODBUS_Branch_Process_0x05(original_array, size, response_array, response_array_size);
			break;
		case 0x10:
			MODBUS_Branch_Process_0x10(original_array, size, response_array, response_array_size);
			break;

		default:
			*response_array_size = 0;
			break;
	}

}


void MODBUS_Init(Global_TypeDef *_global_struct){
	global_struct = _global_struct;

}
