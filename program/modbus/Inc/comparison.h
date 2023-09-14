#ifndef COMPARATOR_H_
#define COMPARATOR_H_

#include "main.h"

typedef struct{
	uint8_t power_on_delay_finished : 1;
}Comparison_Status_TypeDef;

typedef struct{
	Comparison_Status_TypeDef status;
}Comparison_TypeDef;




#endif /* COMPARATOR_H_ */
