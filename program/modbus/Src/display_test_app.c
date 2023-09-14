
#include "main.h"
#include "global_typedef.h"
#include "display.h"

static Global_TypeDef *global_struct;


void Display_Test_App_Process_Cyclic(uint16_t cyclic_period_ms){
	Button_Enabled(1);
	Display_Set_Default_Brightness();
	Display_Sengment_Show_For_Display_Test_Mode(global_struct->ram.display_test_mode.segments, global_struct->ram.display_test_mode.leds);
}


void Display_Test_App_Init(Global_TypeDef *_global_struct){
	global_struct = _global_struct;
}
