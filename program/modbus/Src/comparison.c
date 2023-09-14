/*
 * Comparison.c
 *
 *  Created on: 2020/12/14
 *      Author: staff
 */

#include "comparison.h"
#include "gpio.h"
#include "global_typedef.h"
#include "compile_config.h"

#define COMPARISON_PNP_OUTPUT_1 GPIOA, GPIO_PIN_4
#define COMPARISON_NPN_OUTPUT_1 GPIOA, GPIO_PIN_5
#define COMPARISON_PNP_OUTPUT_2 GPIOA, GPIO_PIN_6
#define COMPARISON_NPN_OUTPUT_2 GPIOA, GPIO_PIN_7


#define __Comparison1_SET_PNP() HAL_GPIO_WritePin(COMPARISON_PNP_OUTPUT_1, SET)
#define __Comparison1_SET_NPN() HAL_GPIO_WritePin(COMPARISON_NPN_OUTPUT_1, SET)
#define __Comparison2_SET_PNP() HAL_GPIO_WritePin(COMPARISON_PNP_OUTPUT_2, SET)
#define __Comparison2_SET_NPN() HAL_GPIO_WritePin(COMPARISON_NPN_OUTPUT_2, SET)

#define __Comparison1_RESET_PNP() HAL_GPIO_WritePin(COMPARISON_PNP_OUTPUT_1, RESET)
#define __Comparison1_RESET_NPN() HAL_GPIO_WritePin(COMPARISON_NPN_OUTPUT_1, RESET)
#define __Comparison2_RESET_PNP() HAL_GPIO_WritePin(COMPARISON_PNP_OUTPUT_2, RESET)
#define __Comparison2_RESET_NPN() HAL_GPIO_WritePin(COMPARISON_NPN_OUTPUT_2, RESET)


static Global_TypeDef *global_struct;
static Comparison_TypeDef comparison1 = {0};
static Comparison_TypeDef comparison2 = {0};

uint8_t  Comparison_Get_Output_Transistor_State(uint8_t channel){
	if(channel == 0){
		return global_struct->ram.comparison1.output_transistor_state;
	}
	else{
		return global_struct->ram.comparison2.output_transistor_state;
	}
}

static void Comparison1_Output_State_Process(){
	//enabled 0-1:off 2:NC 3:NO
	switch(global_struct->eeprom.user.comparison1.enabled){
		case 0:
		case 1:
			global_struct->ram.comparison1.output_transistor_state = 0;
			break;
		case 2:
			global_struct->ram.comparison1.output_transistor_state = !global_struct->ram.comparison1.delayed_state;
			break;
		case 3:
			global_struct->ram.comparison1.output_transistor_state = global_struct->ram.comparison1.delayed_state;
			break;
		default:
			global_struct->ram.comparison1.output_transistor_state = 0;
			break;
	}

	//comparison1
	//SET
	if(global_struct->ram.comparison1.output_transistor_state){
		if(global_struct->eeprom.user.comparison1.output_type == 0){
			__Comparison1_RESET_NPN();
			__Comparison1_SET_PNP();
		}
		else{
			__Comparison1_RESET_PNP();
			__Comparison1_SET_NPN();
		}
	}
	//RESET
	else{
		if(global_struct->eeprom.user.comparison1.output_type == 0){
			__Comparison1_RESET_PNP();
		}
		else{
			__Comparison1_RESET_NPN();
		}
	}
}
static void Comparison2_Output_State_Process(){

	switch(global_struct->eeprom.user.comparison2.enabled){
		case 0:
		case 1:
			global_struct->ram.comparison2.output_transistor_state = 0;
			break;
		case 2:
			global_struct->ram.comparison2.output_transistor_state = !global_struct->ram.comparison2.delayed_state;
			break;
		case 3:
			global_struct->ram.comparison2.output_transistor_state = global_struct->ram.comparison2.delayed_state;
			break;
		default:
			global_struct->ram.comparison2.output_transistor_state = 0;
			break;
	}

	//comparison2
	//SET
	if(global_struct->ram.comparison2.output_transistor_state){
		if(global_struct->eeprom.user.comparison2.output_type == 0){
			__Comparison2_RESET_NPN();
			__Comparison2_SET_PNP();
		}
		else{
			__Comparison2_RESET_PNP();
			__Comparison2_SET_NPN();
		}
	}
	//RESET
	else{
		if(global_struct->eeprom.user.comparison2.output_type == 0){
			__Comparison2_RESET_PNP();
		}
		else{
			__Comparison2_RESET_NPN();
		}
	}
}


//comparison1
static void Comparison1_Hysteresis_Mode_Process(){
	if(global_struct->eeprom.user.comparison1.enabled > 1){
		if(global_struct->eeprom.user.comparison1.hys_p1 < global_struct->eeprom.user.comparison1.hys_p2){
			if(global_struct->eeprom.user.comparison1.hys_p1 > global_struct->ram.pressure.percentage_output_adjusted){
				global_struct->ram.comparison1.state = 1;
			}
			if(global_struct->eeprom.user.comparison1.hys_p2 < global_struct->ram.pressure.percentage_output_adjusted){
				global_struct->ram.comparison1.state = 0;
			}
		}
		if(global_struct->eeprom.user.comparison1.hys_p1 > global_struct->eeprom.user.comparison1.hys_p2){
			if(global_struct->eeprom.user.comparison1.hys_p1 < global_struct->ram.pressure.percentage_output_adjusted){
				global_struct->ram.comparison1.state = 1;
			}
			if(global_struct->eeprom.user.comparison1.hys_p2 > global_struct->ram.pressure.percentage_output_adjusted){
				global_struct->ram.comparison1.state = 0;
			}
		}
	}
	else{
		//output disabled
		global_struct->ram.comparison1.state = 0;
	}
}


//comparison2
static void Comparison2_Hysteresis_Mode_Process(){
	if(global_struct->eeprom.user.comparison2.enabled > 1){
		if(global_struct->eeprom.user.comparison2.hys_p1 < global_struct->eeprom.user.comparison2.hys_p2){
			if(global_struct->eeprom.user.comparison2.hys_p1 > global_struct->ram.pressure.percentage_output_adjusted){
				global_struct->ram.comparison2.state = 1;
			}
			if(global_struct->eeprom.user.comparison2.hys_p2 < global_struct->ram.pressure.percentage_output_adjusted){
				global_struct->ram.comparison2.state = 0;
			}
		}
		if(global_struct->eeprom.user.comparison2.hys_p1 > global_struct->eeprom.user.comparison2.hys_p2){
			if(global_struct->eeprom.user.comparison2.hys_p1 < global_struct->ram.pressure.percentage_output_adjusted){
				global_struct->ram.comparison2.state = 1;
			}
			if(global_struct->eeprom.user.comparison2.hys_p2 > global_struct->ram.pressure.percentage_output_adjusted){
				global_struct->ram.comparison2.state = 0;
			}
		}
	}
	else{
		//output disabled
		global_struct->ram.comparison2.state = 0;
	}
}

static void Comparison1_Window_Mode_Process(){
	if(global_struct->eeprom.user.comparison1.enabled > 1){
		int16_t margin_hi = global_struct->eeprom.user.comparison1.win_hi - 100;//1%
		int16_t margin_low = global_struct->eeprom.user.comparison1.win_low + 100;//1%

		if(margin_low < global_struct->ram.pressure.percentage_output_adjusted){
			global_struct->ram.comparison1.state = 0;
		}
		if(global_struct->eeprom.user.comparison1.win_hi < global_struct->ram.pressure.percentage_output_adjusted){
			global_struct->ram.comparison1.state = 1;
		}
		if(margin_hi > global_struct->ram.pressure.percentage_output_adjusted){
			global_struct->ram.comparison1.state = 0;
		}
		if(global_struct->eeprom.user.comparison1.win_low > global_struct->ram.pressure.percentage_output_adjusted){
			global_struct->ram.comparison1.state = 1;
		}
	}
}

static void Comparison2_Window_Mode_Process(){
	if(global_struct->eeprom.user.comparison2.enabled > 1){
		int16_t margin_hi = global_struct->eeprom.user.comparison2.win_hi - 100;//1%
		int16_t margin_low = global_struct->eeprom.user.comparison2.win_low + 100;//1%

		if(margin_low < global_struct->ram.pressure.percentage_output_adjusted){
			global_struct->ram.comparison2.state = 0;
		}
		if(global_struct->eeprom.user.comparison2.win_hi < global_struct->ram.pressure.percentage_output_adjusted){
			global_struct->ram.comparison2.state = 1;
		}
		if(margin_hi > global_struct->ram.pressure.percentage_output_adjusted){
			global_struct->ram.comparison2.state = 0;
		}
		if(global_struct->eeprom.user.comparison2.win_low > global_struct->ram.pressure.percentage_output_adjusted){
			global_struct->ram.comparison2.state = 1;
		}
	}
}



static void Comparison_Mode_Process(){
	if(global_struct->eeprom.user.comparison1.mode){
		Comparison1_Window_Mode_Process();
	}
	else{
		Comparison1_Hysteresis_Mode_Process();
	}

	if(global_struct->eeprom.user.comparison2.mode){
		Comparison2_Window_Mode_Process();
	}
	else{
		Comparison2_Hysteresis_Mode_Process();
	}
}



//100ms
static void Comparison1_Timer_On_Delay(uint8_t cyclic_time){
	static uint16_t timer_counter = 0;
	if(global_struct->ram.comparison1.state){

		if((timer_counter*cyclic_time)/100 >= global_struct->eeprom.user.comparison1.on_delay){
			global_struct->ram.comparison1.delayed_state = global_struct->ram.comparison1.state;
		}
		else{
			timer_counter++;
		}
	}
	else{
		timer_counter = 0;
	}
}

static void Comparison2_Timer_On_Delay(uint8_t cyclic_time){
	static uint16_t timer_counter = 0;
	if(global_struct->ram.comparison2.state){
		if((timer_counter*cyclic_time)/100 >= global_struct->eeprom.user.comparison2.on_delay){
			global_struct->ram.comparison2.delayed_state = global_struct->ram.comparison2.state;
		}
		else{
			timer_counter++;
		}
	}
	else{
		timer_counter = 0;
	}
}

static void Comparison1_Timer_Off_Delay(uint8_t cyclic_time){
	static uint16_t timer_counter = 0;
	if(!global_struct->ram.comparison1.state){

		if((timer_counter*cyclic_time)/100 >= global_struct->eeprom.user.comparison1.off_delay){
			global_struct->ram.comparison1.delayed_state = global_struct->ram.comparison1.state;
		}
		else{
			timer_counter++;
		}
	}
	else{
		timer_counter = 0;
	}
}

static void Comparison2_Timer_Off_Delay(uint8_t cyclic_time){
	static uint16_t timer_counter = 0;
	if(!global_struct->ram.comparison2.state){

		if((timer_counter*cyclic_time)/100 >= global_struct->eeprom.user.comparison2.off_delay){
			global_struct->ram.comparison2.delayed_state = global_struct->ram.comparison2.state;
		}
		else{
			timer_counter++;
		}
	}
	else{
		timer_counter = 0;
	}
}

static uint8_t Comparison1_Timer_Power_On_Delay(uint8_t cyclic_time){
	static uint32_t timer_counter = 0;
	static uint8_t onetime = 0;

	if(timer_counter*cyclic_time/60000 >= global_struct->eeprom.user.comparison1.power_on_delay){
		if(onetime){
			onetime = 0;
			comparison1.status.power_on_delay_finished = 1;
		}
		return 1;
	}
	else{
		timer_counter++;
		if(onetime == 0){
			onetime = 1;
			comparison1.status.power_on_delay_finished = 0;
		}
		return 0;
	}
}


static uint8_t Comparison2_Timer_Power_On_Delay(uint8_t cyclic_time){
	static uint32_t timer_counter = 0;
	static uint8_t onetime = 0;

	if(timer_counter*cyclic_time/60000 >= global_struct->eeprom.user.comparison2.power_on_delay){
		if(onetime){
			onetime = 0;
			comparison2.status.power_on_delay_finished = 1;
		}
		return 1;
	}
	else{
		timer_counter++;

		onetime = 1;
		comparison2.status.power_on_delay_finished = 0;

		return 0;
	}
}


uint8_t Comparison1_Timer_Power_On_Delay_IsFinished(){
	uint8_t b = comparison1.status.power_on_delay_finished;
	comparison1.status.power_on_delay_finished = 0;
	return b;
}

uint8_t Comparison2_Timer_Power_On_Delay_IsFinished(){
	uint8_t b = comparison2.status.power_on_delay_finished;
	comparison2.status.power_on_delay_finished = 0;
	return b;
}

static void Comparison_Timer_Process(uint8_t cyclic_time){

	if(Comparison1_Timer_Power_On_Delay(cyclic_time)){
		Comparison1_Timer_On_Delay(cyclic_time);
		Comparison1_Timer_Off_Delay(cyclic_time);
		Comparison1_Output_State_Process();
	}

	if(Comparison2_Timer_Power_On_Delay(cyclic_time)){
		Comparison2_Timer_On_Delay(cyclic_time);
		Comparison2_Timer_Off_Delay(cyclic_time);
		Comparison2_Output_State_Process();
	}

}

static void Comparison1_Detect_State_Changed(){
	static uint8_t previous_state = 0;
	if(global_struct->ram.comparison1.state != previous_state){
		previous_state = global_struct->ram.comparison1.state;
		global_struct->ram.comparison1.state_changed = 1;
	}
}

static void Comparison2_Detect_State_Changed(){
	static uint8_t previous_state = 0;
	if(global_struct->ram.comparison2.state != previous_state){
		previous_state = global_struct->ram.comparison2.state;
		global_struct->ram.comparison2.state_changed = 1;
	}
}

//10ms
void Comparison_Main_Process_Cyclic(uint8_t cyclic_time){


	if(global_struct->eeprom.system.information.customer_code == CC_WITHOUT_OUTPUT_COMPARISON_FUNCTION){

	}
	else{
		Comparison_Mode_Process();
		Comparison_Timer_Process(cyclic_time);
	}

}




//display test mode
static void Comparison1_Output_Test_Process(){
	//SET
	if(global_struct->ram.display_test_mode.leds[0]){
		if(global_struct->eeprom.user.comparison1.output_type == 0){
			__Comparison1_RESET_NPN();
			__Comparison1_SET_PNP();
		}
		else{
			__Comparison1_RESET_PNP();
			__Comparison1_SET_NPN();
		}
	}
	//RESET
	else{
		if(global_struct->eeprom.user.comparison1.output_type == 0){
			__Comparison1_RESET_PNP();
		}
		else{
			__Comparison1_RESET_NPN();
		}
	}
}

static void Comparison2_Output_Test_Process(){
	//SET
	if(global_struct->ram.display_test_mode.leds[1]){
		if(global_struct->eeprom.user.comparison2.output_type == 0){
			__Comparison2_RESET_NPN();
			__Comparison2_SET_PNP();
		}
		else{
			__Comparison2_RESET_PNP();
			__Comparison2_SET_NPN();
		}
	}
	//RESET
	else{
		if(global_struct->eeprom.user.comparison2.output_type == 0){
			__Comparison2_RESET_PNP();
		}
		else{
			__Comparison2_RESET_NPN();
		}
	}
}
void Comparison_Test_Process_Cyclic(){
	if(global_struct->eeprom.system.information.customer_code == CC_WITHOUT_OUTPUT_COMPARISON_FUNCTION){

	}
	else{
		Comparison1_Output_Test_Process();
		Comparison2_Output_Test_Process();
	}
}

void Comparison_Init(Global_TypeDef *_global_struct){

	__Comparison1_RESET_PNP();
	__Comparison1_RESET_NPN();
	__Comparison2_RESET_PNP();
	__Comparison2_RESET_NPN();

	global_struct = _global_struct;

	global_struct->ram.comparison1.channel = 0;
	global_struct->ram.comparison2.channel = 1;
	global_struct->ram.comparison1.state = 0;
	global_struct->ram.comparison2.state = 0;
	global_struct->ram.comparison1.state_changed = 0;
	global_struct->ram.comparison2.state_changed = 0;
	global_struct->ram.comparison1.delayed_state = 0;
	global_struct->ram.comparison2.delayed_state = 0;
	global_struct->ram.comparison1.output_transistor_state = 0;
	global_struct->ram.comparison2.output_transistor_state = 0;
}
