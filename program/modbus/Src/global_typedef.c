/*
 * global_typedef.c
 *
 *  Created on: 2021/10/26
 *      Author: staff
 */

#include "global_typedef.h"
#include "string.h"
volatile static Global_TypeDef global_struct;// __attribute__ ((aligned(4)));
Global_TypeDef* Global_Init(){

	//memset(&global_struct, 0, sizeof(global_struct));


	//mapping
	global_struct.eeprom.user.user_information.pressure_range_id = &(global_struct.eeprom.system.information.pressure_range_id);
	global_struct.eeprom.user.user_information.production_number = &(global_struct.eeprom.system.information.production_number);
	global_struct.ram.comparison1.setting = &(global_struct.eeprom.user.comparison1);
	global_struct.ram.comparison2.setting = &(global_struct.eeprom.user.comparison2);

	return &global_struct;
}
