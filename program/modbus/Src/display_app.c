/*
 * display_app.c
 *
 *  Created on: 2021/11/01
 *      Author: staff
 */


#include "global_typedef.h"
#include "display_app.h"
#include "display.h"
#include "button.h"
#include "compile_config.h"


static Global_TypeDef *global_struct;
static Display_App_Status_TypeDef display_app_status = {0};

//メンテするとき注意
static int32_t Display_App_Mode_Sub_Setting_Factory_Reset(int32_t setting_value_origin);



// "---"を表示するかどうかの判定(圧力表示モード)
static uint8_t Display_App_ProcessBar_Branch_Main(){
	uint8_t b = 0;
	switch(display_app_status.mode_state.main){
		case MODE_MAIN_PRESSURE:
			if(global_struct->ram.button.continuously.mode){
				b = 1;
			}
			else if(global_struct->ram.button.continuously.up && global_struct->ram.button.continuously.down){
				b = 1;
			}
			else{
				b = 0;
			}
			break;



		case MODE_MAIN_MIN:
		case MODE_MAIN_MAX:
		case MODE_MAIN_HYS_1P1:
		case MODE_MAIN_HYS_1P2:
		case MODE_MAIN_HYS_2P1:
		case MODE_MAIN_HYS_2P2:
		case MODE_MAIN_WIN_1HI:
		case MODE_MAIN_WIN_1LO:
		case MODE_MAIN_WIN_2HI:
		case MODE_MAIN_WIN_2LO:
			if(global_struct->ram.button.continuously.mode){
				b = 1;
			}
			break;

		default:
			break;
	}
	return b;
}

// "---"を表示するかどうかの判定(サブ設定モード)
static uint8_t Display_App_ProcessBar_Branch_Sub(){
	uint8_t b = 0;

	if(global_struct->ram.button.continuously.mode){
		b = 1;
	}
	else{
		switch(display_app_status.mode_state.sub){
			case MODE_SUB_ADDRESS:
				break;

			case MODE_SUB_BAUDRATE:
				break;

			default:
				break;
		}
	}
	return b;
}

// "---"を表示するかどうかの判定
static uint8_t Display_App_ProcessBar_Branch_Parent(){
	uint8_t b = 0;
	switch(display_app_status.mode_state.parent){
		case MODE_PARENT_MAIN:
			b = Display_App_ProcessBar_Branch_Main();
			break;

		case MODE_PARENT_SUB:
			b = Display_App_ProcessBar_Branch_Sub();
			break;

		default:
			b = 0;
			break;
	}
	return b;
}


//モード遷移先(圧力表示モード)
static void Display_App_Transition_State_Branch_Main(){

	uint8_t next_mode_transision = 0;
	uint8_t next_up_transision = 0;
	uint8_t next_down_transision = 0;
	//クリックするたびにここに入る
	if(display_app_status.flags.transition_clicked){
		//mode
		if(global_struct->ram.button.clicked.mode){
			next_mode_transision = 1;//次のモードに遷移するフラグ
			display_app_status.mode_state.setting_mode = 0;
			display_app_status.flags.increment = 0;
			display_app_status.flags.decrement = 0;
			display_app_status.flags.confirmed = 0;
		}
		//ent
		else if(global_struct->ram.button.clicked.ent){
			if((display_app_status.mode_state.main == MODE_MAIN_MIN) || (display_app_status.mode_state.main == MODE_MAIN_MAX)){
				display_app_status.flags.confirmed = 1;
			}
			else{
				if(display_app_status.mode_state.setting_mode == 0){
					display_app_status.mode_state.setting_mode = 1;
				}
				else{
					display_app_status.flags.confirmed = 1;//決定ボタンのフラグ
				}
			}
		}
		//up
		else if(global_struct->ram.button.clicked.up){
			next_up_transision = 1;
			if(display_app_status.mode_state.setting_mode){
				display_app_status.flags.increment = 1;//設定に入っている状態でup/downを押したときのフラグ(下で回収している)
			}
		}
		//down
		else if(global_struct->ram.button.clicked.down){
			next_down_transision = 1;
			if(display_app_status.mode_state.setting_mode){
				display_app_status.flags.decrement = 1;
			}
		}
	}

	if(display_app_status.mode_state.main == MODE_MAIN_PRESSURE){
		display_app_status.mode_state.setting_mode = 0;
		//LOCK
		if(global_struct->ram.button.very_long.mode && global_struct->ram.button.very_long.down){
			display_app_status.mode_state.main = MODE_MAIN_LOCK_UNLOCK;
		}
		//ZERO ADJUSTMENT
		else if(global_struct->ram.button.very_long.up && global_struct->ram.button.very_long.down){
			display_app_status.mode_state.main = MODE_MAIN_ZERO_ADJUSTMENT;
		}
		//SUB SETTING MODE
		else if(global_struct->ram.button.very_long.mode){
			display_app_status.mode_state.sub = 0;
			display_app_status.mode_state.parent = MODE_PARENT_SUB;
		}
		else{

		}
	}
	else{
		if(global_struct->ram.button.very_long.mode){
			display_app_status.mode_state.main = MODE_MAIN_PRESSURE;
		}
	}


	//modeクリックでの遷移
	if(next_mode_transision){
		switch(display_app_status.mode_state.main){
			case MODE_MAIN_PRESSURE:
				if(global_struct->eeprom.system.information.customer_code != CC_WITHOUT_OUTPUT_COMPARISON_FUNCTION){
					if(global_struct->eeprom.user.comparison1.mode){
						display_app_status.mode_state.main = MODE_MAIN_WIN_1HI;
					}
					else{
						display_app_status.mode_state.main = MODE_MAIN_HYS_1P1;
					}
				}
				break;

			case MODE_MAIN_MIN:
			case MODE_MAIN_MAX:
				display_app_status.mode_state.main = MODE_MAIN_PRESSURE;
				break;


			case MODE_MAIN_ZERO_ADJUSTMENT:
				break;

			case MODE_MAIN_LOCK_UNLOCK:
				break;

			case MODE_MAIN_HYS_1P1:
				display_app_status.mode_state.main = MODE_MAIN_HYS_1P2;
				break;

			case MODE_MAIN_HYS_1P2:
				if(global_struct->eeprom.user.comparison2.mode){
					display_app_status.mode_state.main = MODE_MAIN_WIN_2HI;
				}
				else{
					display_app_status.mode_state.main = MODE_MAIN_HYS_2P1;
				}
				break;

			case MODE_MAIN_HYS_2P1:
				display_app_status.mode_state.main = MODE_MAIN_HYS_2P2;
				break;

			case MODE_MAIN_HYS_2P2:
				display_app_status.mode_state.main = MODE_MAIN_PRESSURE;
				break;



			case MODE_MAIN_WIN_1HI:
				display_app_status.mode_state.main = MODE_MAIN_WIN_1LO;
				break;

			case MODE_MAIN_WIN_1LO:
				if(global_struct->eeprom.user.comparison2.mode){
					display_app_status.mode_state.main = MODE_MAIN_WIN_2HI;
				}
				else{
					display_app_status.mode_state.main = MODE_MAIN_HYS_2P1;
				}
				break;

			case MODE_MAIN_WIN_2HI:
				display_app_status.mode_state.main = MODE_MAIN_WIN_2LO;
				break;

			case MODE_MAIN_WIN_2LO:
				display_app_status.mode_state.main = MODE_MAIN_PRESSURE;
				break;

			default:
				break;
		}
	}



	//upを押したときの遷移
	if(next_up_transision){
		switch(display_app_status.mode_state.main){
			case MODE_MAIN_PRESSURE:
			case MODE_MAIN_MIN:
				display_app_status.mode_state.main = MODE_MAIN_MAX;
				break;

			default:
				break;
		}
	}

	//downを押したときの遷移
	if(next_down_transision){
		switch(display_app_status.mode_state.main){
			case MODE_MAIN_PRESSURE:
			case MODE_MAIN_MAX:
				display_app_status.mode_state.main = MODE_MAIN_MIN;
				break;

			default:
				break;
		}
	}

	display_app_status.flags.transition_clicked = 0;
}


static volatile uint8_t sub_menu_mode_list_size;
static volatile uint8_t *sub_menu_mode_list_pointer;
static void Display_App_Create_Sub_Menu(){

	volatile static const uint8_t sub_menu_mode_template_list_full_function[] = {
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
	};

	volatile static const uint8_t sub_menu_mode_template_list_general_function[] = {
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
	};

	volatile static const uint8_t sub_menu_mode_template_list_without_inverting[] = {
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
		MODE_SUB_ECO,
		MODE_SUB_TEST,
		MODE_SUB_FACTORY_RESET,
	};

	volatile static const uint8_t sub_menu_mode_template_list_without_comparison_function[] = {
		MODE_SUB_ADDRESS,
		MODE_SUB_BAUDRATE,
		MODE_SUB_FILTER_DISPLAY,
		MODE_SUB_FILTER_OUTPUT,
		MODE_SUB_LOW_CUT_DISPLAY,
		MODE_SUB_SIGN_INVERSION_DISPLAY,
		MODE_SUB_SIGN_INVERSION_OUTPUT,
		MODE_SUB_ECO,
		MODE_SUB_TEST,
		MODE_SUB_FACTORY_RESET,
	};

	//メニュー分岐
	switch(global_struct->eeprom.system.information.customer_code){
		//フルスペック
		case CC_FULL_FUNCTION:
			sub_menu_mode_list_size = sizeof(sub_menu_mode_template_list_full_function);
			sub_menu_mode_list_pointer = sub_menu_mode_template_list_full_function;
			break;

		//標準
		case CC_GENERAL_FUNCTION:
			sub_menu_mode_list_size = sizeof(sub_menu_mode_template_list_general_function);
			sub_menu_mode_list_pointer = sub_menu_mode_template_list_general_function;
			break;

		//出力反転機能なし
		case CC_WITHOUT_INVERTING_FUNCTION:
			sub_menu_mode_list_size = sizeof(sub_menu_mode_template_list_without_inverting);
			sub_menu_mode_list_pointer = sub_menu_mode_template_list_without_inverting;
			break;

		//比較出力なし
		case CC_WITHOUT_OUTPUT_COMPARISON_FUNCTION:
			sub_menu_mode_list_size = sizeof(sub_menu_mode_template_list_without_comparison_function);
			sub_menu_mode_list_pointer = sub_menu_mode_template_list_without_comparison_function;
			break;

		//フルスペック
		default:
			sub_menu_mode_list_size = sizeof(sub_menu_mode_template_list_full_function);
			sub_menu_mode_list_pointer = sub_menu_mode_template_list_full_function;
			break;
	}

}

//モード遷移先(サブ設定モード)
static void Display_App_Transition_State_Branch_Sub(){

	//サブ設定モードでは、どの状態からでもmodeボタン長押しによって圧力表示モードに遷移する
	//これはmode長押し
	if(global_struct->ram.button.very_long.mode){
		display_app_status.mode_state.parent = MODE_PARENT_MAIN;
		display_app_status.mode_state.main = MODE_MAIN_PRESSURE;
		display_app_status.mode_state.setting_mode = 0;
		display_app_status.flags.confirmed = 0;
		return;
	}

	uint8_t next_mode_transision = 0;
	//クリックするたびにここに入る
	if(display_app_status.flags.transition_clicked){
		//mode
		if(global_struct->ram.button.clicked.mode){
			next_mode_transision = 1;//次のモードに遷移するフラグ
			display_app_status.mode_state.setting_mode = 0;
			display_app_status.flags.increment = 0;
			display_app_status.flags.decrement = 0;
			display_app_status.flags.confirmed = 0;
		}
		//ent
		else if(global_struct->ram.button.clicked.ent){
			if(display_app_status.mode_state.setting_mode == 0){
				display_app_status.mode_state.setting_mode = 1;
			}
			else{
				display_app_status.flags.confirmed = 1;//決定ボタンのフラグ
			}
		}
		//up
		else if(global_struct->ram.button.clicked.up){
			if(display_app_status.mode_state.setting_mode){
				display_app_status.flags.increment = 1;//設定に入っている状態でup/downを押したときのフラグ(下で回収している)
			}
		}
		//down
		else if(global_struct->ram.button.clicked.down){
			if(display_app_status.mode_state.setting_mode){
				display_app_status.flags.decrement = 1;
			}
		}
	}

	//遷移先 ※遥か下の表示内容の関数(Display_App_Show_Branch_Sub)もメンテすること！Display_App_Transition_State_Branch_Sub
	if(next_mode_transision){

		//モード番号のインデックスを探す
		uint8_t found_index = 0;
		for(uint8_t i = 0; i < sub_menu_mode_list_size; i++){
			if(sub_menu_mode_list_pointer[i] == display_app_status.mode_state.sub){
				found_index = i;
				break;
			}
		}

		//次のインデックスにするためにインクリメント
		//オーバーフローなら0に戻す
		found_index++;
		if(found_index >= sub_menu_mode_list_size){
			found_index = 0;
		}

		//遷移
		display_app_status.mode_state.sub = sub_menu_mode_list_pointer[found_index];

	}

	display_app_status.flags.transition_clicked = 0;

}


//モードの遷移先(最上位)
static void Display_App_Transition_State_Branch_Parent(){

	switch(display_app_status.mode_state.parent){

		case MODE_PARENT_OPENING:
			break;
		case MODE_PARENT_MAIN:
			Display_App_Transition_State_Branch_Main();
			break;
		case MODE_PARENT_SUB:
			Display_App_Transition_State_Branch_Sub();
			break;

		default:
			break;
	}
}


//遷移フラグの回収
static void Display_App_Flag_Recovery(){

	if(display_app_status.flags.transition_very_long_pressed || global_struct->ram.button.clicked.any){
		//クリックフラグ
		if(global_struct->ram.button.clicked.any){
			display_app_status.flags.transition_clicked = 1;
			global_struct->ram.button.clicked.any = 0;
		}
		//長押しフラグ
		if(display_app_status.flags.transition_very_long_pressed){
			display_app_status.flags.transition_very_long_pressed = 0;
		}

		Display_App_Transition_State_Branch_Parent();
		display_app_status.flags.transitioned_very_long_pressed = 1;

	}
}




//ボタン長押しでの動作(プロセスバーを表示するかどうか)
static void Display_App_Button_And_ProcessBar_Behavior(){
	if(global_struct->ram.button.very_long.any){
		if(display_app_status.flags.transitioned_very_long_pressed){
			display_app_status.flags.transition_very_long_pressed = 0;
		}
		else{
			display_app_status.flags.transition_very_long_pressed = 1;
			Display_ProcessBar_Stop();
		}
	}
	else{
		if(global_struct->ram.button.continuously.any){

			//mode_state????
			uint8_t b = Display_App_ProcessBar_Branch_Parent();
			if(b){
				Display_ProcessBar_Start();
				Display_Blink_Stop();
			}
		}
		else{

			display_app_status.flags.transitioned_very_long_pressed = 0;
			Display_ProcessBar_Stop();
		}
	}
}


//決定ボタン押したときに点滅するギミック
static uint8_t Display_App_Confirm_Blink(){
	if(Display_Blink_IsFinished()){
		return 2;
	}
	else if(Display_Blink_IsOnGoing() == 0){
		Display_Blink_Config(30, 4);//300ms
		Display_Blink_Start();
		return 0;
	}
	else{
		return 1;
	}
}

//比較出力???表示処理
static void Display_App_LED_Process(){
	Display_LED_Comparison_State(0, global_struct->ram.comparison1.output_transistor_state);
	Display_LED_Comparison_State(1, global_struct->ram.comparison2.output_transistor_state);
}


//比較出力をパワーオン??ィレイでリセットする
static void Display_App_Min_Max_Reset_Process(){
	if(Comparison1_Timer_Power_On_Delay_IsFinished()){
		Sensor_Clear_Min_Max();
	}
	if(Comparison2_Timer_Power_On_Delay_IsFinished()){
		Sensor_Clear_Min_Max();
	}
}


//画面がタイムアウトしたときの処理
static void Display_App_Timeout_Monitoring_Process(){
	//タイ??アウトしたら??の圧力表示に戻??
	if(Procedure_Timeout_Timer_IsTimeout()){
		display_app_status.mode_state.main = MODE_MAIN_PRESSURE;
		display_app_status.mode_state.parent = MODE_PARENT_MAIN;
		return;
	}

	//  下記条件"以下"でタイマ開始
	//	display_app_status.mode_state.main == MODE_MAIN_PRESSURE;
	//	display_app_status.mode_state.parent == MODE_PARENT_MAIN;
	//	display_app_status.mode_state.parent == MODE_PARENT_SUB && display_app_status.mode_state.setting_mode == 1
	if(display_app_status.mode_state.main == MODE_MAIN_PRESSURE && display_app_status.mode_state.parent == MODE_PARENT_MAIN){
		Procedure_Timeout_Timer_Stop();
	}
	else if(display_app_status.mode_state.parent == MODE_PARENT_SUB && display_app_status.mode_state.setting_mode == 1){
		Procedure_Timeout_Timer_Stop();
	}
	else{
		Procedure_Timeout_Timer_Start();
	}

	//なにかボタンを押したらリロード
	if(global_struct->ram.button.now.any){
		Procedure_Timeout_Timer_Reload();
	}
}

//▼▲でインクリメントする範囲のテンプレート
//skip_value_array[]にはlimit_topやlimit_bottomの値を含めないこと！
static int32_t Display_App_Setting_Increment_Normal(int32_t setting_value_origin, int32_t limit_top, int32_t limit_bottom, int32_t *skip_value_array, uint16_t skip_value_array_size){


	//LOCKしているときは" LOC"の表示をしてreturn
	if(global_struct->eeprom.user.mode.protected && (display_app_status.flags.increment || display_app_status.flags.decrement)){
		if(Display_Lit_IsOnGoing()){
			Display_String_Show(" LOC");
			Display_Set_Dot_Position(0);
		}
		else if(Display_Lit_IsFinished()){
			display_app_status.flags.decrement = 0;
			display_app_status.flags.increment = 0;
		}
		else{
			Display_Lit_Config(120);//1200ms
			Display_Lit_Start();
		}
		return setting_value_origin;
	}

	int32_t next_add_value;
	//設定範囲をくるくる回るように表示(1, 2, 3, 4, 0, 1, 2, 3, 4, 0, ....)
	//▲
	if(display_app_status.flags.increment){


		next_add_value = setting_value_origin + 1;

		for(uint16_t i = 0; i < skip_value_array_size; i++){
			if(skip_value_array[i] == next_add_value)
				next_add_value++;
		}

		if(next_add_value > limit_top){
			setting_value_origin = limit_bottom;
		}
		else{
			setting_value_origin = next_add_value;
		}
		display_app_status.flags.increment = 0;
	}

	//▼
	else if(display_app_status.flags.decrement){


		next_add_value = setting_value_origin - 1;

		for(uint16_t i = 0; i < skip_value_array_size; i++){
			if(skip_value_array[i] == next_add_value)
				next_add_value--;
		}

		if(next_add_value < limit_bottom){
			setting_value_origin = limit_top;
		}
		else{
			setting_value_origin = next_add_value;
		}
		display_app_status.flags.decrement = 0;

	}

	return setting_value_origin;
}

//▼▲でインクリメントする内容のテンプレート
static int32_t Display_App_Setting_Increment_Fast(int32_t setting_value_origin, int32_t limit_top, int32_t limit_bottom){
	int8_t increment_value = 0;
	int32_t next_add_value;


	increment_value = Button_Get_Increment_Value_Cyclic(10);
	if(display_app_status.flags.increment || (increment_value > 0)){

		if(global_struct->eeprom.user.mode.protected){
			if(Display_Lit_IsOnGoing()){
				Display_String_Show(" LOC");
				Display_Set_Dot_Position(0);
			}
			else if(Display_Lit_IsFinished()){
				display_app_status.flags.increment = 0;
			}
			else{
				Display_Lit_Config(120);//1200ms
				Display_Lit_Start();
			}
			return setting_value_origin;
		}

		if(setting_value_origin != limit_top){
			next_add_value = setting_value_origin + increment_value + display_app_status.flags.increment;
			if(next_add_value > limit_top){
				setting_value_origin = limit_top;
			}
			else{
				setting_value_origin += increment_value + display_app_status.flags.increment;
			}
		}
		display_app_status.flags.increment = 0;
	}
	else if(display_app_status.flags.decrement || (increment_value < 0)){

		if(global_struct->eeprom.user.mode.protected){
			if(Display_Lit_IsOnGoing()){
				Display_String_Show(" LOC");
				Display_Set_Dot_Position(0);
			}
			else if(Display_Lit_IsFinished()){
				display_app_status.flags.decrement = 0;
			}
			else{
				Display_Lit_Config(120);//1200ms
				Display_Lit_Start();
			}
			return setting_value_origin;
		}

		if(setting_value_origin >= limit_bottom){
			next_add_value = setting_value_origin + increment_value - display_app_status.flags.decrement;
			if(next_add_value <= limit_bottom){
				setting_value_origin = limit_bottom;
			}
			else{
				setting_value_origin += increment_value - display_app_status.flags.decrement;
			}
		}
		display_app_status.flags.decrement = 0;
	}

	return setting_value_origin;
}


//xxx_Mode_Sub_Setting_xxx と xxx_Mode_Main_Setting_xxxは、ENTキーを押したときに入る画面
//Hys:P1,P2 Win:Hi,Lo
static int32_t Display_App_Mode_Main_Setting_Hys_Win(int32_t setting_value_origin){//10,000%

	int8_t increment_value = 0;
	int32_t limit_top, limit_bottom, span, next_add_value;

	//圧力からパーセンテージを計算して代入(比較出力のパーセンテージで判断しているため)
	//percentage -> pascal
	span = global_struct->eeprom.system.information.fullscale_plus + global_struct->eeprom.system.information.fullscale_minus;
	limit_top = global_struct->eeprom.system.information.fullscale_plus + (int16_t)(global_struct->eeprom.system.information.scale_margin * span) / 100;
	limit_bottom = -(global_struct->eeprom.system.information.fullscale_minus + (int16_t)(global_struct->eeprom.system.information.scale_margin * span) / 100);
	int16_t pascal = (span * setting_value_origin)/10000;

	//設定時の表示状態
	uint8_t point = Display_Set_Default_Dot_Position();
	Display_Int_Decimal_Show(pascal, point+1);



	pascal = Display_App_Setting_Increment_Fast(pascal, limit_top, limit_bottom);

	//圧力からパーセンテージを計算して代入(比較出力のパーセンテージで判断しているため)
	setting_value_origin = (10000 * pascal)/(span);
	return setting_value_origin;
}

//設定画面のテンプレート画面
static int32_t Display_App_Setting_Template(uint8_t *title, uint8_t dot_position, int32_t setting_value, int32_t (*Function)()){
	static int32_t acc_stack = 0;
	static int32_t return_stack = 0;
	static uint8_t onetime = 0;



	if (display_app_status.mode_state.setting_mode == 0) {
		Display_String_Show(title);//" 1FC"
		Display_Set_Dot_Position(dot_position);//2
		Display_Blink_Stop();
		onetime = 0;
		return_stack = setting_value;
	}
	else {
		if (display_app_status.flags.confirmed) {

			if(global_struct->eeprom.user.mode.protected){
				if(Display_Lit_IsFinished()){
					display_app_status.flags.confirmed = 0;
					onetime = 0;
				}
				else if(Display_Lit_IsOnGoing()){
					Display_String_Show(" LOC");
					Display_Set_Dot_Position(0);
				}
				else{
					Display_Lit_Config(120);//1200ms
					Display_Lit_Start();
				}
				return_stack = setting_value;

			}
			else{
				//初期化モードNOで点滅させない
				if((Function == Display_App_Mode_Sub_Setting_Factory_Reset) && (acc_stack == 0)){
					return_stack = acc_stack;
					display_app_status.flags.confirmed = 0;
					onetime = 0;
				}
				else{
					switch(Display_App_Confirm_Blink()){
						case 0:
							return_stack = acc_stack;
							EEPROM_Backup_Request_User();
							break;
						case 1:
							break;
						default:
							display_app_status.flags.confirmed = 0;
							onetime = 0;
							break;
					}
				}
			}
		}
		else {
			if (onetime == 0) {
				onetime = 1;
				acc_stack =	setting_value;
			}

			acc_stack = Function(acc_stack);//引数の関数ポインタを実行する
			return_stack = setting_value;
		}
	}

	return return_stack;
}


//最大値最小値表示
static void Display_App_Show_Main_Min_Max(uint8_t *title, uint8_t overflowed, uint8_t underflowed, int16_t min_max_value, uint8_t min_max_bool){


//	Procedure_Timeout_Timer_Config(15000);//15s
//	Procedure_Timeout_Timer_Start();
//min_max_bool ... 0:min 1:max

	//10ms
	static uint16_t counter = 0;
	static uint8_t state = 0;

	if(display_app_status.flags.confirmed){
		if(global_struct->eeprom.user.mode.protected){
			Display_Lit_Config(120);//1200ms
			Display_Lit_Start();
			display_app_status.flags.confirmed = 0;
		}
		else{
			display_app_status.flags.confirmed = 0;
			Display_Blink_Config(25, 4);
			Display_Blink_Start();
			if(min_max_bool){
				Sensor_Clear_Max_Display();
			}
			else{
				Sensor_Clear_Min_Display();
			}
		}
	}


	if(Display_Lit_IsOnGoing()){
		if(global_struct->eeprom.user.mode.protected){
			Display_String_Show(" LOC");
			Display_Set_Dot_Position(0);
		}
	}
	else if(state==0){
		Display_String_Show(title);
		Display_Set_Dot_Position(0);
	}
	else{
		if(overflowed){
			Display_String_Show(" FFF");
			Display_Set_Dot_Position(0);
		}
		else if(underflowed){
			Display_String_Show("-FFF");
			Display_Set_Dot_Position(0);
		}
		else{
			//最大と最小の圧力表示
			uint8_t point = Display_Set_Default_Dot_Position();
			int16_t span = global_struct->eeprom.system.information.fullscale_plus + global_struct->eeprom.system.information.fullscale_minus;
			int16_t pascal = (span * min_max_value)/10000;
			Display_Int_Decimal_Show(pascal, point+1);
		}
	}

	//1.2s
	if(counter >= 110){
		counter = 0;
		if(state){
			state = 0;
		}
		else{
			state = 1;
		}
	}
	else{
		counter++;
	}
}

//表示内容(圧力表示モード)
static void Display_App_Show_Branch_Main(){
	static uint8_t onetime = 0;//一時保存するときにつかう
	static int32_t acc_stack = 0;//一時保保存/退避用
	uint8_t blink_flag = 0;

	switch(display_app_status.mode_state.main){

		case MODE_MAIN_PRESSURE:
			global_struct->ram.status_and_flags.mode_state = 0;//??ストモードから帰ってきたときに計算有効にする
			Display_Blink_Stop();

			if(global_struct->ram.pressure.overflowed_display){
				Display_String_Show(" FFF");
				Display_Set_Dot_Position(0);
			}
			else if(global_struct->ram.pressure.underflowed_display){
				Display_String_Show("-FFF");
				Display_Set_Dot_Position(0);
			}
			else{
				//圧力表示
				uint8_t point = Display_Set_Default_Dot_Position();
				Display_Int_Decimal_Show(global_struct->ram.pressure.pascal_display, point+1);
			}
			break;

		case MODE_MAIN_MIN:
			Display_App_Show_Main_Min_Max(" ___", Sensor_Get_Display_Min_IsOverflowed(), Sensor_Get_Display_Min_IsUnderflowed(), Sensor_Get_Pressure_Percentage_Min_Display(), 0);
			break;

		case MODE_MAIN_MAX:
			Display_App_Show_Main_Min_Max(" ^^^", Sensor_Get_Display_Max_IsOverflowed(), Sensor_Get_Display_Max_IsUnderflowed(), Sensor_Get_Pressure_Percentage_Max_Display(), 1);
			break;


		case MODE_MAIN_ZERO_ADJUSTMENT:
			if(Display_Blink_IsFinished() || Display_Lit_IsFinished()){
				display_app_status.mode_state.main = MODE_MAIN_PRESSURE;
				break;
			}
			if((Display_Blink_IsOnGoing() == 0) && (Display_Lit_IsOnGoing() == 0)){
				if(global_struct->eeprom.user.mode.protected){
					Display_Lit_Config(120);//1200ms
					Display_Lit_Start();
					Display_String_Show(" LOC");
					Display_Set_Dot_Position(0);
				}
				else if(Sensor_Zero_Adjustment()){
					Display_ZeroFill_Show();
					Display_Set_Dot_Position(0);
					Display_Blink_Config(30, 4);//300ms
					Display_Blink_Start();
				}
				else{
					Display_Error_Show();
					Display_Set_Dot_Position(0);
					Display_Lit_Config(120);//1200ms
					Display_Lit_Start();
				}
			}
//			if(Display_Lit_IsOnGoing()){
//				Display_String_Show(" LOC");
//				Display_Set_Dot_Position(0);
//			}
			break;

		case MODE_MAIN_LOCK_UNLOCK:
			if(Display_Lit_IsFinished()){
				display_app_status.mode_state.main = MODE_MAIN_PRESSURE;
				break;
			}
			if(Display_Lit_IsOnGoing() == 0){
				if(global_struct->eeprom.user.mode.protected == 0){
					Display_String_Show(" LOC");
					Display_Set_Dot_Position(0);
					global_struct->eeprom.user.mode.protected = 1;
				}
				else{
					Display_String_Show(" UNL");
					Display_Set_Dot_Position(0);
					global_struct->eeprom.user.mode.protected = 0;
				}
				Display_Lit_Config(120);//1200ms
				Display_Lit_Start();
				EEPROM_Backup_Request_User();
			}
			break;



		case MODE_MAIN_HYS_1P1:
			global_struct->eeprom.user.comparison1.hys_p1 = Display_App_Setting_Template(
					" 1P1",
					2,
					global_struct->eeprom.user.comparison1.hys_p1,

					Display_App_Mode_Main_Setting_Hys_Win
					);
			break;

		case MODE_MAIN_HYS_1P2:
			global_struct->eeprom.user.comparison1.hys_p2 = Display_App_Setting_Template(
					" 1P2",
					2,
					global_struct->eeprom.user.comparison1.hys_p2,

					Display_App_Mode_Main_Setting_Hys_Win
					);
			break;

		case MODE_MAIN_HYS_2P1:
			global_struct->eeprom.user.comparison2.hys_p1 = Display_App_Setting_Template(
					" 2P1",
					2,
					global_struct->eeprom.user.comparison2.hys_p1,

					Display_App_Mode_Main_Setting_Hys_Win
					);
			break;

		case MODE_MAIN_HYS_2P2:
			global_struct->eeprom.user.comparison2.hys_p2 = Display_App_Setting_Template(
					" 2P2",
					2,
					global_struct->eeprom.user.comparison2.hys_p2,

					Display_App_Mode_Main_Setting_Hys_Win
					);
			break;

		case MODE_MAIN_WIN_1HI:
			global_struct->eeprom.user.comparison1.win_hi = Display_App_Setting_Template(
					" 1HI",
					2,
					global_struct->eeprom.user.comparison1.win_hi,

					Display_App_Mode_Main_Setting_Hys_Win
					);
			break;

		case MODE_MAIN_WIN_1LO:
			global_struct->eeprom.user.comparison1.win_low = Display_App_Setting_Template(
					" 1LO",
					2,
					global_struct->eeprom.user.comparison1.win_low,

					Display_App_Mode_Main_Setting_Hys_Win
					);
			break;

		case MODE_MAIN_WIN_2HI:
			global_struct->eeprom.user.comparison2.win_hi = Display_App_Setting_Template(
					" 2HI",
					2,
					global_struct->eeprom.user.comparison2.win_hi,

					Display_App_Mode_Main_Setting_Hys_Win
					);
			break;
		case MODE_MAIN_WIN_2LO:
			global_struct->eeprom.user.comparison2.win_low = Display_App_Setting_Template(
					" 2LO",
					2,
					global_struct->eeprom.user.comparison2.win_low,

					Display_App_Mode_Main_Setting_Hys_Win
					);
			break;

		default:
			break;
	}

}

//xxx_Mode_Sub_Setting_xxx と xxx_Mode_Main_Setting_xxxは、ENTキーを押したときに入る画面
static int32_t Display_App_Mode_Sub_Setting_Address(int32_t setting_value_origin){

	int8_t increment_value = 0;
	int32_t limit_top, limit_bottom;

	//設定時の表示文言
	Display_Set_Dot_Position(2);
	Display_Top_String_And_HEX_Show(" A  ", 2, setting_value_origin);


	limit_top = 254;
	limit_bottom = 1;

	setting_value_origin = Display_App_Setting_Increment_Fast(setting_value_origin, limit_top, limit_bottom);

	return setting_value_origin;
}


static int32_t Display_App_Mode_Sub_Setting_Baudrate(int32_t setting_value_origin){


	/*
	 * 0x00:1200 0x01:2400 0x02:4800 0x03:9600 0x04:14400 0x05:19200
	 * 0x06:38400 0x07:57600 0x08:115200 0x09:230400 0x0A:460800 0x0B:921600 baud
	 */
	//設定時の表示文言
	const uint32_t _baudrate_array[] = {
			12,
			24,
			48,
			96,
			144,
			192,
			384,
			576,
			1152,
			2304,
			4608,
			9216,
	};
	Display_Uint_Decimal_Show(_baudrate_array[setting_value_origin], 1);
	Display_Set_Dot_Position(1);


	int32_t limit_top, limit_bottom, next_value;
	int32_t skip_value_array[] = {0};
	uint16_t skip_value_array_size = 0;

	//設定できる範囲
	if(global_struct->eeprom.system.information.maintenance_mode == 1){
		limit_top = 0x0b;
		limit_bottom = 0x00;
	}
	else{
		limit_top = 0x05;
		limit_bottom = 0x03;
		skip_value_array[0] = 0x04;//14400baudを除外
		skip_value_array_size = 1;
	}


	setting_value_origin = Display_App_Setting_Increment_Normal(setting_value_origin, limit_top, limit_bottom, skip_value_array, skip_value_array_size);


	return setting_value_origin;
}



//比較
static int32_t Display_App_Mode_Sub_Setting_Comparison_Function(int32_t setting_value_origin){



	//設定時の表示文言
	const uint8_t *_mode_array[] = {
			" HYS",
			" WIN"
	};
	Display_String_Show(_mode_array[setting_value_origin]);
	Display_Set_Dot_Position(0);

	int32_t limit_top, limit_bottom;
	int32_t skip_value_array[] = {0};
	uint16_t skip_value_array_size = 0;

	limit_top = 1;
	limit_bottom = 0;

	setting_value_origin = Display_App_Setting_Increment_Normal(setting_value_origin, limit_top, limit_bottom, skip_value_array, skip_value_array_size);

	return setting_value_origin;
}

static int32_t Display_App_Mode_Sub_Setting_Comparison_Output(int32_t setting_value_origin){



	//設定時の表示文言
	const uint8_t *_str_array[] = {
			" ---",
			" OFF",
			"  NC",
			"  NO",
	};

	Display_String_Show(_str_array[setting_value_origin]);
	Display_Set_Dot_Position(0);

	int32_t limit_top, limit_bottom;
	int32_t skip_value_array[] = {0};
	uint16_t skip_value_array_size = 0;

	limit_top = 3;
	limit_bottom = 1;

	setting_value_origin = Display_App_Setting_Increment_Normal(setting_value_origin, limit_top, limit_bottom, skip_value_array, skip_value_array_size);

	return setting_value_origin;
}

static int32_t Display_App_Mode_Sub_Setting_Comparison_On_Delay(int32_t setting_value_origin){

	int32_t limit_top, limit_bottom;

	//設定時の表示文言
	Display_Top_String_And_Uint_Decimal_Show(" D  ", 2, setting_value_origin);
	Display_Set_Dot_Position(1);

	limit_top = 20;
	limit_bottom = 0;

	setting_value_origin = Display_App_Setting_Increment_Fast(setting_value_origin, limit_top, limit_bottom);

	return setting_value_origin;
}

static int32_t Display_App_Mode_Sub_Setting_Comparison_Off_Delay(int32_t setting_value_origin){

	int32_t limit_top, limit_bottom;

	//設定時の表示文言
	Display_Top_String_And_Uint_Decimal_Show(" D  ", 2, setting_value_origin);
	Display_Set_Dot_Position(1);

	limit_top = 20;
	limit_bottom = 0;

	setting_value_origin = Display_App_Setting_Increment_Fast(setting_value_origin, limit_top, limit_bottom);

	return setting_value_origin;
}

static int32_t Display_App_Mode_Sub_Setting_Comparison_Power_On_Delay(int32_t setting_value_origin){

	int32_t limit_top, limit_bottom;

	//設定時の表示文言
	Display_Top_String_And_Uint_Decimal_Show(" D  ", 2, setting_value_origin);
	Display_Set_Dot_Position(0);

	limit_top = 20;
	limit_bottom = 0;

	setting_value_origin = Display_App_Setting_Increment_Fast(setting_value_origin, limit_top, limit_bottom);

	return setting_value_origin;
}


//移動平均フィルタ
static int32_t Display_App_Mode_Sub_Setting_Filter(int32_t setting_value_origin){



	//設定時の表示文言
	const uint8_t *_str_array[] = {
			" F02",
			" F10",
			" F20",
			" F40",
	};

	Display_String_Show(_str_array[setting_value_origin]);
	Display_Set_Dot_Position(1);

	int32_t limit_top, limit_bottom;
	int32_t skip_value_array[] = {0};
	uint16_t skip_value_array_size = 0;

	limit_top = 3;
	limit_bottom = 0;

	setting_value_origin = Display_App_Setting_Increment_Normal(setting_value_origin, limit_top, limit_bottom, skip_value_array, skip_value_array_size);

	return setting_value_origin;
}


//表示ローカット
static int32_t Display_App_Mode_Sub_Setting_Low_Cut(int32_t setting_value_origin){



	//設定時の表示文言
	const uint8_t *_str_array[] = {
			" CT0",
			" CT1",
			" CT2",
			" CT3",
			" CT4",
			" CT5",
	};

	Display_String_Show(_str_array[setting_value_origin]);
	Display_Set_Dot_Position(0);

	int32_t limit_top, limit_bottom;
	int32_t skip_value_array[] = {0};
	uint16_t skip_value_array_size = 0;

	limit_top = 5;
	limit_bottom = 0;

	setting_value_origin = Display_App_Setting_Increment_Normal(setting_value_origin, limit_top, limit_bottom, skip_value_array, skip_value_array_size);

	return setting_value_origin;
}

//符号反転
static int32_t Display_App_Mode_Sub_Setting_Sign_Inversion(int32_t setting_value_origin){



	//設定時の表示文言
	const uint8_t *_str_array[] = {
			" OFF",
			"  ON",
	};

	Display_String_Show(_str_array[setting_value_origin]);
	Display_Set_Dot_Position(0);

	int32_t limit_top, limit_bottom;
	int32_t skip_value_array[] = {0};
	uint16_t skip_value_array_size = 0;

	limit_top = 1;
	limit_bottom = 0;

	setting_value_origin = Display_App_Setting_Increment_Normal(setting_value_origin, limit_top, limit_bottom, skip_value_array, skip_value_array_size);

	return setting_value_origin;
}


static int32_t Display_App_Mode_Sub_Setting_ECO(int32_t setting_value_origin){



	//設定時の表示文言
	const uint8_t *_str_array[] = {
			" OFF",
			"  ON",
	};

	Display_String_Show(_str_array[setting_value_origin]);
	Display_Set_Dot_Position(0);

	int32_t limit_top, limit_bottom;
	int32_t skip_value_array[] = {0};
	uint16_t skip_value_array_size = 0;

	limit_top = 1;
	limit_bottom = 0;

	setting_value_origin = Display_App_Setting_Increment_Normal(setting_value_origin, limit_top, limit_bottom, skip_value_array, skip_value_array_size);

	return setting_value_origin;
}

static int32_t Display_App_Mode_Sub_Setting_Test(int32_t setting_value_origin){
	int32_t limit_top, limit_bottom;

	//設定時の表示文言
	uint8_t point = Display_Set_Default_Dot_Position();
	Display_Int_Decimal_Show(setting_value_origin, point+1);
	int32_t percentage_10 = (global_struct->eeprom.system.information.fullscale_plus + global_struct->eeprom.system.information.fullscale_minus) /10;

	if(global_struct->eeprom.system.information.scale_type == 0){
		limit_top = global_struct->eeprom.system.information.fullscale_plus+percentage_10;
		limit_bottom = -percentage_10;
	}
	else if(global_struct->eeprom.system.information.scale_type == 1){
		limit_top = global_struct->eeprom.system.information.fullscale_plus+percentage_10;
		limit_bottom = -global_struct->eeprom.system.information.fullscale_minus-percentage_10;
	}
	else{
		limit_top = percentage_10;
		limit_bottom = -global_struct->eeprom.system.information.fullscale_minus-percentage_10;
	}

	//圧力からパーセンテージを計算して代入(比較出力のパーセンテージで判断しているため)
	int32_t percentage = (10000 * setting_value_origin)/(limit_top - limit_bottom);
	global_struct->ram.pressure.percentage_output_adjusted = percentage;
	global_struct->ram.pressure.percentage_display_adjusted = percentage;

	setting_value_origin = Display_App_Setting_Increment_Fast(setting_value_origin, limit_top, limit_bottom);

	return setting_value_origin;
}

static int32_t Display_App_Mode_Sub_Setting_Factory_Reset(int32_t setting_value_origin){



	//設定時の表示文言
	const uint8_t *_str_array[] = {
			"  NO",
			" YES",
	};

	Display_String_Show(_str_array[setting_value_origin]);
	Display_Set_Dot_Position(0);

	int32_t limit_top, limit_bottom;
	int32_t skip_value_array[] = {0};
	uint16_t skip_value_array_size = 0;

	limit_top = 1;
	limit_bottom = 0;

	setting_value_origin = Display_App_Setting_Increment_Normal(setting_value_origin, limit_top, limit_bottom, skip_value_array, skip_value_array_size);

	return setting_value_origin;
}


//表示内容(サブ設定モード) ※遥か上の表示内容の関数(Display_App_Transition_State_Branch_Sub)もメンテすること
static void Display_App_Show_Branch_Sub(){
	static uint8_t onetime = 0;//一時保存するときにつかう
	static int32_t acc_stack = 0;//一時保保存/退避用

	switch(display_app_status.mode_state.sub){
		case MODE_SUB_ADDRESS:
			acc_stack = 0;//工場出荷時設定の尻ぬぐい

			global_struct->eeprom.user.communication.slave_address = Display_App_Setting_Template(
					" ADR",
					0,
					global_struct->eeprom.user.communication.slave_address,

					Display_App_Mode_Sub_Setting_Address
					);
			break;


		case MODE_SUB_BAUDRATE:
			acc_stack = global_struct->eeprom.user.communication.baud_rate;
			global_struct->eeprom.user.communication.baud_rate = Display_App_Setting_Template(
					" BAU",
					0,
					global_struct->eeprom.user.communication.baud_rate,

					Display_App_Mode_Sub_Setting_Baudrate
					);
			if(acc_stack != global_struct->eeprom.user.communication.baud_rate){
				Communication_RS485_Config_Reflesh();
				acc_stack = 0;
			}
			break;


		case MODE_SUB_COMPARISON1_FUNCTION:
			global_struct->eeprom.user.comparison1.mode = Display_App_Setting_Template(
					" 1FC",
					2,
					global_struct->eeprom.user.comparison1.mode,

					Display_App_Mode_Sub_Setting_Comparison_Function
					);
			break;


		case MODE_SUB_COMPARISON1_OUTPUT:
			global_struct->eeprom.user.comparison1.enabled = Display_App_Setting_Template(
					" 1OP",
					2,
					global_struct->eeprom.user.comparison1.enabled,

					Display_App_Mode_Sub_Setting_Comparison_Output
					);
			break;

		case MODE_SUB_COMPARISON1_ON_DELAY:
			global_struct->eeprom.user.comparison1.on_delay = Display_App_Setting_Template(
					" 1ON",
					2,
					global_struct->eeprom.user.comparison1.on_delay,

					Display_App_Mode_Sub_Setting_Comparison_On_Delay
					);
			break;

		case MODE_SUB_COMPARISON1_OFF_DELAY:
			global_struct->eeprom.user.comparison1.off_delay = Display_App_Setting_Template(
					" 1OF",
					2,
					global_struct->eeprom.user.comparison1.off_delay,

					Display_App_Mode_Sub_Setting_Comparison_Off_Delay
					);
			break;

		case MODE_SUB_COMPARISON1_POWER_ON_DELAY:
			global_struct->eeprom.user.comparison1.power_on_delay = Display_App_Setting_Template(
					" 1DP",
					2,
					global_struct->eeprom.user.comparison1.power_on_delay,

					Display_App_Mode_Sub_Setting_Comparison_Power_On_Delay
					);
			break;

		case MODE_SUB_COMPARISON2_FUNCTION:
			global_struct->eeprom.user.comparison2.mode = Display_App_Setting_Template(
					" 2FC",
					2,
					global_struct->eeprom.user.comparison2.mode,

					Display_App_Mode_Sub_Setting_Comparison_Function
					);
			break;


		case MODE_SUB_COMPARISON2_OUTPUT:
			global_struct->eeprom.user.comparison2.enabled = Display_App_Setting_Template(
					" 2OP",
					2,
					global_struct->eeprom.user.comparison2.enabled,

					Display_App_Mode_Sub_Setting_Comparison_Output
					);
			break;

		case MODE_SUB_COMPARISON2_ON_DELAY:
			global_struct->eeprom.user.comparison2.on_delay = Display_App_Setting_Template(
					" 2ON",
					2,
					global_struct->eeprom.user.comparison2.on_delay,

					Display_App_Mode_Sub_Setting_Comparison_On_Delay
					);
			break;

		case MODE_SUB_COMPARISON2_OFF_DELAY:
			global_struct->eeprom.user.comparison2.off_delay = Display_App_Setting_Template(
					" 2OF",
					2,
					global_struct->eeprom.user.comparison2.off_delay,

					Display_App_Mode_Sub_Setting_Comparison_Off_Delay
					);
			break;

		case MODE_SUB_COMPARISON2_POWER_ON_DELAY:
			global_struct->eeprom.user.comparison2.power_on_delay = Display_App_Setting_Template(
					" 2DP",
					2,
					global_struct->eeprom.user.comparison2.power_on_delay,

					Display_App_Mode_Sub_Setting_Comparison_Power_On_Delay
					);
			break;

		case MODE_SUB_FILTER_DISPLAY:
			acc_stack = global_struct->eeprom.user.display.average_time;
			global_struct->eeprom.user.display.average_time = Display_App_Setting_Template(
					" FLD",
					0,
					global_struct->eeprom.user.display.average_time,
					Display_App_Mode_Sub_Setting_Filter
					);

			if(acc_stack != global_struct->eeprom.user.display.average_time){
				Analog_Set_Average_Time_Display(global_struct->eeprom.user.display.average_time);
				acc_stack = 0;
			}


			break;

		case MODE_SUB_FILTER_OUTPUT:
			acc_stack = global_struct->eeprom.user.output.average_time;
			global_struct->eeprom.user.output.average_time = Display_App_Setting_Template(
					" FLO",
					0,
					global_struct->eeprom.user.output.average_time,

					Display_App_Mode_Sub_Setting_Filter
					);
			if(acc_stack != global_struct->eeprom.user.output.average_time){
				Analog_Set_Average_Time_Display(global_struct->eeprom.user.output.average_time);
				acc_stack = 0;
			}
			break;


		case MODE_SUB_LOW_CUT_DISPLAY:
			global_struct->eeprom.user.display.low_cut = Display_App_Setting_Template(
					" LCD",
					0,
					global_struct->eeprom.user.display.low_cut,

					Display_App_Mode_Sub_Setting_Low_Cut
					);
			break;

		case MODE_SUB_SIGN_INVERSION_DISPLAY:
			global_struct->eeprom.user.display.sign_inversion = Display_App_Setting_Template(
					" SGD",
					0,
					global_struct->eeprom.user.display.sign_inversion,

					Display_App_Mode_Sub_Setting_Sign_Inversion
					);
			break;

		case MODE_SUB_SIGN_INVERSION_OUTPUT:
			global_struct->eeprom.user.output.output_inversion = Display_App_Setting_Template(
					" SGO",
					0,
					global_struct->eeprom.user.output.output_inversion,

					Display_App_Mode_Sub_Setting_Sign_Inversion
					);
			break;

		case MODE_SUB_ECO:
			global_struct->eeprom.user.mode.eco_mode = Display_App_Setting_Template(
					" ECO",
					0,
					global_struct->eeprom.user.mode.eco_mode,
					Display_App_Mode_Sub_Setting_ECO
					);
			break;

		case MODE_SUB_TEST:


			if (display_app_status.mode_state.setting_mode == 0) {
				Display_String_Show(" TST");
				Display_Set_Dot_Position(0);
				Display_Blink_Stop();
				onetime = 0;

			}
			else {
				if (display_app_status.flags.confirmed) {
					display_app_status.flags.confirmed = 0;//決定してもなにも起こさない
				}
				if(global_struct->eeprom.user.mode.protected){
					if(Display_Lit_IsFinished()){
						display_app_status.mode_state.setting_mode = 0;
					}
					else if(Display_Lit_IsOnGoing()){
						Display_String_Show(" LOC");
						Display_Set_Dot_Position(0);
					}
					else{
						Display_Lit_Config(120);//1200ms
						Display_Lit_Start();
					}
				}
				else {
					global_struct->ram.status_and_flags.mode_state = 2;//test mode
					if (onetime == 0) {
						onetime = 1;
						Display_Blink_Config(10, 0xff);
						Display_Blink_Start();

						global_struct->ram.pressure.pascal_display = 0;
						global_struct->ram.pressure.pascal_output = 0;
					}
					global_struct->ram.pressure.pascal_display = Display_App_Mode_Sub_Setting_Test(global_struct->ram.pressure.pascal_display);
					global_struct->ram.pressure.pascal_output = global_struct->ram.pressure.pascal_display;
				}
			}

			acc_stack = 0;//工場出荷時設定の準備
			break;

		case MODE_SUB_FACTORY_RESET:

			//テストモードの尻拭い(1回だけ実行)
			if(global_struct->ram.status_and_flags.mode_state == 2){
				global_struct->ram.status_and_flags.mode_state = 0;
			}

			//onetimeの初期値はここで必ず0になる
			onetime = Display_App_Setting_Template(
				" CLR",
				0,
				acc_stack,//初回はいつも"NO"
				Display_App_Mode_Sub_Setting_Factory_Reset
			);

			//yes⇒no または no⇒yes の時に1回だけ実行
			if(onetime  != acc_stack) {
				acc_stack = onetime;
				if(onetime){
					EEPROM_Factory_Reset();
				}
			}

			break;

	}
}

//表示内容(最上位)
static void Display_App_Show_Branch_Parent(){

	//プロセスバーの表示中に前回の画面と交互に表示してしまう現象回避(????な??)
	if(Display_ProcessBar_IsOnGoing()){
		return;
	}

	switch(display_app_status.mode_state.parent){
		case MODE_PARENT_MAIN:
			Display_App_Show_Branch_Main();
			break;
		case MODE_PARENT_SUB:
			Display_App_Show_Branch_Sub();
		default:
			break;
	}
}


//画面の明るさ制御
static void Display_App_Brightness_Control_Process(){
	if(global_struct->eeprom.user.mode.eco_mode == 0){
		Display_Set_Default_Brightness();
	}
	else{
		switch(display_app_status.mode_state.parent){
			case MODE_PARENT_MAIN:
				if(display_app_status.mode_state.main == MODE_MAIN_PRESSURE){
					Display_Set_Eco_Brightness();
				}
				else{
					Display_Set_Default_Brightness();
				}
				break;

			default:
				Display_Set_Default_Brightness();
				break;
		}
	}
}


void Display_App_Process(){

	static uint8_t flag_transition = 0;

	Display_App_Button_And_ProcessBar_Behavior();
	Display_App_Flag_Recovery();
	Display_App_Brightness_Control_Process();
	Display_App_Show_Branch_Parent();
	Display_App_Timeout_Monitoring_Process();

}

/* 10ms */
void Display_App_Process_Cyclic(){

	//画面
	Display_App_Process();

	//比較出力絡み
	Display_App_LED_Process();
	Display_App_Min_Max_Reset_Process();
}




//電源投入後直後のアニメーション
uint8_t Display_App_Show_Openning(uint8_t opening_state){

	static uint8_t brightness_counter = 160;
	uint8_t point = Display_Set_Default_Dot_Position();
	Button_Enabled(0);
	switch(opening_state){
		case 0://fullscale minus
			Display_Set_Brightness_Without_Backup(16);
			Display_LED_Comparison_State(1, 1);
			Display_LED_Comparison_State(0, 0);
			if(global_struct->eeprom.system.information.scale_type == 1){
				Display_Int_Decimal_Show(-global_struct->eeprom.system.information.fullscale_minus, point+1);
			}
			else{
				Display_Int_Decimal_Show(global_struct->eeprom.system.information.fullscale_minus, point+1);
			}
			return 0;

		case 1://fullscale plus
			Display_LED_Comparison_State(1, 0);
			Display_LED_Comparison_State(0, 1);
			Display_Int_Decimal_Show(global_struct->eeprom.system.information.fullscale_plus, point+1);
			return 0;

		case 2://version
			return 0;

		case 3://all
			Display_LED_Comparison_State(1, 1);
			Display_LED_Comparison_State(0, 1);
			Display_Set_Dot_Position(7);
			Display_Int_Decimal_Show(8888);
			return 0;

		case 4://disappearing animation (16times loop)
			Display_LED_Comparison_State(1, 1);
			Display_LED_Comparison_State(0, 1);
			Display_Set_Dot_Position(7);
			Display_Int_Decimal_Show(8888);
			Display_Set_Brightness_Without_Backup(brightness_counter/10);
			brightness_counter--;
			return 0;

		case 5://display off
			Display_LED_Comparison_State(1, 0);
			Display_LED_Comparison_State(0, 0);
			Display_Set_Dot_Position(0);
			Display_String_Show_Without_Dot("    ");
			return 0;

		case 6://display on
			display_app_status.mode_state.parent = MODE_PARENT_MAIN;
			display_app_status.mode_state.main = MODE_MAIN_PRESSURE;

			Display_Enabled(1);
			Sensor_Clear_Min_Max();
			Procedure_Timeout_Timer_Config(15000);//15sec
			return 0;

		case 7:
			Button_Enabled(1);
			return 1;

		default:
			return 0;
	}
}



void Display_App_Init(Global_TypeDef *_global_struct){
	global_struct = _global_struct;
	display_app_status.mode_state.parent = MODE_PARENT_MAIN;
	display_app_status.mode_state.main = MODE_MAIN_PRESSURE;
	display_app_status.mode_state.sub = 0;
	Display_App_Create_Sub_Menu();
}
