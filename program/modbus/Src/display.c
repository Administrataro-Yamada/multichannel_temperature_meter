
#include "gpio.h"
#include "display.h"
#include "global_typedef.h"
#include "compile_config.h"

//#define OFF_TIMER_START 	HAL_TIM_OC_Start_IT(htim_brightness, TIM_CHANNEL_1)
//#define OFF_TIMER_STOP 		HAL_TIM_OC_Stop_IT(htim_brightness, TIM_CHANNEL_1)
#define BRIGHTNESS_SET		__HAL_TIM_SET_COMPARE(htim_brightness, TIM_CHANNEL_1, display_io.brightness)




static Display_IO_TypeDef display_io = {0};
static Global_TypeDef *global_struct;
static TIM_HandleTypeDef *htim_brightness;


//fixed
static uint16_t Display_HEX_To_BCD(uint16_t hex){

	uint8_t dig4, dig3, dig2, dig1;

	dig4 = hex/1000;
	dig3 = (hex-dig4*1000)/100;
    dig2 = (hex-dig4*1000-dig3*100)/10;
	dig1 = hex-dig4*1000-dig3*100-dig2*10;

	return dig4<<12 | dig3<<8 | dig2<<4 | dig1;
}


static void Display_HEX_To_Segmentdata(uint16_t val, uint8_t effective_digits, uint8_t *segmentdata){

	uint8_t dig;
	if(val>>12)
		dig = 4;
	else if(val>>8)
		dig = 3;
	else if(val>>4)
		dig = 2;
	else
		dig = 1;


	for(uint8_t i = 0; i < dig; i++){
		uint8_t j = 0x0f & ( val>>(4*i) );
		segmentdata[i] = _led_num_segment_data[j];
	}

	//不要な桁削除
	for(uint8_t i = 0; i < 4-dig; i++){
		segmentdata[3-i] = 0;
	}

	//有効数字処理
	if(dig < effective_digits){
		for(uint8_t i = 0; i < effective_digits - dig; i++){
			segmentdata[1+i] = _led_num_segment_data[0];
		}
	}


}

static void Display_Effective_Digits_HEX_To_Segmentdata(uint16_t val, uint8_t effective_digits, uint8_t *segmentdata){

	for(uint8_t i = 0; i < effective_digits; i++){
		uint8_t j = 0x0f & ( val>>(4*i) );
		segmentdata[i] = _led_num_segment_data[j];
	}
	for(uint8_t i = 0; i < 4-effective_digits; i++){
		segmentdata[i + effective_digits] = 0;
	}
}


static void Display_Join_String_And_Number(uint8_t *input_string, uint8_t *input_number, uint8_t effective_digits, uint8_t *output_string){
	for(uint8_t i = 0; i < 4; i++){
		output_string[i] = input_string[i];
	}
	for(uint8_t i = 0; i < effective_digits; i++){
		output_string[i] = input_number[i];
	}
}

//fixed
static void Display_String_To_Segmentdata(uint8_t *str, uint8_t *segmentdata){
	for(uint8_t i = 0; i < 4; i++){
		if((uint8_t)'A' <= str[i] && str[i] <= (uint8_t)'Z'){
			segmentdata[i] = _led_alphabet_segment_data[( str[i] - (uint8_t)'A' )];
		}
		else if((uint8_t)'0' <= str[i] && str[i] <= (uint8_t)'9'){
			segmentdata[i] = _led_num_segment_data[( str[i] - (uint8_t)'0' )];
		}
		else{

			uint8_t data;
			switch(str[i]){
				case (uint8_t)('0'):
					data = SEGMENT_NUM_0;
					break;
				case (uint8_t)'-':
					data = SEGMENT_SYMBOL_HYPHEN;
					break;
				case (uint8_t)'_':
					data = SEGMENT_SYMBOL_UNDERBAR;
					break;
				case (uint8_t)'^':
					data = SEGMENT_SYMBOL_OVERBAR;
					break;
				case (uint8_t)' ':
					data = SEGMENT_BLANK;
					break;

				default:
					data = 0;
					break;
			}
			segmentdata[i] = data;
		}
	}
}

//fixed
static void Display_Add_Minus_Symbol(){
	for(uint8_t i = 1; i < 4; i++){
		if(display_io.digit_buffer[i] == 0){
			display_io.digit_buffer[i] = SEGMENT_SYMBOL_HYPHEN;
			break;
		}
	}
}

//fixed
static void Display_Serial_Write(uint16_t data)
{

    for(uint8_t i = 0; i < 12; i++){

    	CLK_LOW();
        DATA_LOW();
    	if ( ((data>>i) & 0x1) == 0)
            DATA_HIGH();
        else
        	DATA_LOW();

        CLK_HIGH();
    }
    DATA_HIGH();
    CLK_LOW();
    DATA_LOW();

}

//ダイナミック点灯処理
static void Display_Show(){


	if(DISPLAY_MODE_BLANK == (DISPLAY_MODE_BLANK & display_io.show_mode)){
		Display_Serial_Write(0);
	}

	else if(display_io.show_digit_counter < 4){
		D5_LOW();
		uint8_t dp[4] = {
				0,
				(display_io.dot_position>>0) & 0b1,
				(display_io.dot_position>>1) & 0b1,
				(display_io.dot_position>>2) & 0b1,
		};
		Display_Serial_Write(0b100000000000>>display_io.show_digit_counter | display_io.digit_buffer[display_io.show_digit_counter] | dp[display_io.show_digit_counter] );
	}


	//Hi_Lo_LED
	else if(display_io.show_digit_counter == 4){

		D5_HIGH();
		uint16_t hi_lo = 0b000000000000;
		if(display_io.led[0] == 1){
			hi_lo |= 0b000010000000;
		}
		if(display_io.led[1] == 1){
			hi_lo |= 0b000000100000;
		}

		Display_Serial_Write(hi_lo);

	}

	if(display_io.show_digit_counter > 5){
		display_io.show_digit_counter = 0;
	}
	else{
		display_io.show_digit_counter++;
	}
}

//for test display mode size=5
void Display_Sengment_Show_For_Display_Test_Mode(uint8_t *digits_array, uint8_t *leds){
	display_io.show_mode = DISPLAY_MODE_DISPLAY_TEST_MODE;
	display_io.dot_position = 0;
	for(uint8_t i = 0; i < 4; i++){
		display_io.digit_buffer[i] = digits_array[i];
	}
	for(uint8_t i = 0; i < 2; i++){
		display_io.led[i] = leds[i];
	}
}

uint8_t Display_Set_Default_Dot_Position(){
	display_io.dot_position = global_struct->eeprom.system.information.point_position;
	return global_struct->eeprom.system.information.point_position;
}

void Display_Set_Dot_Position(uint8_t pos){
	display_io.dot_position = pos;
}

void Display_LED_Comparison_State(uint8_t num, uint8_t state){
	if(global_struct->eeprom.system.information.customer_code != CC_WITHOUT_OUTPUT_COMPARISON_FUNCTION){
		display_io.led[num] = state;
	}
	else{

	}
}

void Display_Uint_Decimal_Show(uint16_t dec, uint8_t effective_digits){
	display_io.value = dec;
	display_io.effective_digits = effective_digits;
	display_io.show_mode &= DISPLAY_MODE_BLANK;
	display_io.show_mode |= DISPLAY_MODE_UINT_DECIMAL;
}

void Display_Int_Decimal_Show(int16_t dec, uint8_t effective_digits){
	display_io.value = dec;
	display_io.effective_digits = effective_digits;
	display_io.show_mode &= DISPLAY_MODE_BLANK;
	display_io.show_mode |= DISPLAY_MODE_INT_DECIMAL;
}

void Display_HEX_Show(uint16_t hex, uint8_t effective_digits){
	display_io.value = hex;
	display_io.effective_digits = effective_digits;
	display_io.show_mode &= DISPLAY_MODE_BLANK;
	display_io.show_mode |= DISPLAY_MODE_HEX;
}

void Display_ZeroFill_Show(){
	display_io.show_mode &= DISPLAY_MODE_BLANK;
	display_io.show_mode |= DISPLAY_MODE_ZERO_FILL;
}
void Display_Error_Show(){
	display_io.show_mode &= DISPLAY_MODE_BLANK;
	display_io.show_mode |= DISPLAY_MODE_ERROR;
}

void Display_String_Show_Without_Dot(uint8_t *alphabet){
	for(uint8_t i = 0; i < 4; i++){
		display_io.str_buffer[i] = alphabet[3-i];
	}
	display_io.show_mode &= DISPLAY_MODE_BLANK;
	display_io.show_mode |= DISPLAY_MODE_STRING_WITHOUT_DOT;
}

void Display_String_Show(uint8_t *alphabet){
	for(uint8_t i = 0; i < 4; i++){
		display_io.str_buffer[i] = alphabet[3-i];
	}
	display_io.show_mode &= DISPLAY_MODE_BLANK;
	display_io.show_mode |= DISPLAY_MODE_STRING;
}

void Display_Top_String_And_HEX_Show(uint8_t *top_str, uint8_t effective_digits, uint8_t hex){
	for(uint8_t i = 0; i < 4; i++){
		display_io.str_buffer[i] = top_str[3-i];
	}
	display_io.value = hex;
	display_io.effective_digits = effective_digits;
	display_io.show_mode &= DISPLAY_MODE_BLANK;
	display_io.show_mode |= DISPLAY_MODE_TOP_STRING_AND_HEX;
}

void Display_Top_String_And_Uint_Decimal_Show(uint8_t *top_str, uint8_t effective_digits, uint16_t dec){
	for(uint8_t i = 0; i < 4; i++){
		display_io.str_buffer[i] = top_str[3-i];
	}
	display_io.value = dec;
	display_io.effective_digits = effective_digits;
	display_io.show_mode &= DISPLAY_MODE_BLANK;
	display_io.show_mode |= DISPLAY_MODE_TOP_STRING_AND_UINT_DECIMAL;
}

void Display_Set_Brightness(uint8_t brightness){
	display_io.brightness = brightness;
	global_struct->eeprom.user.display.brightness = brightness;
	global_struct->eeprom.user.backup_request = 1;
	BRIGHTNESS_SET;
}

void Display_Set_Brightness_Without_Backup(uint8_t brightness){
	display_io.brightness = brightness;
	BRIGHTNESS_SET;
}



void Display_Set_Eco_Brightness(){
	Display_Set_Brightness_Without_Backup(5);
}

void Display_Set_Default_Brightness(){
		Display_Set_Brightness_Without_Backup(global_struct->eeprom.user.display.brightness);
}

void Display_Enabled(uint8_t b){
	if(b){
		display_io.show_mode &= ~DISPLAY_MODE_BLANK;
	}
	else{
		display_io.show_mode |= DISPLAY_MODE_BLANK;
	}
}

static uint8_t Display_IsEnabled(){
	return DISPLAY_MODE_BLANK != (DISPLAY_MODE_BLANK & display_io.show_mode);
}

void Display_Lit_Config(uint16_t period_ms){
	display_io.lit.period_ms = period_ms;
}

void Display_Lit_Start(){
	if(display_io.lit.enabled == 0){
		display_io.lit.enabled = 1;
		display_io.lit.finished = 0;
	}
}

void Display_Lit_Stop(){
	if(display_io.lit.enabled){
		display_io.lit.enabled = 0;
		display_io.lit.finished = 0;
	}
}
uint8_t Display_Lit_IsOnGoing(){
	return display_io.lit.enabled;
}

uint8_t Display_Lit_IsFinished(){
	uint8_t finished = display_io.lit.finished;
	display_io.lit.finished = 0;
	return finished;
}

//点灯処理
static void Display_Lit_Process(){
	static uint16_t lit_timer = 0;
	if(display_io.lit.enabled){
		if(display_io.lit.period_ms == lit_timer){
			lit_timer = 0;
			display_io.lit.enabled = 0;
			display_io.lit.finished = 1;
		}
		else{
			lit_timer++;
		}
	}
	else{
		display_io.lit.enabled = 0;
		lit_timer = 0;
	}
}


void Display_Blink_Config(uint16_t period_ms, uint8_t times){
	/*
	 * period_ms : blink period
	 * times : blink times. 0xff -> infinity
	 */
	display_io.blink.period_ms = period_ms;
	display_io.blink.times = times;
}

void Display_Blink_Start(){
	if(display_io.blink.enabled == 0)
		display_io.blink.enabled = 1;
}

void Display_Blink_Stop(){
	if(display_io.blink.enabled){
		display_io.blink.enabled = 0;
		Display_Enabled(1);
	}
}
uint8_t Display_Blink_IsOnGoing(){
	return display_io.blink.enabled;
}

uint8_t Display_Blink_IsFinished(){
	uint8_t finished = display_io.blink.finished;
	display_io.blink.finished = 0;
	return finished;
}

//点滅処理
static void Display_Blink_Process(){
	if(display_io.blink.enabled){
		static uint16_t blink_timer = 0;
		if(display_io.blink.times){
			if(display_io.blink.period_ms <= blink_timer){
				if(Display_IsEnabled())
					Display_Enabled(0);
				else
					Display_Enabled(1);

				//0xff -> infinity
				if(display_io.blink.times == 0xff){
				}
				else if(display_io.blink.times > 0){
					display_io.blink.times--;
				}

				blink_timer = 0;
			}
			else{
				blink_timer++;
			}
		}
		else{
			blink_timer = 0;
			display_io.blink.enabled = 0;
			display_io.blink.finished = 1;
			Display_Enabled(1);
		}
	}
}


void Display_ProcessBar_Start(){
	if(display_io.process_bar.enabled == 0){
		display_io.process_bar.counter = 0;
		display_io.process_bar.enabled = 1;
	}
}

void Display_ProcessBar_Stop(){
	if(display_io.process_bar.enabled){
		display_io.process_bar.counter = 0;
		display_io.process_bar.enabled = 0;
	}
}

uint8_t Display_ProcessBar_IsOnGoing(){
	return 	display_io.process_bar.enabled;
}

//プロセスバー
static void Display_ProcessBar_Process(){
	static uint16_t process_timer = 0;

	if(display_io.process_bar.enabled){
		Display_Set_Dot_Position(0);
		if(display_io.process_bar.counter < 3){
			Display_String_Show_Without_Dot(_led_process_bar[display_io.process_bar.counter]);
		}
		else{
			process_timer = 0;
			return;
		}

		//500ms
		if(50 <= process_timer){
			process_timer = 0;
			display_io.process_bar.counter++;
		}
		else{
			process_timer++;
		}


	}
	else{
		display_io.process_bar.counter = 0;
		process_timer = 0;
	}
}




/*10ms thread*/
void Display_Process_Cyclic(){

	uint16_t bcd;
	uint8_t _num_segment[4] = {0};
	uint8_t _string_segment[4] = {0};

	Display_ProcessBar_Process();
	Display_Blink_Process();
	Display_Lit_Process();

//	if(!Display_IsEnabled()){
//		for(uint8_t i = 0; i < 4; i++)
//			display_io.digit_buffer[i] = 0;
//
//		return;
//	}
	if(Display_ProcessBar_IsOnGoing()){
		Display_Set_Dot_Position(0);
		Display_String_To_Segmentdata(display_io.str_buffer, display_io.digit_buffer);
		return;
	}
	else{

		switch(display_io.show_mode){

			case DISPLAY_MODE_STRING_WITHOUT_DOT:
				Display_Set_Dot_Position(0);
				Display_String_To_Segmentdata(display_io.str_buffer, display_io.digit_buffer);
				break;

			case DISPLAY_MODE_STRING:
				Display_String_To_Segmentdata(display_io.str_buffer, display_io.digit_buffer);
				break;

			case DISPLAY_MODE_ZERO_FILL:
				//Display_String_Show();
				Display_String_To_Segmentdata((uint8_t*)"000 ", display_io.digit_buffer);
				Display_Set_Dot_Position(0);
				break;

			case DISPLAY_MODE_ERROR:
				//Display_String_Show();
				Display_String_To_Segmentdata((uint8_t*)"RRE ", display_io.digit_buffer);
				Display_Set_Dot_Position(0);
				break;

			case DISPLAY_MODE_HEX:
				Display_HEX_To_Segmentdata(display_io.value, display_io.effective_digits, display_io.digit_buffer);
				break;


			case DISPLAY_MODE_INT_DECIMAL:
				if(display_io.value >= 0){
					bcd = Display_HEX_To_BCD(display_io.value);
					Display_HEX_To_Segmentdata(bcd, display_io.effective_digits, display_io.digit_buffer);
				}
				else{
					bcd = Display_HEX_To_BCD((int16_t)-display_io.value);
					Display_HEX_To_Segmentdata(bcd, display_io.effective_digits, display_io.digit_buffer);
					Display_Add_Minus_Symbol();
				}
				break;

			case DISPLAY_MODE_UINT_DECIMAL:
				bcd = Display_HEX_To_BCD(display_io.value);
				Display_HEX_To_Segmentdata(bcd, display_io.effective_digits, display_io.digit_buffer);
				break;

			case DISPLAY_MODE_TOP_STRING_AND_HEX:
				Display_Effective_Digits_HEX_To_Segmentdata(display_io.value, display_io.effective_digits, _num_segment);
				Display_String_To_Segmentdata(display_io.str_buffer, _string_segment);
				Display_Join_String_And_Number(_string_segment, _num_segment, display_io.effective_digits, display_io.digit_buffer);
				break;

			case DISPLAY_MODE_TOP_STRING_AND_UINT_DECIMAL:
				bcd = Display_HEX_To_BCD(display_io.value);
				Display_Effective_Digits_HEX_To_Segmentdata(bcd, display_io.effective_digits, _num_segment);
				Display_String_To_Segmentdata(display_io.str_buffer, _string_segment);
				Display_Join_String_And_Number(_string_segment, _num_segment, display_io.effective_digits, display_io.digit_buffer);
				break;

			case DISPLAY_MODE_DISPLAY_TEST_MODE:
				break;
			default:
				for(uint8_t i = 0; i < 4; i++)
					display_io.digit_buffer[i] = 0;
				break;
		}
	}
}


void __Display_Clear()
{
    DATA_HIGH();
    for(uint8_t i = 0; i < 12; i++){
    	CLK_LOW();
        CLK_HIGH();
    }
    DATA_HIGH();
    CLK_LOW();
    DATA_LOW();
}

//1msのタイマ割り込みが入る。ダイナミック点灯処理
void Display_Dynamic_Cyclic(void){

	Display_Show();

}

//ダイナミック点灯と同じタイマで、しきい値で割り込みが入る。消灯処理
void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim){

	if(htim->Instance == htim_brightness->Instance){
		D5_LOW();
		Display_Serial_Write(0);
	}

}

void HAL_TIM_ErrorCallback(TIM_HandleTypeDef *htim){

	if (htim->Instance == htim_brightness->Instance) {
		HAL_TIM_Base_Stop_IT(htim);
		HAL_TIM_Base_Stop(htim);
		htim->Instance->SR = 0; //FLAG CLEAR
		HAL_TIM_Base_Start_IT(htim);
		HAL_TIM_OC_Start_IT(htim, TIM_CHANNEL_1);
	}
}




void Display_Init(Global_TypeDef *_global_struct, TIM_HandleTypeDef *_htim_brightness){
	global_struct = _global_struct;
	htim_brightness = _htim_brightness;

	Display_Enabled(1);
	Display_Set_Default_Brightness();

	Display_Int_Decimal_Show(0, 1);
	HAL_TIM_Base_Start_IT(_htim_brightness);
	HAL_TIM_OC_Start_IT(_htim_brightness, TIM_CHANNEL_1);
	Display_Blink_Config(25, 3);
	Display_Set_Default_Dot_Position();
	display_io.process_bar.counter = 0;
	display_io.process_bar.enabled = 0;

}



