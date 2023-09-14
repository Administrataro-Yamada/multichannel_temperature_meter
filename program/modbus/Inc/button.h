#ifndef BUTTON_H_
#define BUTTON_H_




//PIN Defines
#define BUTTON_MODE GPIOB, GPIO_PIN_5
#define BUTTON_UP GPIOB, GPIO_PIN_6
#define BUTTON_DOWN GPIOB, GPIO_PIN_7
#define BUTTON_ENT GPIOB, GPIO_PIN_8



#define __Button_Is_Mode_Pressed()		(!HAL_GPIO_ReadPin(BUTTON_MODE))
#define __Button_Is_Up_Pressed() 		(!HAL_GPIO_ReadPin(BUTTON_UP))
#define __Button_Is_Down_Pressed()		(!HAL_GPIO_ReadPin(BUTTON_DOWN))
#define __Button_Is_Ent_Pressed() 		(!HAL_GPIO_ReadPin(BUTTON_ENT))

typedef enum{
	INCREMENT_STOP,
	INCREMENT_SLOW,
	INCREMENT_MID,
	INCREMENT_FAST,
	INCREMENT_FASTER,
}Button_Increment_Speed;

typedef struct{
	Button_Increment_Speed speed;
	uint8_t counter;
}Button_Increment_Speed_Flag;


typedef struct{
	Button_Increment_Speed_Flag speed_flag;
}Button_TypeDef;

#endif /* BUTTON_H_ */
