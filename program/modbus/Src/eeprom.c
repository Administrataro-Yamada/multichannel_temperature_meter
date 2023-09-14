/*
 * eeprom->c
 *
 *  Created on: 2021/10/19
 *      Author: staff
 */


#include "i2c.h"
#include "eeprom.h"
#include "global_typedef.h"
#include "gpio.h"
#include "string.h"
#include "version.h"

#define EEPROM_GPIO_WP GPIOA, GPIO_PIN_15

#define __EEPROM_SET_GPIO_WP() HAL_GPIO_WritePin(EEPROM_GPIO_WP, SET)
#define __EEPROM_RESET_GPIO_WP() HAL_GPIO_WritePin(EEPROM_GPIO_WP, RESET)

static EEPROM_TypeDef *eeprom;
static I2C_HandleTypeDef *hi2c;
static uint8_t *factory_reset_flag;




static EEPROM_STATUS EEPROM_Read_Bytes(uint8_t start_address, uint8_t *data_array, uint16_t size){
	HAL_StatusTypeDef _status = HAL_ERROR;
	while(_status != HAL_OK){
		_status = HAL_I2C_Master_Transmit(hi2c, EEPROM_SLAVE_ADDRESS, &start_address, 1, 100);
	}

	_status = HAL_ERROR;
	while(_status != HAL_OK){
		_status = HAL_I2C_Master_Receive(hi2c, EEPROM_SLAVE_ADDRESS, data_array, size, 100);
	}

	return EEPROM_OK;
}



static EEPROM_STATUS EEPROM_Write_I2C(uint8_t *data_array, uint16_t size){
	HAL_StatusTypeDef _status = HAL_ERROR;
	__EEPROM_RESET_GPIO_WP();
	while(_status != HAL_OK){
		_status = HAL_I2C_Master_Transmit(hi2c, EEPROM_SLAVE_ADDRESS, data_array, size, 100);
	}
	__EEPROM_SET_GPIO_WP();
	return EEPROM_OK;
}

//fixed
static EEPROM_STATUS EEPROM_Write_DataArray(uint16_t start_address, uint8_t *data_array, uint16_t size){
	if(size > EEPROM_SPEC_BYTE_SIZE){
		return EEPROM_NG;
	}
	if((start_address + size) > EEPROM_SPEC_BYTE_SIZE){
		return EEPROM_NG;
	}


	/*
	 * 0x00 0x01 0x02 ... 0x06 0x07 -----  address
	 * .... .... .... ... 0x12 0x34 --+
	 * 0x56 0x78 0x9a ... 0xbc 0xdf --+--  data
	 * 0xaa 0xbb .... ... .... .... --+
	 *
	 */

	//buf[0]縺ｯstart_address
	//buf[1~]縺ｯ繝�繝ｼ繧ｿ
	uint8_t buf[EEPROM_SPEC_PAGE_BYTE_SIZE +1];
	buf[0] = start_address;


	uint8_t start_address_page = start_address/EEPROM_SPEC_PAGE_BYTE_SIZE;
	uint8_t first_page_size = EEPROM_SPEC_PAGE_BYTE_SIZE*(start_address_page+1) - start_address;

	//1繝壹�ｼ繧ｸ逶ｮ縺ｧ邨ゆｺ�縺ｮ蝣ｴ蜷�
	if(size <= first_page_size){
		for(uint8_t i = 0; i < size; i++){
			buf[i+1] = data_array[i];
		}
		EEPROM_Write_I2C(buf, size+1);
		return EEPROM_OK;
	}
	else{
		for(uint8_t i = 0; i < first_page_size; i++){
			buf[i+1] = data_array[i];
		}
	}

	//騾∽ｿ｡
	EEPROM_Write_I2C(buf, first_page_size + 1);



	//縺薙�ｮ譎らせ縺ｧ蠢�縺壹�壹�ｼ繧ｸ縺ｮ蜈磯�ｭ縺ｫ縺ｪ繧�
	start_address += first_page_size;
	size -= first_page_size;


	//2繝壹�ｼ繧ｸ逶ｮ莉･髯�
	uint8_t middle_page = size/EEPROM_SPEC_PAGE_BYTE_SIZE;
	for(uint8_t page = 0; page < middle_page; page++){
		buf[0] = start_address;
		for(uint8_t i = 0; i < EEPROM_SPEC_PAGE_BYTE_SIZE; i++){
			buf[i+1] = data_array[i + first_page_size + page*EEPROM_SPEC_PAGE_BYTE_SIZE];
		}
		EEPROM_Write_I2C(buf, EEPROM_SPEC_PAGE_BYTE_SIZE + 1);
		start_address += EEPROM_SPEC_PAGE_BYTE_SIZE;
		size -= EEPROM_SPEC_PAGE_BYTE_SIZE;
	}


	//譛�邨ゅ�壹�ｼ繧ｸ縺ｮ髟ｷ縺輔′0縺�縺｣縺溷�ｴ蜷育ｵゅｏ繧�
	if(size == 0)
		return EEPROM_OK;

	buf[0] = start_address;
	for(uint8_t i = 0; i < size; i++){
		buf[i+1] = data_array[i + middle_page*EEPROM_SPEC_PAGE_BYTE_SIZE+first_page_size];
	}
	EEPROM_Write_I2C(buf, size + 1);
	return EEPROM_OK;

}



static EEPROM_STATUS EEPROM_Verify_User(){

	uint8_t _buf[EEPROM_BUFFER_SIZE] = {0};

	uint8_t size = sizeof(eeprom->user.mode);
	EEPROM_Read_Bytes(EEPROM_PAGE_NUMBER_MODE * EEPROM_SPEC_PAGE_BYTE_SIZE, (uint8_t *)_buf, size);
	for(uint8_t i = 0; i < size; i++){
		if(_buf[i] != (uint8_t)*((uint8_t *)&(eeprom->user.mode)+i))
			return EEPROM_NG;
	}

	size = sizeof(eeprom->user.display);
	EEPROM_Read_Bytes(EEPROM_PAGE_NUMBER_DISPLAY * EEPROM_SPEC_PAGE_BYTE_SIZE, (uint8_t *)_buf, size);
	for(uint8_t i = 0; i < size; i++){
		if(_buf[i] != (uint8_t)*((uint8_t *)&(eeprom->user.display)+i))
			return EEPROM_NG;
	}

	size = sizeof(eeprom->user.output);
	EEPROM_Read_Bytes(EEPROM_PAGE_NUMBER_OUTPUT * EEPROM_SPEC_PAGE_BYTE_SIZE, (uint8_t *)_buf, size);
	for(uint8_t i = 0; i < size; i++){
		if(_buf[i] != (uint8_t)*((uint8_t *)&(eeprom->user.output)+i))
			return EEPROM_NG;
	}

	size = sizeof(eeprom->user.communication);
	EEPROM_Read_Bytes(EEPROM_PAGE_NUMBER_COMMUNICATION * EEPROM_SPEC_PAGE_BYTE_SIZE, (uint8_t *)_buf, size);
	for(uint8_t i = 0; i < size; i++){
		if(_buf[i] != (uint8_t)*((uint8_t *)&(eeprom->user.communication)+i))
			return EEPROM_NG;
	}
	size = sizeof(eeprom->user.comparison1);
	EEPROM_Read_Bytes(EEPROM_PAGE_NUMBER_COMPARISON_OUTPUT1 * EEPROM_SPEC_PAGE_BYTE_SIZE, (uint8_t *)_buf, size);
	for(uint8_t i = 0; i < size; i++){
		if(_buf[i] != (uint8_t)*((uint8_t *)&(eeprom->user.comparison1)+i))
			return EEPROM_NG;
	}
	size = sizeof(eeprom->user.comparison2);
	EEPROM_Read_Bytes(EEPROM_PAGE_NUMBER_COMPARISON_OUTPUT2 * EEPROM_SPEC_PAGE_BYTE_SIZE, (uint8_t *)_buf, size);
	for(uint8_t i = 0; i < size; i++){
		if(_buf[i] != (uint8_t)*((uint8_t *)&(eeprom->user.comparison2)+i))
			return EEPROM_NG;
	}
	return EEPROM_OK;
}

static EEPROM_STATUS EEPROM_Verify_Sensor(){
	uint8_t size = 0;
	uint8_t _buf[EEPROM_BUFFER_SIZE] = {0};

	size = sizeof(eeprom->sensor.zero_adjustment);
	EEPROM_Read_Bytes(EEPROM_PAGE_NUMBER_ZERO_ADJUSTMENT * EEPROM_SPEC_PAGE_BYTE_SIZE, (uint8_t *)_buf, size);
	for(uint8_t i = 0; i < size; i++){
		if(_buf[i] != (uint8_t)*((uint8_t *)&(eeprom->sensor.zero_adjustment)+i))
			return EEPROM_NG;
	}
	return EEPROM_OK;
}

static EEPROM_STATUS EEPROM_Verify_System(){
	uint8_t size = 0;
	uint8_t _buf[EEPROM_BUFFER_SIZE] = {0};

	size = sizeof(eeprom->system.information);
	EEPROM_Read_Bytes(EEPROM_PAGE_NUMBER_INFORMATION * EEPROM_SPEC_PAGE_BYTE_SIZE, (uint8_t *)_buf, size);
	for(uint8_t i = 0; i < size; i++){
		if(_buf[i] != (uint8_t)*((uint8_t *)&(eeprom->system.information)+i))
			return EEPROM_NG;
	}

	size = sizeof(eeprom->system.correction_value);
	EEPROM_Read_Bytes(EEPROM_PAGE_NUMBER_CORRECTION_VALUE * EEPROM_SPEC_PAGE_BYTE_SIZE, (uint8_t *)_buf, size);
	for(uint8_t i = 0; i < size; i++){
		if(_buf[i] != (uint8_t)*((uint8_t *)&(eeprom->system.correction_value)+i))
			return EEPROM_NG;
	}
	return EEPROM_OK;
}

static EEPROM_STATUS EEPROM_Verify(){

	if(EEPROM_Verify_User() == EEPROM_NG)
		return EEPROM_NG;
	if(EEPROM_Verify_Sensor() == EEPROM_NG)
		return EEPROM_NG;
	if(EEPROM_Verify_System() == EEPROM_NG)
		return EEPROM_NG;

	return EEPROM_OK;
}


static EEPROM_STATUS EEPROM_BackUp_User(){
	uint8_t counter = 0;
	do{

		EEPROM_Write_DataArray(EEPROM_PAGE_NUMBER_MODE * EEPROM_SPEC_PAGE_BYTE_SIZE, (uint8_t *)&(eeprom->user.mode), sizeof(eeprom->user.mode));
		EEPROM_Write_DataArray(EEPROM_PAGE_NUMBER_DISPLAY * EEPROM_SPEC_PAGE_BYTE_SIZE, (uint8_t *)&(eeprom->user.display), sizeof(eeprom->user.display));
		EEPROM_Write_DataArray(EEPROM_PAGE_NUMBER_OUTPUT * EEPROM_SPEC_PAGE_BYTE_SIZE, (uint8_t *)&(eeprom->user.output), sizeof(eeprom->user.output));
		EEPROM_Write_DataArray(EEPROM_PAGE_NUMBER_COMMUNICATION * EEPROM_SPEC_PAGE_BYTE_SIZE, (uint8_t *)&(eeprom->user.communication), sizeof(eeprom->user.communication));
		EEPROM_Write_DataArray(EEPROM_PAGE_NUMBER_COMPARISON_OUTPUT1 * EEPROM_SPEC_PAGE_BYTE_SIZE, (uint8_t *)&(eeprom->user.comparison1), sizeof(eeprom->user.comparison1));
		EEPROM_Write_DataArray(EEPROM_PAGE_NUMBER_COMPARISON_OUTPUT2 * EEPROM_SPEC_PAGE_BYTE_SIZE, (uint8_t *)&(eeprom->user.comparison2), sizeof(eeprom->user.comparison2));

		if(EEPROM_Verify_User() == EEPROM_OK)
			return EEPROM_OK;

		counter++;

	}while(counter < 3);

	return EEPROM_NG;
}

static EEPROM_STATUS EEPROM_BackUp_Sensor(){
	uint8_t counter = 0;
	do{

		if(eeprom->sensor.zero_adjustment.count < 0xffffffff){
			eeprom->sensor.zero_adjustment.count++;
		}

		EEPROM_Write_DataArray(EEPROM_PAGE_NUMBER_ZERO_ADJUSTMENT * EEPROM_SPEC_PAGE_BYTE_SIZE, (uint8_t *)&(eeprom->sensor.zero_adjustment), sizeof(eeprom->sensor.zero_adjustment));

		if(EEPROM_Verify_Sensor() == EEPROM_OK)
			return EEPROM_OK;

		counter++;

	}while(counter < 3);

	return EEPROM_NG;
}

static EEPROM_STATUS EEPROM_BackUp_System(){
	uint8_t counter = 0;
	do{

		EEPROM_Write_DataArray(EEPROM_PAGE_NUMBER_INFORMATION * EEPROM_SPEC_PAGE_BYTE_SIZE, (uint8_t *)&(eeprom->system.information), sizeof(eeprom->system.information));
		EEPROM_Write_DataArray(EEPROM_PAGE_NUMBER_CORRECTION_VALUE * EEPROM_SPEC_PAGE_BYTE_SIZE, (uint8_t *)&(eeprom->system.correction_value), sizeof(eeprom->system.correction_value));

		if(EEPROM_Verify_System() == EEPROM_OK)
			return EEPROM_OK;

		counter++;

	}while(counter < 3);

	return EEPROM_NG;
}

static EEPROM_STATUS EEPROM_BackUp(){

	EEPROM_STATUS status = EEPROM_OK;

	if(eeprom->backup_paused == 0){

		if(eeprom->user.backup_request | eeprom->backup_request_all){
			eeprom->user.backup_request = 0;
			status |= EEPROM_BackUp_User();
		}

		if(eeprom->sensor.backup_request | eeprom->backup_request_all){
			eeprom->sensor.backup_request = 0;
			status |= EEPROM_BackUp_Sensor();
		}

		if(eeprom->system.backup_request | eeprom->backup_request_all){
			eeprom->system.backup_request = 0;
			status |= EEPROM_BackUp_System();
		}

		if(eeprom->backup_request_all == 1){
			eeprom->backup_request_all = 0;
		}

	}

	//保証寿命100万回
	if(eeprom->sensor.zero_adjustment.count > 1000000){
		Status_Set_StatusCode(SC_EEPROM_ERROR);
	}

	return status;
}

static EEPROM_STATUS EEPROM_Load_All(){
	static uint8_t load_ongoing = 0;
	if(eeprom->load_request == 1){
		load_ongoing = 1;
		eeprom->backup_request_all = 0;
		eeprom->load_request = 0;

		do{
			EEPROM_Read_Bytes(EEPROM_PAGE_NUMBER_MODE * EEPROM_SPEC_PAGE_BYTE_SIZE, (uint8_t *)&(eeprom->user.mode), sizeof(eeprom->user.mode));
			EEPROM_Read_Bytes(EEPROM_PAGE_NUMBER_DISPLAY * EEPROM_SPEC_PAGE_BYTE_SIZE, (uint8_t *)&(eeprom->user.display), sizeof(eeprom->user.display));
			EEPROM_Read_Bytes(EEPROM_PAGE_NUMBER_OUTPUT * EEPROM_SPEC_PAGE_BYTE_SIZE, (uint8_t *)&(eeprom->user.output), sizeof(eeprom->user.output));
			EEPROM_Read_Bytes(EEPROM_PAGE_NUMBER_COMMUNICATION * EEPROM_SPEC_PAGE_BYTE_SIZE, (uint8_t *)&(eeprom->user.communication), sizeof(eeprom->user.communication));
			EEPROM_Read_Bytes(EEPROM_PAGE_NUMBER_COMPARISON_OUTPUT1 * EEPROM_SPEC_PAGE_BYTE_SIZE, (uint8_t *)&(eeprom->user.comparison1), sizeof(eeprom->user.comparison1));
			EEPROM_Read_Bytes(EEPROM_PAGE_NUMBER_COMPARISON_OUTPUT2 * EEPROM_SPEC_PAGE_BYTE_SIZE, (uint8_t *)&(eeprom->user.comparison2), sizeof(eeprom->user.comparison2));

			EEPROM_Read_Bytes(EEPROM_PAGE_NUMBER_ZERO_ADJUSTMENT * EEPROM_SPEC_PAGE_BYTE_SIZE, (uint8_t *)&(eeprom->sensor.zero_adjustment), sizeof(eeprom->sensor.zero_adjustment));

			EEPROM_Read_Bytes(EEPROM_PAGE_NUMBER_INFORMATION * EEPROM_SPEC_PAGE_BYTE_SIZE, (uint8_t *)&(eeprom->system.information), sizeof(eeprom->system.information));
			EEPROM_Read_Bytes(EEPROM_PAGE_NUMBER_CORRECTION_VALUE * EEPROM_SPEC_PAGE_BYTE_SIZE, (uint8_t *)&(eeprom->system.correction_value), sizeof(eeprom->system.correction_value));
		}while(EEPROM_Verify() != EEPROM_OK);
	}

	//初回起動時
	else if(eeprom->load_request == 2){
		eeprom->backup_request_all = 0;
		eeprom->load_request = 0;

		//system.information
		eeprom->system.information.setting_status_flag = 1;
		eeprom->system.information.maintenance_mode = 1;
		eeprom->system.information.software_version = SOFTWARE_VERSION;
		eeprom->system.information.hardware_version = HARDWARE_VERSION;
		eeprom->system.information.mechanism_version = MECHANISM_VERSION;
		eeprom->system.information.production_number = 0x12345678;
		eeprom->system.information.customer_code = CC_FULL_FUNCTION;

		/*
		6	D50
		8	D100
		11	D200
		12	D300
		13	D500
		16	D1000
		21	D+-50
		22	D+-100
		23	D+-200
		24	D+-300
		25	D+-500
		28	E2
		29	E3
		30	E5
		*/

		eeprom->system.information.pressure_range_id = 13;//500Pa
		eeprom->system.information.point_position = 0;
		eeprom->system.information.scale_type = 0;
		eeprom->system.information.fullscale_plus = 500;
		eeprom->system.information.fullscale_minus = 0;
		eeprom->system.information.scale_margin = 10;//10%
		eeprom->system.information.output_type = 0b0100;
		eeprom->system.information.output_phy_protocol = 1;
		eeprom->system.information.comparison_count = 2;
		eeprom->system.information.analog_count = 0;
		eeprom->system.information.digital_count = 1;

		//system.correction
		eeprom->system.correction_value.zero_offset_fine = 0x07ff;
		eeprom->system.correction_value.zero_offset_rough = 0x07ff;
		eeprom->system.correction_value.correction_count = 9;
		eeprom->system.correction_value.inspection_temperature = 0;
		eeprom->system.correction_value.inspection_temperature_offset = 0;
		eeprom->system.correction_value.temperature_reference = 0;
		eeprom->system.correction_value.temperature_reference_adc = 0;

		eeprom->system.correction_value.temperature_zero = 0;
		eeprom->system.correction_value.temperature_span = 0;

		eeprom->system.correction_value.correction[0] = 29984;	//0.0%
		eeprom->system.correction_value.correction[1] = 31160;	//12.5%
		eeprom->system.correction_value.correction[2] = 32336;	//25.0%
		eeprom->system.correction_value.correction[3] = 33512;	//37.5%
		eeprom->system.correction_value.correction[4] = 34688;	//50.0%
		eeprom->system.correction_value.correction[5] = 35864;	//62.5%
		eeprom->system.correction_value.correction[6] = 37040;	//75.0%
		eeprom->system.correction_value.correction[7] = 38216;	//87.5%
		eeprom->system.correction_value.correction[8] = 39395;	//100.0%





		//user
		eeprom->user.mode.eco_mode = 0;
		eeprom->user.mode.protected = 0;

		eeprom->user.display.low_cut = 0;
		eeprom->user.display.brightness = 16;
		eeprom->user.display.sign_inversion = 0;
		eeprom->user.display.average_time = 1;

		eeprom->user.output.output_enabled = 1;
		eeprom->user.output.average_time = 1;
		eeprom->user.output.output_inversion = 0;


		eeprom->user.communication.slave_address = 1;
		eeprom->user.communication.baud_rate = 8;
		eeprom->user.communication.parity = 0;
		eeprom->user.communication.stop_bit = 0;
		eeprom->user.communication.message_protocol = 1;

		eeprom->user.comparison1.power_on_delay = 0;
		eeprom->user.comparison1.output_type = 0;//PNP
		eeprom->user.comparison1.on_delay = 0;
		eeprom->user.comparison1.off_delay = 0;
		eeprom->user.comparison1.mode = 0;
		eeprom->user.comparison1.enabled = 3;
		if(eeprom->system.information.scale_type){
			eeprom->user.comparison1.hys_p1 = 4000;
			eeprom->user.comparison1.hys_p2 = 3900;
			eeprom->user.comparison1.win_hi = 4000;
			eeprom->user.comparison1.win_low = -4000;
		}
		else{
			eeprom->user.comparison1.hys_p1 = 9000;
			eeprom->user.comparison1.hys_p2 = 8900;
			eeprom->user.comparison1.win_hi = 9000;
			eeprom->user.comparison1.win_low = 1000;
		}

		eeprom->user.comparison2.power_on_delay = 0;
		eeprom->user.comparison2.output_type = 0;//PNP
		eeprom->user.comparison2.on_delay = 0;
		eeprom->user.comparison2.off_delay = 0;
		eeprom->user.comparison2.mode = 0;
		eeprom->user.comparison2.enabled = 3;
		if(eeprom->system.information.scale_type){
			eeprom->user.comparison2.hys_p1 = -4000;
			eeprom->user.comparison2.hys_p2 = -3900;
			eeprom->user.comparison2.win_hi = 3000;
			eeprom->user.comparison2.win_low = -3000;
		}
		else{
			eeprom->user.comparison2.hys_p1 = 1000;
			eeprom->user.comparison2.hys_p2 = 1100;
			eeprom->user.comparison2.win_hi = 8000;
			eeprom->user.comparison2.win_low = 2000;
		}

		//sensor
		//these not update
		//eeprom->sensor.zero_adjustment.count = 0
		//eeprom->sensor.zero_adjustment.display = 0;
		//eeprom->sensor.zero_adjustment.output = 0;



		eeprom->backup_request_all = 1;//ram -> eeprom

		return EEPROM_OK;
	}


	//工場出荷時設定
	else if(eeprom->load_request == 3 || *factory_reset_flag == 1){
		eeprom->backup_request_all = 0;
		eeprom->load_request = 0;
		*factory_reset_flag = 0;

		//system
		eeprom->system.information.maintenance_mode = 0;
		//eeprom->system.information.software_version = SOFTWARE_VERSION;

		//user
		eeprom->user.mode.eco_mode = 0;
		eeprom->user.mode.protected = 0;

		eeprom->user.display.low_cut = 2;
		eeprom->user.display.brightness = 16;
		eeprom->user.display.sign_inversion = 0;
		eeprom->user.display.average_time = 1;

		eeprom->user.output.output_enabled = 1;
		eeprom->user.output.average_time = 1;
		eeprom->user.output.output_inversion = 0;


		eeprom->user.communication.slave_address = 1;
		eeprom->user.communication.baud_rate = 3;
		eeprom->user.communication.parity = 0;
		eeprom->user.communication.stop_bit = 0;
		eeprom->user.communication.message_protocol = 1;

		eeprom->user.comparison1.power_on_delay = 0;
		eeprom->user.comparison1.on_delay = 0;
		eeprom->user.comparison1.off_delay = 0;
		eeprom->user.comparison1.mode = 0;
		eeprom->user.comparison1.enabled = 2;
		if(eeprom->system.information.scale_type){
			eeprom->user.comparison1.hys_p1 = 4000;
			eeprom->user.comparison1.hys_p2 = 3900;
			eeprom->user.comparison1.win_hi = 4000;
			eeprom->user.comparison1.win_low = -4000;
		}
		else{
			eeprom->user.comparison1.hys_p1 = 9000;
			eeprom->user.comparison1.hys_p2 = 8900;
			eeprom->user.comparison1.win_hi = 9000;
			eeprom->user.comparison1.win_low = 1000;
		}



		eeprom->user.comparison2.power_on_delay = 0;
		eeprom->user.comparison2.on_delay = 0;
		eeprom->user.comparison2.off_delay = 0;
		eeprom->user.comparison2.mode = 0;
		eeprom->user.comparison2.enabled = 2;
		if(eeprom->system.information.scale_type){
			eeprom->user.comparison2.hys_p1 = -4000;
			eeprom->user.comparison2.hys_p2 = -3900;
			eeprom->user.comparison2.win_hi = 3000;
			eeprom->user.comparison2.win_low = -3000;
		}
		else{
			eeprom->user.comparison2.hys_p1 = 1000;
			eeprom->user.comparison2.hys_p2 = 1100;
			eeprom->user.comparison2.win_hi = 8000;
			eeprom->user.comparison2.win_low = 2000;
		}




		eeprom->backup_request_all = 1;//ram -> eeprom

		return EEPROM_OK;
	}
	return EEPROM_NG;
}

void EEPROM_Process_Cyclic(){
	EEPROM_Load_All();
	EEPROM_BackUp();
}


void EEPROM_Backup_Request_User(){
	eeprom->user.backup_request = 1;
}

void EEPROM_Backup_Request_Sensor(){
	eeprom->system.backup_request = 1;
}

void EEPROM_Backup_Request_System(){
	eeprom->sensor.backup_request = 1;
}

void EEPROM_Backup_Request_All(){
	eeprom->backup_request_all = 1;
}

void EEPROM_Load_Request(){
	eeprom->load_request = 1;
}

void EEPROM_Factory_Reset(){
	eeprom->load_request = 3;
}

void EEPROM_Init(Global_TypeDef* _global_struct, I2C_HandleTypeDef *_hi2c){
	eeprom = &(_global_struct->eeprom);
	factory_reset_flag = &_global_struct->ram.status_and_flags.factory_reset_flag;
	hi2c = _hi2c;
	//memset(eeprom, 0, sizeof(*eeprom));

	eeprom->load_request = 1;
	EEPROM_Load_All();

	if((eeprom->system.information.setting_status_flag == 0xff) || (eeprom->system.information.setting_status_flag == 0)){
		eeprom->load_request = 2;
	}
	else{
		eeprom->load_request = 1;
	}
	EEPROM_Load_All();

}

