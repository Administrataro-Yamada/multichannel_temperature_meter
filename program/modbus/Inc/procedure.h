/*
 * procedure.h
 *
 *  Created on: 2021/12/15
 *      Author: staff
 */

#ifndef PROCEDURE_H_
#define PROCEDURE_H_

typedef struct{
	uint16_t counter;
	uint16_t timeout_time;
	uint8_t timeout_flag : 1;
	uint8_t enabled : 1;
	uint8_t timeout_paused : 1;

}Procedure_Timeout_Timer_TypeDef;



typedef struct{
	uint16_t cyclic_period_ms;
}Procedure_Status_TypeDef;


typedef struct{
	Procedure_Status_TypeDef status;
	Procedure_Timeout_Timer_TypeDef timeout_timer;
}Procedure_TypeDef;
#endif /* PROCEDURE_H_ */
