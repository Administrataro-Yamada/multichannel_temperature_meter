/*
 * display_app.h
 *
 *  Created on: 2021/12/08
 *      Author: oki
 */

#ifndef DISPLAY_APP_H_
#define DISPLAY_APP_H_

static const uint8_t setting_option_modbus_address[] = "A";
static const uint8_t *setting_option_modbus_baudrate[] = {" F02", " F10", " F20", " F40"};
static const uint8_t *setting_option_filter_output[] = {" F02", " F10", " F20", " F40"};
static const uint8_t *setting_option_sign_inversion_display[] = {" off", "  on"};

typedef enum{
	ANIMATION_FULLSCALE_MINUS,
	ANIMATION_FULLSCALE_PLUS,
	ANIMATION_VERSION,
	ANIMATION_LIGHTS_ALL,
	ANIMATION_LIGHTS_DISAPPEARING,
	ANIMATION_DISPLAY_OFF,
	ANIMATION_DISPLAY_ON,
	ANIMATION_FINISH,
}DISPLAY_OPENING_ANIMATION;

typedef enum{
	MODE_PARENT_OPENING,
	MODE_PARENT_MAIN,
	MODE_PARENT_SUB,
}DISPLAY_APP_MODE_PARENT;

typedef enum{
	MODE_MAIN_PRESSURE,
	MODE_MAIN_ZERO_ADJUSTMENT,
	MODE_MAIN_LOCK_UNLOCK,
	MODE_MAIN_MIN,
	MODE_MAIN_MAX,
	MODE_MAIN_HYS_1P1,
	MODE_MAIN_HYS_1P2,
	MODE_MAIN_HYS_2P1,
	MODE_MAIN_HYS_2P2,
	MODE_MAIN_WIN_1HI,
	MODE_MAIN_WIN_1LO,
	MODE_MAIN_WIN_2HI,
	MODE_MAIN_WIN_2LO,
}DISPLAY_APP_MODE_MAIN;

typedef enum{
	MODE_SUB_ADDRESS,
	MODE_SUB_BAUDRATE,

	MODE_SUB_COMPARISON1_FUNCTION,
	MODE_SUB_COMPARISON1_OUTPUT,
	MODE_SUB_COMPARISON1_ON_DELAY,
	MODE_SUB_COMPARISON1_OFF_DELAY,
	MODE_SUB_COMPARISON1_POWER_ON_DELAY,

	MODE_SUB_COMPARISON2_FUNCTION,
	MODE_SUB_COMPARISON2_OUTPUT,
	MODE_SUB_COMPARISON2_ON_DELAY,
	MODE_SUB_COMPARISON2_OFF_DELAY,
	MODE_SUB_COMPARISON2_POWER_ON_DELAY,

	MODE_SUB_FILTER_DISPLAY,
	MODE_SUB_FILTER_OUTPUT,
	MODE_SUB_LOW_CUT_DISPLAY,
	MODE_SUB_SIGN_INVERSION_DISPLAY,
	MODE_SUB_SIGN_INVERSION_OUTPUT,
	MODE_SUB_ECO,
	MODE_SUB_TEST,

	MODE_SUB_FACTORY_RESET,
}DISPLAY_APP_MODE_SUB;

typedef struct{
	DISPLAY_APP_MODE_PARENT parent;
	DISPLAY_APP_MODE_MAIN main;
	DISPLAY_APP_MODE_SUB sub;
	uint8_t setting_mode : 1;
}Display_App_Mode_State_TypeDef;

typedef struct{
	uint8_t transition_very_long_pressed : 1;
	uint8_t transitioned_very_long_pressed : 1;
	uint8_t transition_clicked : 1;
	uint8_t confirmed : 1;
	uint8_t increment : 1;
	uint8_t decrement : 1;
}Display_App_Flags_TypeDef;

typedef struct{
	Display_App_Flags_TypeDef flags;
	Display_App_Mode_State_TypeDef mode_state;
}Display_App_Status_TypeDef;


#endif /* DISPLAY_APP_H_ */
