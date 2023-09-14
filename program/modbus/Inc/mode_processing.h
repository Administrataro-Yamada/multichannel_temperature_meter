/*
 * mode_processing.h
 *
 *  Created on: 2020/12/11
 *      Author: staff
 */

#ifndef MODE_PROCESSING_H_
#define MODE_PROCESSING_H_


typedef enum Mode{
	MODE_PRESSURE_DISPLAYED 	= 1 << 1,
	MODE_MAIN_SETTING 			= 1 << 2,
	MODE_SUB_SETTING 			= 1 << 3,
	MODE_PRESSURE_MAX 			= 1 << 4,
	MODE_PRESSURE_MIN 			= 1 << 5,
}Mode;


typedef struct{
	Mode current_mode;
	uint8_t mode_counter;
}Mode_TypeDef;


#endif /* MODE_PROCESSING_H_ */
