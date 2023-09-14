/*
 * sensor.c
 *
 *  Created on: 2021/10/21
 *      Author: staff
 */
#include "main.h"
#include "analog.h"
#include "sensor.h"
#include "global_typedef.h"

static Global_TypeDef *global_struct;

static Sensor_TypeDef sensor = {0};


static int16_t Sensor_Calibrate_Percentage_With_Temperature(int32_t percentage_origin){

	int64_t tz = (global_struct->eeprom.system.correction_value.temperature_zero);//zero
    int64_t ts = (global_struct->eeprom.system.correction_value.temperature_span);//span

    int64_t delta_temperature = global_struct->ram.temperature.temperature - global_struct->eeprom.system.correction_value.temperature_reference;


    //calc zero
    int64_t percentage_zero = tz * delta_temperature;
    percentage_zero /= 100*100;

    //calc span
    int64_t percentage_span_denominator = ((int64_t)percentage_origin + percentage_zero) * (int64_t)100000;//分子
    int64_t percentage_span_numerator = -ts * delta_temperature;//分母
    percentage_span_numerator /= 1000;
    percentage_span_numerator += 100000;
    int64_t percentage_span = percentage_span_denominator / percentage_span_numerator;



    return percentage_span;
}




static int16_t Sensor_Convert_ADC_To_Calibrated_Percentage_With_Correction(uint16_t adc){

	uint32_t index;


	if(global_struct->eeprom.system.correction_value.correction[global_struct->eeprom.system.correction_value.correction_count-1] < adc){
		index = global_struct->eeprom.system.correction_value.correction_count-1;
	}
	else if(global_struct->eeprom.system.correction_value.correction[0] > adc){
		index = 1;
	}
	else{
		for(index = 1; index < global_struct->eeprom.system.correction_value.correction_count-1; index++){
			if(adc < global_struct->eeprom.system.correction_value.correction[index]){
				break;
			}
		}
	}

	int32_t delta_span =  global_struct->eeprom.system.correction_value.correction[index] - global_struct->eeprom.system.correction_value.correction[index-1];
	int32_t delta_adc = adc - global_struct->eeprom.system.correction_value.correction[index - 1];

	int32_t corrected_percentage = (delta_adc*10000 / delta_span) / (global_struct->eeprom.system.correction_value.correction_count - 1);
	corrected_percentage += 10000 / (global_struct->eeprom.system.correction_value.correction_count - 1) * (index-1);

	//0: +100%, 1: +-50%, 2: -100%
	switch(global_struct->eeprom.system.information.scale_type){
		case 0:
			corrected_percentage = corrected_percentage;
			break;
		case 1:
			corrected_percentage = corrected_percentage - 5000;
			break;
		case 2:
			corrected_percentage = corrected_percentage;
			break;
		default:
			break;
	}
	return (int16_t)corrected_percentage;
}

static int16_t Sensor_Convert_Percentage_To_Pascal(int16_t percentage){
	uint16_t fs = global_struct->eeprom.system.information.fullscale_plus + global_struct->eeprom.system.information.fullscale_minus;
	int16_t pascal = (int32_t)(fs * percentage)/10000;
	return pascal;
}



static void Sensor_Calculate_Pressure_Percentage_Output(){
	if(global_struct->ram.status_and_flags.mode_state == 2){// test modeのときは計算結果を代入しない(値が衝突するため)
		return;
	}
	global_struct->ram.pressure.percentage_output_raw = Sensor_Convert_ADC_To_Calibrated_Percentage_With_Correction(global_struct->ram.pressure.adc_output);
	global_struct->ram.pressure.percentage_calibrated_output = Sensor_Calibrate_Percentage_With_Temperature(global_struct->ram.pressure.percentage_output_raw);

	int16_t acc_pressure_percentage = global_struct->ram.pressure.percentage_calibrated_output - global_struct->eeprom.sensor.zero_adjustment.output;

	//sign inversion
	if(global_struct->eeprom.user.output.output_inversion){
		acc_pressure_percentage = -acc_pressure_percentage;
	}

	global_struct->ram.pressure.percentage_output_adjusted = acc_pressure_percentage;

}
static void Sensor_Calculate_Pressure_Percentage_Display(){
	if(global_struct->ram.status_and_flags.mode_state == 2){// test modeのときは計算結果を代入しない(値が衝突するため)
		return;
	}
	global_struct->ram.pressure.percentage_display_raw = Sensor_Convert_ADC_To_Calibrated_Percentage_With_Correction(global_struct->ram.pressure.adc_display);
	global_struct->ram.pressure.percentage_calibrated_display = Sensor_Calibrate_Percentage_With_Temperature(global_struct->ram.pressure.percentage_display_raw);

	int16_t acc_pressure_percentage = global_struct->ram.pressure.percentage_calibrated_display - global_struct->eeprom.sensor.zero_adjustment.display;

	//sign inversion
	if(global_struct->eeprom.user.display.sign_inversion){
		acc_pressure_percentage = -acc_pressure_percentage;
	}

	//low cut
	if(global_struct->eeprom.user.display.low_cut*100 <= acc_pressure_percentage || -global_struct->eeprom.user.display.low_cut*100 >= acc_pressure_percentage){
		global_struct->ram.pressure.percentage_display_adjusted = acc_pressure_percentage;
	}
	else{
		global_struct->ram.pressure.percentage_display_adjusted = 0;
	}
}

static void Sensor_Calculate_Pressure_Pascal_Output(){
	if(global_struct->ram.status_and_flags.mode_state == 2){// test modeのときは計算結果を代入しない(値が衝突するため)
		return;
	}
	global_struct->ram.pressure.pascal_output = Sensor_Convert_Percentage_To_Pascal(global_struct->ram.pressure.percentage_output_adjusted);
}

static void Sensor_Calculate_Pressure_Pascal_Display(){
	if(global_struct->ram.status_and_flags.mode_state == 2){// test modeのときは計算結果を代入しない(値が衝突するため)
		return;
	}
	global_struct->ram.pressure.pascal_display = Sensor_Convert_Percentage_To_Pascal(global_struct->ram.pressure.percentage_display_adjusted);
}



static uint8_t Sensor_Calculate_IsOverflowed(int16_t percentage_pressure, uint8_t sign_inversion){
	int16_t plimit;
	int16_t mlimit;
	switch(global_struct->eeprom.system.information.scale_type){
	case 0:
		plimit = (int16_t)((int16_t)global_struct->eeprom.system.information.scale_margin*(int16_t)100 + (int16_t)10000);
		mlimit = (int16_t)((int16_t)global_struct->eeprom.system.information.scale_margin*(int16_t)100);
		break;
	case 1:
		plimit = (int16_t)((int16_t)global_struct->eeprom.system.information.scale_margin*(int16_t)100 + (int16_t)5000);
		mlimit = (int16_t)((int16_t)global_struct->eeprom.system.information.scale_margin*(int16_t)100 + (int16_t)5000);
		break;
	case 2:
		plimit = (int16_t)((int16_t)global_struct->eeprom.system.information.scale_margin*(int16_t)100);
		mlimit = (int16_t)((int16_t)global_struct->eeprom.system.information.scale_margin*(int16_t)100 + (int16_t)10000);
		break;
	}


	uint8_t b = 0;

	if(sign_inversion){
		//sign inversion
		if(-percentage_pressure <= -mlimit){
			b = 1;
		}
		else{
			b = 0;
		}
	}
	else{
		if(percentage_pressure >= plimit){
			b = 1;
		}
		else{
			b = 0;
		}
	}
	return b;
}

static uint8_t Sensor_Calculate_IsUnderflowed(int16_t percentage_pressure, uint8_t sign_inversion){
	int16_t plimit;
	int16_t mlimit;
	switch(global_struct->eeprom.system.information.scale_type){
	case 0:
		plimit = (int16_t)((int16_t)global_struct->eeprom.system.information.scale_margin*(int16_t)100 + (int16_t)10000);
		mlimit = (int16_t)((int16_t)global_struct->eeprom.system.information.scale_margin*(int16_t)100);
		break;
	case 1:
		plimit = (int16_t)((int16_t)global_struct->eeprom.system.information.scale_margin*(int16_t)100 + (int16_t)5000);
		mlimit = (int16_t)((int16_t)global_struct->eeprom.system.information.scale_margin*(int16_t)100 + (int16_t)5000);
		break;
	case 2:
		plimit = (int16_t)((int16_t)global_struct->eeprom.system.information.scale_margin*(int16_t)100);
		mlimit = (int16_t)((int16_t)global_struct->eeprom.system.information.scale_margin*(int16_t)100 + (int16_t)10000);
		break;
	}


	uint8_t b = 0;
	if(sign_inversion){
		//sign inversion
		if(-percentage_pressure >= plimit){
			b = 1;
		}
		else{
			b = 0;
		}
	}
	else{
		if(percentage_pressure <= -mlimit){
			b = 1;
		}
		else{
			b = 0;
		}
	}
	return b;
}

static void Sensor_Calculate_Overflow_Underflow(){

	global_struct->ram.pressure.overflowed_display = Sensor_Calculate_IsOverflowed(global_struct->ram.pressure.percentage_display_adjusted, global_struct->eeprom.user.display.sign_inversion);
	global_struct->ram.pressure.underflowed_display = Sensor_Calculate_IsUnderflowed(global_struct->ram.pressure.percentage_display_adjusted, global_struct->eeprom.user.display.sign_inversion);

	global_struct->ram.pressure.overflowed_output = Sensor_Calculate_IsOverflowed(global_struct->ram.pressure.percentage_output_adjusted, global_struct->eeprom.user.output.output_inversion);
	global_struct->ram.pressure.underflowed_output = Sensor_Calculate_IsUnderflowed(global_struct->ram.pressure.percentage_output_adjusted, global_struct->eeprom.user.output.output_inversion);


	if(global_struct->ram.pressure.overflowed_output){
		Status_Set_StatusCode(SC_OVER_FLOW);
	}
	if(global_struct->ram.pressure.underflowed_output){
		Status_Set_StatusCode(SC_UNDER_FLOW);
	}

	//display
	sensor.display.overflowed_max = Sensor_Calculate_IsOverflowed(sensor.display.percentage_max, global_struct->eeprom.user.display.sign_inversion);
	sensor.display.underflowed_max = Sensor_Calculate_IsUnderflowed(sensor.display.percentage_max, global_struct->eeprom.user.display.sign_inversion);
	sensor.display.overflowed_min = Sensor_Calculate_IsOverflowed(sensor.display.percentage_min, global_struct->eeprom.user.display.sign_inversion);
	sensor.display.underflowed_min = Sensor_Calculate_IsUnderflowed(sensor.display.percentage_min, global_struct->eeprom.user.display.sign_inversion);
	//output
	sensor.output.overflowed_max = Sensor_Calculate_IsOverflowed(sensor.output.percentage_max, global_struct->eeprom.user.output.output_inversion);
	sensor.output.underflowed_max = Sensor_Calculate_IsUnderflowed(sensor.output.percentage_max, global_struct->eeprom.user.output.output_inversion);
	sensor.output.overflowed_min = Sensor_Calculate_IsOverflowed(sensor.output.percentage_min, global_struct->eeprom.user.output.output_inversion);
	sensor.output.underflowed_min = Sensor_Calculate_IsUnderflowed(sensor.output.percentage_min, global_struct->eeprom.user.output.output_inversion);


}

uint8_t Sensor_Get_Display_Min_IsOverflowed(){
	return sensor.display.overflowed_min;
}
uint8_t Sensor_Get_Display_Min_IsUnderflowed(){
	return sensor.display.underflowed_min;
}
uint8_t Sensor_Get_Display_Max_IsOverflowed(){
	return sensor.display.overflowed_max;
}
uint8_t Sensor_Get_Display_Max_IsUnderflowed(){
	return sensor.display.underflowed_max;
}

uint8_t Sensor_Get_Output_Min_IsOverflowed(){
	return sensor.output.overflowed_min;
}
uint8_t Sensor_Get_Output_Min_IsUnderflowed(){
	return sensor.output.underflowed_min;
}
uint8_t Sensor_Get_Output_Max_IsOverflowed(){
	return sensor.output.overflowed_max;
}
uint8_t Sensor_Get_Output_Max_IsUnderflowed(){
	return sensor.output.underflowed_max;
}


static void Sensor_Min_Max_Store_Process_Display(){

	//TODO オンオフディレイを実装する
	if(sensor.display.percentage_max < global_struct->ram.pressure.percentage_display_adjusted){
		sensor.display.percentage_max = global_struct->ram.pressure.percentage_display_adjusted;
	}
	if(sensor.display.percentage_min > global_struct->ram.pressure.percentage_display_adjusted){
		sensor.display.percentage_min = global_struct->ram.pressure.percentage_display_adjusted;
	}
}

static void Sensor_Min_Max_Store_Process_Output(){
	//TODO オンオフディレイを実装する
	if(sensor.output.percentage_max < global_struct->ram.pressure.percentage_output_adjusted){
		sensor.output.percentage_max = global_struct->ram.pressure.percentage_output_adjusted;
	}
	if(sensor.output.percentage_min > global_struct->ram.pressure.percentage_output_adjusted){
		sensor.output.percentage_min = global_struct->ram.pressure.percentage_output_adjusted;
	}
}

static void Sensor_Min_Max_Clear_Process(){
	if(sensor.output.reset_flag_min){
		sensor.output.percentage_min = 0;
		sensor.output.reset_flag_min = 0;
		Status_Reset_StatusCode(SC_UNDER_FLOW);
	}
	if(sensor.output.reset_flag_max){
		sensor.output.percentage_max = 0;
		sensor.output.reset_flag_max = 0;
		Status_Reset_StatusCode(SC_OVER_FLOW);
	}
	if(sensor.display.reset_flag_min){
		sensor.display.percentage_min = 0;
		sensor.display.reset_flag_min = 0;
	}
	if(sensor.display.reset_flag_max){
		sensor.display.percentage_max = 0;
		sensor.display.reset_flag_max = 0;
	}

}

void Sensor_Clear_Min_Display(){
	sensor.display.reset_flag_min = 1;
}
void Sensor_Clear_Max_Display(){
	sensor.display.reset_flag_max = 1;
}
void Sensor_Clear_Min_Output(){
	sensor.output.reset_flag_min = 1;
}
void Sensor_Clear_Max_Output(){
	sensor.output.reset_flag_max = 1;
}
void Sensor_Clear_Min_Max_Display(){
	sensor.display.reset_flag_min = 1;
	sensor.display.reset_flag_max = 1;
}

void Sensor_Clear_Min_Max_Output(){
	sensor.output.reset_flag_min = 1;
	sensor.output.reset_flag_max = 1;
}

void Sensor_Clear_Min_Max(){
	sensor.display.reset_flag_min = 1;
	sensor.display.reset_flag_max = 1;
	sensor.output.reset_flag_min = 1;
	sensor.output.reset_flag_max = 1;
}

uint8_t Sensor_Zero_Adjustment(){

	if(global_struct->ram.pressure.percentage_calibrated_output > 2000 || global_struct->ram.pressure.percentage_calibrated_output < -2000){
		Status_Set_StatusCode(SC_ZERO_ADJUSTMENT_ERROR);
		return 0;
	}
	else{
		global_struct->eeprom.sensor.zero_adjustment.output = global_struct->ram.pressure.percentage_calibrated_output;
		global_struct->eeprom.sensor.zero_adjustment.display = global_struct->ram.pressure.percentage_calibrated_display;
		global_struct->eeprom.sensor.backup_request = 1;
		return 1;
	}
}

int16_t Sensor_Get_Pressure_Percentage_Max_Display(){
	return 	sensor.display.percentage_max;
}
int16_t Sensor_Get_Pressure_Percentage_Min_Display(){
	return 	sensor.display.percentage_min;
}
int16_t Sensor_Get_Pressure_Percentage_Max_Output(){
	return 	sensor.output.percentage_max;
}
int16_t Sensor_Get_Pressure_Percentage_Min_Output(){
	return 	sensor.output.percentage_min;
}

int16_t Sensor_Get_Pressure_Percentage_Output(){
	return global_struct->ram.pressure.percentage_output_adjusted;
}
int16_t Sensor_Get_Pressure_Percentage_Display(){
	return global_struct->ram.pressure.percentage_display_adjusted;
}
int16_t Sensor_Get_Pressure_Pascal_Output(){
	return global_struct->ram.pressure.pascal_output;
}
int16_t Sensor_Get_Pressure_Pascal_Display(){
	return global_struct->ram.pressure.pascal_display;
}



//1ms
void Sensor_Process_Cyclic(){
	Sensor_Calculate_Pressure_Percentage_Display();
	Sensor_Calculate_Pressure_Percentage_Output();
	Sensor_Calculate_Pressure_Pascal_Display();
	Sensor_Calculate_Pressure_Pascal_Output();
	Sensor_Calculate_Overflow_Underflow();
	Sensor_Min_Max_Clear_Process();
	Sensor_Min_Max_Store_Process_Display();
	Sensor_Min_Max_Store_Process_Output();

	if(global_struct->ram.status_and_flags.zero_adjustment_flag){
		global_struct->ram.status_and_flags.zero_adjustment_flag = 0;
		Sensor_Zero_Adjustment();
	}

	//modbus.c
	if(global_struct->ram.status_and_flags.clear_max_min_flag){
		Sensor_Clear_Min_Max_Output();
		global_struct->ram.status_and_flags.clear_max_min_flag = 0;
	}
}

void Sensor_Init(Global_TypeDef *_global_struct){
	global_struct = _global_struct;
}
