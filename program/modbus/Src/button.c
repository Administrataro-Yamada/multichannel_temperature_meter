/*
 * button.c
 *
 *  Created on: 2020/12/09
 *      Author: staff
 */

#include "gpio.h"
#include "button.h"
#include "global_typedef.h"

static Global_TypeDef *global_struct;
static Button_TypeDef button = {0};

static uint8_t Button_Is_Pressed_Any_Buttons(){
	return global_struct->ram.button.now.ent
			|| global_struct->ram.button.now.up
			|| global_struct->ram.button.now.down
			|| global_struct->ram.button.now.mode;
}

static uint8_t Button_Is_Pressed_All_Buttons(){
	return global_struct->ram.button.now.ent
			&& global_struct->ram.button.now.up
			&& global_struct->ram.button.now.down
			&& global_struct->ram.button.now.mode;
}

static void Button_Clear_State_Continuously_Pressed_Buttons(){
	global_struct->ram.button.continuously.mode = 0;
	global_struct->ram.button.continuously.up = 0;
	global_struct->ram.button.continuously.down = 0;
	global_struct->ram.button.continuously.ent = 0;
	global_struct->ram.button.continuously.all = 0;
	global_struct->ram.button.continuously.any = 0;
}

static void Button_Clear_State_Very_Long_Pressed_Buttons(){
	global_struct->ram.button.very_long.mode = 0;
	global_struct->ram.button.very_long.up = 0;
	global_struct->ram.button.very_long.down = 0;
	global_struct->ram.button.very_long.ent = 0;
	global_struct->ram.button.very_long.all = 0;
	global_struct->ram.button.very_long.any = 0;
}


void Button_Clear_State_Clicked_Buttons(){
	global_struct->ram.button.clicked.mode = 0;
	global_struct->ram.button.clicked.up = 0;
	global_struct->ram.button.clicked.down = 0;
	global_struct->ram.button.clicked.ent = 0;
	global_struct->ram.button.clicked.all = 0;
	global_struct->ram.button.clicked.any = 0;
}

static void Button_Clear_State_Released_Buttons(){
	global_struct->ram.button.released.mode = 0;
	global_struct->ram.button.released.up = 0;
	global_struct->ram.button.released.down = 0;
	global_struct->ram.button.released.ent = 0;
	global_struct->ram.button.released.all = 0;
	global_struct->ram.button.released.any = 0;
}

static void Button_Store_State_Continuously_Pressed_Buttons(){
	global_struct->ram.button.continuously.mode = global_struct->ram.button.now.mode;
	global_struct->ram.button.continuously.up = global_struct->ram.button.now.up;
	global_struct->ram.button.continuously.down = global_struct->ram.button.now.down;
	global_struct->ram.button.continuously.ent = global_struct->ram.button.now.ent;
	global_struct->ram.button.continuously.all = global_struct->ram.button.now.all;
	global_struct->ram.button.continuously.any = global_struct->ram.button.now.any;
}

static void Button_Store_State_Very_Long_Pressed_Buttons(){
	global_struct->ram.button.very_long.mode = global_struct->ram.button.now.mode;
	global_struct->ram.button.very_long.up = global_struct->ram.button.now.up;
	global_struct->ram.button.very_long.down = global_struct->ram.button.now.down;
	global_struct->ram.button.very_long.ent = global_struct->ram.button.now.ent;
	global_struct->ram.button.very_long.all = global_struct->ram.button.now.all;
	global_struct->ram.button.very_long.any = global_struct->ram.button.now.any;
}

static void Button_Store_State_Released_Buttons(){
	global_struct->ram.button.released.mode = global_struct->ram.button.now.mode;
	global_struct->ram.button.released.up = global_struct->ram.button.now.up;
	global_struct->ram.button.released.down = global_struct->ram.button.now.down;
	global_struct->ram.button.released.ent = global_struct->ram.button.now.ent;
	global_struct->ram.button.released.all = global_struct->ram.button.now.all;
	global_struct->ram.button.released.any = global_struct->ram.button.now.any;
}

static void Button_Store_State_Clicked_Buttons(){
	global_struct->ram.button.clicked.mode = global_struct->ram.button.released.mode;
	global_struct->ram.button.clicked.up = global_struct->ram.button.released.up;
	global_struct->ram.button.clicked.down = global_struct->ram.button.released.down;
	global_struct->ram.button.clicked.ent = global_struct->ram.button.released.ent;
	global_struct->ram.button.clicked.all = global_struct->ram.button.released.all;
	global_struct->ram.button.clicked.any = global_struct->ram.button.released.any;
}

static void Button_Get_Now_Pressed_Buttons(){
	global_struct->ram.button.now.mode = __Button_Is_Mode_Pressed();
	global_struct->ram.button.now.up = __Button_Is_Up_Pressed();
	global_struct->ram.button.now.down = __Button_Is_Down_Pressed();
	global_struct->ram.button.now.ent = __Button_Is_Ent_Pressed();
	global_struct->ram.button.now.any = Button_Is_Pressed_Any_Buttons();
	global_struct->ram.button.now.all = Button_Is_Pressed_All_Buttons();
}

void Button_Enabled(uint8_t b){
	global_struct->ram.button.enabled = b;
}

RAM_Button_Clicked Button_Clicked_State(uint8_t clear_state){
	RAM_Button_Clicked state = global_struct->ram.button.clicked;
	if(clear_state)
		Button_Clear_State_Clicked_Buttons();
	return state;
}

static void Button_Process_Continuously_Pressed_Detection(){

	Button_Get_Now_Pressed_Buttons();

	static uint16_t counter = 0;

	if(global_struct->ram.button.now.any){

		Button_Store_State_Released_Buttons();

		//500ms
		if(counter >= 50){
			Button_Store_State_Continuously_Pressed_Buttons();
			return;
		}
		else{
			counter++;
		}
	}
	else{
		if(global_struct->ram.button.very_long.any){
			//長押し後の場合はクリックではない
			Button_Clear_State_Clicked_Buttons();
		}
		else{
			//チャタリングを加味して 50ms以上でクリック判定
			if(counter > 5){
				Button_Store_State_Clicked_Buttons();
			}
		}
		counter = 0;
		Button_Clear_State_Continuously_Pressed_Buttons();
	}
}


static void Button_Process_Very_Long_Pressed_Detection(){
	static uint16_t counter = 0;
	if(global_struct->ram.button.continuously.any){
		if(counter == 150){
			Button_Store_State_Very_Long_Pressed_Buttons();
		}
		else{
			counter++;
		}
	}
	else{
		Button_Clear_State_Very_Long_Pressed_Buttons();
		counter = 0;
	}
}

//10ms
static void Button_Process_Increment_Speed(){
	static uint16_t counter = 0;
	if(global_struct->ram.button.continuously.up || global_struct->ram.button.continuously.down){

		switch(counter){
			case 50:
				button.speed_flag.speed = INCREMENT_SLOW;
				break;
			case 450:
				button.speed_flag.speed = INCREMENT_MID;
				break;
			case 850:
				button.speed_flag.speed = INCREMENT_FAST;
				break;
			case 1250:
				button.speed_flag.speed = INCREMENT_FASTER;
				break;
			default:
				break;
		}

		if(counter < 1500){
			counter++;
		}

	}
	else{
		button.speed_flag.speed = INCREMENT_STOP;
		counter = 0;
		button.speed_flag.counter = 0;
	}
}

//10ms
int8_t Button_Get_Increment_Value_Cyclic(uint8_t cyclic_period_ms){
	int8_t val = 0;

	switch(button.speed_flag.speed){

		// 250msのうち、0msのときに値を返す
		case INCREMENT_SLOW:
			if(button.speed_flag.counter == 0){
				val = 1;
			}
			break;

		// 250msのうち、0msと125msのときに値を返す(slowに対して倍の頻度)
		case INCREMENT_MID:
			if(button.speed_flag.counter == 0 || button.speed_flag.counter == 125/cyclic_period_ms){
				val = 1;
			}
			break;


		case INCREMENT_FAST:
			if(button.speed_flag.counter == 0){
				val = 15;
			}
			break;

		case INCREMENT_FASTER:
			if(button.speed_flag.counter == 0 || button.speed_flag.counter == 125/cyclic_period_ms){
				val = 15;
			}
			break;

		default:
			break;
	}


	if(button.speed_flag.counter == 250/cyclic_period_ms){
		button.speed_flag.counter = 0;
	}
	else{
		button.speed_flag.counter++;
	}

	if(global_struct->ram.button.continuously.up){
		return val;
	}
	else if(global_struct->ram.button.continuously.down){
		return -val;
	}

}


void Button_Maintenunce_Mode(){
	Button_Get_Now_Pressed_Buttons();
	if(Button_Is_Pressed_All_Buttons()){
		global_struct->eeprom.system.information.maintenance_mode = 1;
		global_struct->eeprom.user.communication.slave_address = 1;
		global_struct->eeprom.user.communication.baud_rate = 8;
		global_struct->eeprom.backup_request_all = 1;
	}
}

//10ms
void Button_Process_Cyclic(){
	if(global_struct->ram.button.enabled){
		Button_Process_Continuously_Pressed_Detection();
		Button_Process_Very_Long_Pressed_Detection();
		Button_Process_Increment_Speed();
	}
}

void Button_Init(Global_TypeDef *_global_struct){
	global_struct = _global_struct;

	Button_Clear_State_Clicked_Buttons();
	Button_Clear_State_Continuously_Pressed_Buttons();
	Button_Clear_State_Very_Long_Pressed_Buttons();
	Button_Clear_State_Released_Buttons();



}


