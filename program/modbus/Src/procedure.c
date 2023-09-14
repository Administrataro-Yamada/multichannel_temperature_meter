/*
 * procedure.c
 *
 *  Created on: 2021/12/15
 *      Author: staff
 */

#include "main.h"
#include "procedure.h"

static Procedure_TypeDef procedure = {};



uint8_t Procedure_Timeout_Timer_IsTimeout(){
	uint8_t timeout_flag = procedure.timeout_timer.timeout_flag;
	procedure.timeout_timer.timeout_flag = 0;
	return timeout_flag;
}

uint8_t  Procedure_Timeout_Timer_IsOnGoing(){
	return procedure.timeout_timer.enabled;
}
void Procedure_Timeout_Timer_Stop(){
	if(procedure.timeout_timer.enabled){
		procedure.timeout_timer.timeout_flag = 0;
		procedure.timeout_timer.counter = 0;
		procedure.timeout_timer.timeout_paused = 0;
		procedure.timeout_timer.enabled = 0;
	}
}
void Procedure_Timeout_Timer_Start(){
	if(procedure.timeout_timer.enabled == 0){
		procedure.timeout_timer.timeout_flag = 0;
		procedure.timeout_timer.counter = 0;
		procedure.timeout_timer.timeout_paused = 0;
		procedure.timeout_timer.enabled = 1;
	}
}

void Procedure_Timeout_Timer_Paused(){
	procedure.timeout_timer.timeout_paused = 1;
}
void Procedure_Timeout_Timer_ReStart(){
	procedure.timeout_timer.timeout_paused = 0;
}

void Procedure_Timeout_Timer_Reload(){
	procedure.timeout_timer.counter = 0;
}

static void Procedure_Timeout_Timer_Process(uint8_t cyclic_period_ms){
	if(procedure.timeout_timer.timeout_paused){
		return;
	}

	if(procedure.timeout_timer.enabled){
		if(procedure.timeout_timer.counter*(uint16_t)cyclic_period_ms >= procedure.timeout_timer.timeout_time){
			procedure.timeout_timer.timeout_flag = 1;
			procedure.timeout_timer.enabled = 0;
			procedure.timeout_timer.counter = 0;
		}
		else{
			procedure.timeout_timer.timeout_flag = 0;
			procedure.timeout_timer.counter++;
		}
	}
	else{
		procedure.timeout_timer.counter = 0;
		return;
	}
}


void Procedure_Timeout_Timer_Config(uint16_t timeout_time_ms){
	procedure.timeout_timer.timeout_time = timeout_time_ms;
}





void Procedure_Process_Cyclic(uint8_t cyclic_period_ms){
	procedure.status.cyclic_period_ms = cyclic_period_ms;
	Procedure_Timeout_Timer_Process(cyclic_period_ms);
}




