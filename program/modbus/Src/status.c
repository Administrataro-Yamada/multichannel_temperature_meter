/*
 * error.c
 *
 *  Created on: 2022/02/24
 *      Author: staff
 */


#include "status.h"
#include "global_typedef.h"

static Global_TypeDef *global_struct;

void Status_Set_ErrorCode(ERROR_CODE error_code){
	global_struct->ram.status_and_flags.error_code |= error_code;
}

void Status_Reset_ErrorCode(ERROR_CODE error_code){
	global_struct->ram.status_and_flags.error_code &= ~error_code;
}

void Status_Clear_ErrorCode(){
	global_struct->ram.status_and_flags.error_code = EC_NO_ERROR;
}

ERROR_CODE Status_Get_ErrorCode(){
	ERROR_CODE code = global_struct->ram.status_and_flags.error_code;
	Status_Clear_ErrorCode();
	return code;
}

void Status_Set_StatusCode(STATUS_CODE status_code){
	global_struct->ram.status_and_flags.status_code |= status_code;
}

void Status_Reset_StatusCode(STATUS_CODE status_code){
	global_struct->ram.status_and_flags.status_code &= ~status_code;
}

void Status_Clear_StatusCode(){
	global_struct->ram.status_and_flags.status_code &= SC_REWRITE_LIMIT_ERROR;
}

STATUS_CODE Status_Get_StatusCode(){
	ERROR_CODE code = global_struct->ram.status_and_flags.status_code;
	Status_Clear_StatusCode();
	return code;
}

void Status_Init(Global_TypeDef *_global_struct){
	global_struct = _global_struct;
	global_struct->ram.status_and_flags.status_code = 0;
	global_struct->ram.status_and_flags.error_code = 0;
}
