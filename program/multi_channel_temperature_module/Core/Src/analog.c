/*
 * analog.c
 *
 *  Created on: Nov 22, 2021
 *      Author: staff
 */

#include "adc.h"
#include "analog.h"



#define VDD_VOLTAGE 3300 //mV
#define AVERAGE_TIME 32
#define ANALOG_CHANNEL_COUNT	7


static uint16_t adc_buffer[ANALOG_CHANNEL_COUNT];
static uint16_t averaged_adc[ANALOG_CHANNEL_COUNT];





static void Analog_Averaged_ADC_Process(uint16_t *adc_pointer){

	static uint16_t counter = 0;
	static uint16_t _adc_average_array[ANALOG_CHANNEL_COUNT][AVERAGE_TIME] = {0};
	static uint32_t sum[ANALOG_CHANNEL_COUNT] = {0};

	for(uint8_t i = 0; i < ANALOG_CHANNEL_COUNT; i++){
		_adc_average_array[i][counter] = adc_buffer[TEMPERATURE_CHANNEL_REFERENCE];

		sum[i] += _adc_average_array[i][counter];

		counter++;
		if(counter >= AVERAGE_TIME){
			counter = 0;
		}

		sum[i] -= _adc_average_array[i][counter];

		adc_pointer[i] = (uint32_t)sum[i]/(uint32_t)AVERAGE_TIME;
	}
}
static int16_t Analog_Convert_To_Voltage(uint16_t adc){

}

static int16_t Analog_Convert_To_Vdd(uint16_t adc){
	int16_t voltage = (int32_t)(adc*VDD_VOLTAGE)/4095;
	return voltage;
}

static int16_t Analog_Convert_To_Vref(uint16_t adc){
	int16_t voltage = Analog_Convert_To_Voltage(adc);

}

static int16_t Analog_Get_Thermocouple_Type_K(int16_t voltage){
	int32_t temperature;
	return (int16_t)temperature;
}
static int16_t Analog_Convert_To_Temperature(uint16_t adc){
	int16_t voltage = Analog_Convert_To_Voltage(adc);
	int16_t vref = Analog_Convert_To_Voltage(averaged_adc[TEMPERATURE_CHANNEL_VREF]);
	int16_t vdd = Analog_Convert_To_Vdd(averaged_adc[TEMPERATURE_CHANNEL_VDD]);
	int32_t temperature = (voltage - vref);
	return ;
}

static int16_t Analog_Convert_To_Reference(uint16_t adc){
	int16_t voltage = Analog_Convert_To_Voltage(adc);
}


static int16_t Analog_Get_Temperature(TEMPERATURE_CHANNEL channel){

	int16_t value = 0;
	switch (channel) {
		case TEMPERATURE_CHANNEL_CH1:
		case TEMPERATURE_CHANNEL_CH2:
		case TEMPERATURE_CHANNEL_CH3:
		case TEMPERATURE_CHANNEL_CH4:
			value = Analog_Convert_To_Temperature(averaged_adc[channel]);
			break;

		case TEMPERATURE_CHANNEL_REFERENCE:
			value = Analog_Convert_To_Reference(averaged_adc[channel]);
			break;

		default:
			break;
	}
	return value;

}

void Analog_Process_Cyclic(){

	Analog_Averaged_ADC_Process(averaged_adc);

}

void Analog_Init(void(*ADC_DMA_Start)(uint8_t*, uint8_t)){

	ADC_DMA_Start(adc_buffer, ANALOG_CHANNEL_COUNT);
	//HAL_ADCEx_Calibration_Start(_hadc);
	//HAL_ADC_Start_DMA(_hadc, global_struct->adc_buffer, 2);



}
