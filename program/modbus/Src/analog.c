/*
 * analog.c
 *
 *  Created on: 2020/12/11
 *      Author: staff
 */

#include "adc.h"
#include "analog.h"
#include "global_typedef.h"

#define AVERAGE_MAX_TIME 4000

typedef struct{
	uint16_t average_time_display;
	uint16_t average_time_output;
	uint16_t average_time_vdd;
	uint16_t average_time_temperature;
	uint8_t initialize_flag_output : 1;
	uint8_t initialize_flag_display : 1;
}Average_Time_TypeDef;


static uint16_t _adc[3]; //0:Sensor, 1:Temperature, 2:VDD

static Global_TypeDef *global_struct;
static Average_Time_TypeDef average_time;
TIM_HandleTypeDef *htim_osc;
TIM_HandleTypeDef *htim_sensor_calibration;


static uint16_t Analog_Get_Average_Time(uint16_t index){
	uint16_t val = 0;
	switch(index){
		case 0:
			val = 200;
			break;
		case 1:
			val = 1000;
			break;
		case 2:
			val = 2000;
			break;
		case 3:
			val = 4000;
			break;
		default:
			val = index;
			break;
	}
	return val;
}

static uint16_t Analog_Get_ADC_Averaged_Temperature(){

	volatile static uint16_t counter = 0;
	volatile static uint16_t _adc_average_array[200] = {0};
	volatile static uint32_t sum = 0;

	sum -= _adc_average_array[counter];
	_adc_average_array[counter] = _adc[1];
	sum += _adc_average_array[counter];

	counter++;
	if(counter >= average_time.average_time_temperature){
		counter = 0;
	}


	return (uint32_t)sum/(uint32_t)average_time.average_time_temperature;
}

static uint16_t Analog_Get_ADC_Averaged_Vdd(){

	volatile static uint16_t counter = 0;
	volatile static uint16_t _adc_average_array[200] = {0};

	volatile static uint32_t sum = 0;

	sum -= _adc_average_array[counter];

	_adc_average_array[counter] = _adc[2];
	sum += _adc_average_array[counter];

	counter++;
	if(counter >= average_time.average_time_vdd){
		counter = 0;
	}

	return (uint32_t)sum/(uint32_t)average_time.average_time_vdd;

}

static uint16_t Analog_Get_ADC_Averaged_Output(){
	volatile static uint16_t counter = 0;
	volatile static uint16_t _adc_average_array[AVERAGE_MAX_TIME] = {0};
	volatile static uint32_t sum = 0;

	if(average_time.initialize_flag_output){
		average_time.initialize_flag_output = 0;
		uint16_t adc_acc = _adc_average_array[counter];
		sum = 0;
		for(uint16_t i = 0; i < average_time.average_time_output; i++){
			_adc_average_array[i] = adc_acc;
			sum += adc_acc;
		}
		counter = 0;
	}
	else{

		sum -= _adc_average_array[counter];

		_adc_average_array[counter] = _adc[0];
		sum += _adc_average_array[counter];


		counter++;
		if(counter >= average_time.average_time_output){
			counter = 0;
		}
	}

	return (uint32_t)sum/(uint32_t)average_time.average_time_output;

}

static uint16_t Analog_Get_ADC_Averaged_Display(){
	volatile static uint16_t counter = 0;
	volatile static uint16_t _adc_average_array[AVERAGE_MAX_TIME] = {0};
	volatile static uint32_t sum = 0;

	if(average_time.initialize_flag_display){
		average_time.initialize_flag_display = 0;

		uint16_t adc_acc = _adc_average_array[counter];
		sum = 0;
		for(uint16_t i = 0; i < average_time.average_time_display; i++){
			_adc_average_array[i] = adc_acc;
			sum += adc_acc;
		}
		counter = 0;
	}
	else{

		sum -= _adc_average_array[counter];

		_adc_average_array[counter] = _adc[0];
		sum += _adc_average_array[counter];

		counter++;
		if(counter >= average_time.average_time_display){
			counter = 0;
		}
	}
	return (uint32_t)sum/(uint32_t)average_time.average_time_display;

}

typedef struct{
	float z_1;
	float z_2;
	float a[3];
	float b[3];
	float A;
	float omega;
	float sin_omega;
	float cos_omega;
	float alpha;
	float Q;
	float cutoff_freq;
	float sampling_freq;
	float prewarping_freq;
}Analog_Filter_Characteristics_TypeDef;


//縺薙ｌ縺ｯ菴ｿ縺｣縺ｦ縺ｪ縺�縺代←縺昴�ｮ縺�縺｡螳溯｣�縺励◆縺ИIR縺ｮLPF
#include "math.h"
static void Analog_IIR_LPF_Init(Analog_Filter_Characteristics_TypeDef *filter, int32_t sampling_freq, int32_t cutoff_freq){

	filter->sampling_freq = sampling_freq;//1000Hz -> 1000000
	filter->cutoff_freq = cutoff_freq;
	filter->A = pow(10, 1/40.0); //0dB
	filter->Q = 1 / 1.414;

	filter->prewarping_freq = 1/(2*pi()) * tan((pi()*filter->cutoff_freq)/filter->sampling_freq);
	filter->omega = 2 * pi() * filter->prewarping_freq;
	filter->sin_omega = sin(filter->omega);
	filter->cos_omega = cos(filter->omega);
	filter->alpha = 2 * filter->sin_omega / (filter->Q);


	filter->b[1] = 1 - filter->cos_omega;
	filter->b[0] = filter->b[1] / 2.0;
	filter->b[2] = filter->b[0];

	filter->a[0] = 1 + filter->alpha;
	filter->a[1] = -2 * filter->cos_omega;
	filter->a[2] = 1 - filter->alpha;
}
static uint32_t Analog_IIR_LPF_Get_Value(Analog_Filter_Characteristics_TypeDef *filter, float z_0){
	volatile static float z_1 = 0;
	volatile static float z_2 = 0;

	float output = filter->b[0]/filter->a[0] * z_0 + filter->b[1]/filter->b[0] * z_1 + filter->b[2]/filter->b[0] * z_2 + filter->a[1]/filter->a[0] * z_1 + filter->a[2]/filter->a[0] * z_2;

	z_2 = z_1;
	z_1 = output;
	return output;
}




static void Analog_Process_Set_Offset_Zero(){
	volatile static uint16_t rough = 0;
	volatile static uint16_t fine = 0;

	if(rough != global_struct->eeprom.system.correction_value.zero_offset_rough){
		rough = global_struct->eeprom.system.correction_value.zero_offset_rough;
		__HAL_TIM_SET_COMPARE(htim_sensor_calibration, TIM_CHANNEL_1, global_struct->eeprom.system.correction_value.zero_offset_rough);
	}
	if(fine != global_struct->eeprom.system.correction_value.zero_offset_fine){
		fine = global_struct->eeprom.system.correction_value.zero_offset_fine;
		//__HAL_TIM_SET_COMPARE(htim_sensor_calibration, TIM_CHANNEL_3, 0x7ff + (0x7ff - global_struct->eeprom.system.correction_value.zero_offset_fine));
		//__HAL_TIM_SET_COMPARE(htim_sensor_calibration, TIM_CHANNEL_4, 0x7ff - (0x7ff - global_struct->eeprom.system.correction_value.zero_offset_fine));
	}
}





void Analog_Set_Temperature_Calibration_reference(int16_t ref){
	global_struct->eeprom.system.correction_value.temperature_reference = ref;
}
void Analog_Set_Temperature_Calibration_zero(int16_t zero){
	global_struct->eeprom.system.correction_value.temperature_zero = zero;
}
void Analog_Set_Temperature_Calibration_span(int16_t span){
	global_struct->eeprom.system.correction_value.temperature_span = span;
}


void Analog_Set_Average_Time_Temperature(uint8_t index){
	uint16_t time = Analog_Get_Average_Time(index);
	average_time.average_time_temperature = time;
}

void Analog_Set_Average_Time_Vdd(uint8_t index){
	uint16_t time = Analog_Get_Average_Time(index);
	average_time.average_time_vdd = time;
}

void Analog_Set_Average_Time_Output(uint8_t index){
	uint16_t time = Analog_Get_Average_Time(index);
	average_time.average_time_output = time;
	average_time.initialize_flag_output = 1;
}

void Analog_Set_Average_Time_Display(uint8_t index){
	uint16_t time = Analog_Get_Average_Time(index);
	average_time.average_time_display = time;
	average_time.initialize_flag_display = 1;
}




void Analog_Process_OSC(){

	//Activate
	volatile static uint8_t bool = 0;
	if(global_struct->ram.pressure.osc_activated != bool){

		bool = global_struct->ram.pressure.osc_activated;

		if(bool){
			HAL_TIM_PWM_Start(htim_osc, TIM_CHANNEL_1);
		}
		else{
			HAL_TIM_PWM_Stop(htim_osc, TIM_CHANNEL_1);
		}
	}

	//Period
	htim_osc->Init.Period = global_struct->ram.pressure.osc_period;
	//PWM
	__HAL_TIM_SET_COMPARE(htim_osc, TIM_CHANNEL_1, global_struct->ram.pressure.osc_pwm_value);

}

void Analog_Process_Pressure(){
	global_struct->ram.pressure.adc_display = Analog_Get_ADC_Averaged_Display();
	global_struct->ram.pressure.adc_output = Analog_Get_ADC_Averaged_Output();
	global_struct->ram.pressure.adc_temperature = Analog_Get_ADC_Averaged_Temperature();
	global_struct->ram.pressure.adc_vdd = Analog_Get_ADC_Averaged_Vdd();
}


//1ms
void Analog_Process_Cyclic(){
	Analog_Process_Set_Offset_Zero();
	Analog_Process_OSC();
	Analog_Process_Pressure();
}

void Analog_Init(Global_TypeDef *_global_struct, TIM_HandleTypeDef *_htim_osc, TIM_HandleTypeDef *_htim_sensor_calibration, ADC_HandleTypeDef *_hadc){
	global_struct = _global_struct;
	htim_osc = _htim_osc;
	htim_sensor_calibration = _htim_sensor_calibration;

	//64MHz -> 300kHz
	global_struct->ram.pressure.osc_period = 213;
	global_struct->ram.pressure.osc_pwm_value = 106;//Duty 50%
	global_struct->ram.pressure.osc_activated = 1;


	//zero offset rough
	HAL_TIM_PWM_Start(htim_sensor_calibration, TIM_CHANNEL_1);

	//zero offset fine
	//HAL_TIM_PWM_Start(htim_sensor_calibration, TIM_CHANNEL_3);
	//HAL_TIM_PWM_Start(htim_sensor_calibration, TIM_CHANNEL_4);


	Analog_Set_Average_Time_Display(global_struct->eeprom.user.display.average_time);
	Analog_Set_Average_Time_Output(global_struct->eeprom.user.output.average_time);
	Analog_Set_Average_Time_Temperature(0);//200ms
	Analog_Set_Average_Time_Vdd(0);//200ms

	HAL_ADCEx_Calibration_Start(_hadc);
	HAL_ADC_Start_DMA(_hadc, _adc, 3);

}


uint8_t Analog_Init_Low_Voltage_Detection(ADC_HandleTypeDef *_hadc){
	Analog_Set_Average_Time_Vdd(0);//200ms
	HAL_ADCEx_Calibration_Start(_hadc);
	HAL_ADC_Start_DMA(_hadc, _adc, 3);
	volatile static uint16_t counter = 0;
	while(1){
		uint32_t adc =  _adc[2];
		uint32_t v = (uint32_t)((*VREFINT_CAL_ADDR)*16) * (uint32_t)VREFINT_CAL_VREF;
		v /= adc;

		if(3000 < v && v < 3400){
			if(counter >= 64000){
				break;
			}
			else{
				counter++;
			}
		}
	}

	HAL_ADC_Stop_DMA(_hadc);

	return 1;

}
