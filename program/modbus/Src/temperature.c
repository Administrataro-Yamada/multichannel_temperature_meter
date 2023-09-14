/*
 * temperature.c
 *
 *  Created on: 2021/08/17
 *      Author: staff
 */
#include "main.h"
#include "analog.h"
#include "global_typedef.h"
#include "temperature.h"


static Global_TypeDef *global_struct;


static int16_t _Temperature_Get_Temperature(){
	uint16_t adc = global_struct->ram.pressure.adc_temperature;
	int32_t t = adc * global_struct->ram.temperature.vdd;
	//電源電圧
	t /= (float)TEMPSENSOR_CAL_VREFANALOG; //STM32の工場出荷時に温度計を調整した電圧：TEMPSENSOR_CAL_VREFANALOG == 3000mV、現在の電源電圧に対する補正
	t -= (float)((*TEMPSENSOR_CAL1_ADDR)*16);// *TEMPSENSOR_CAL1_ADDR == 1040 @ 12bit, *16は左4シフトに同じ(16bit化)
	//傾き
	t *= TEMPSENSOR_CAL2_TEMP*100 - TEMPSENSOR_CAL1_TEMP*100;// TEMPSENSOR_CAL2_TEMP == 130degC, TEMPSENSOR_CAL1_TEMP == 30degC
	t /= (*TEMPSENSOR_CAL2_ADDR)*16 - (*TEMPSENSOR_CAL1_ADDR)*16;// *TEMPSENSOR_CAL2_ADDR == 1384 @ 12bit


	if(global_struct->ram.temperature.flag_calibration){
		global_struct->ram.temperature.flag_calibration = 0;
		global_struct->eeprom.system.correction_value.inspection_temperature_offset = global_struct->eeprom.system.correction_value.inspection_temperature - t;
		global_struct->eeprom.system.backup_request = 1;
	}

	t += global_struct->eeprom.system.correction_value.inspection_temperature_offset;
//	t += TEMPSENSOR_CAL1_TEMP*100;	//3000(30 degC)
	return (int16_t)t;
}

static uint16_t Temperature_Get_VDD(){
	uint32_t adc = global_struct->ram.pressure.adc_vdd;
	uint32_t v = (uint32_t)((*VREFINT_CAL_ADDR)*16) * (uint32_t)VREFINT_CAL_VREF;
	v /= adc;
	return v;
}

void Temperature_Process_Cyclic(){
	global_struct->ram.temperature.vdd = Temperature_Get_VDD();
	global_struct->ram.temperature.temperature = _Temperature_Get_Temperature();
}

uint16_t Temperature_Get_Vdd(){
	return global_struct->ram.temperature.vdd;
}

int16_t Temperature_Get_Temperature(){
	return global_struct->ram.temperature.temperature;
}

void Temperature_Init(Global_TypeDef *_global_struct){
	global_struct = _global_struct;
}
