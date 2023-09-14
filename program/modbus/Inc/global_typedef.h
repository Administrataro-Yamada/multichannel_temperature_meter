/*
 * global_typedef.h
 *
 *  Created on: 2021/10/19
 *      Author: oki
 */

#ifndef GLOBAL_TYPEDEF_H_
#define GLOBAL_TYPEDEF_H_



#include "stdint.h"
#include "status.h"

#define UART_BUFFER_SIZE 32


typedef struct{
	uint8_t eco_mode;
	uint8_t protected;
}EEPROM_Mode_TypeDef;

typedef struct{
	uint32_t *production_number;		//0850001
	uint8_t *pressure_range_id;		//別紙参照
}EEPROM_User_Information_TypeDef;

typedef struct{
	uint8_t low_cut;
	uint8_t brightness;
	uint8_t sign_inversion;
	uint8_t average_time;
}EEPROM_Display_TypeDef;

typedef struct{
	uint8_t output_enabled;		//0:無効 1:有効
	uint8_t average_time;		//0：200 1：1000 2：4000ms
	uint8_t output_inversion;	//0:無効 1:有効
}EEPROM_Output_TypeDef;

typedef struct{
	uint8_t message_protocol;	//0:無手順 1:MODBUS/RTU 2:MODBUS/ASCII
	uint32_t slave_address;		//0x00～0xff
	uint8_t baud_rate;
	/*
	 * 0x00:1200 0x01:2400 0x02:4800 0x03:9600 0x04:14400 0x05:19200
	 * 0x06:38600 0x07:57600 0x08:115200 0x09:230400 0x0A:460800 0x0B:921600 baud
	 */
	uint8_t parity;				//0:なし 1:偶数 2:奇数
	uint8_t stop_bit;			//0:1bit 1:2bit
}EEPROM_Communication_TypeDef;

typedef struct{
	uint8_t power_on_delay;	//0～20min
	uint8_t output_type;	//0x00：PNP 0x01：NPN
	uint8_t off_delay;		//0.0～2.0s(0~20)
	uint8_t on_delay;		//0.0～2.0s(0~20)
	uint8_t mode;			//0：ヒステリシス 1:ウィンドウ
	uint8_t enabled;		//0:無効 1:OFF 2:NC 3:NO
	int16_t hys_p1;				//ヒステリシスモード
	int16_t hys_p2;
	int16_t win_hi;				//ウィンドウモード
	int16_t win_low;
}EEPROM_Comparison_TypeDef;

typedef struct{
	uint8_t backup_request;
	EEPROM_Mode_TypeDef mode;
	EEPROM_User_Information_TypeDef user_information;
	EEPROM_Display_TypeDef display;
	EEPROM_Output_TypeDef output;
	EEPROM_Communication_TypeDef communication;
	EEPROM_Comparison_TypeDef comparison1;
	EEPROM_Comparison_TypeDef comparison2;
}EEPROM_User_TypeDef;

typedef struct{
	uint32_t count;
	int32_t display;
	int32_t output;
}EEPROM_Zero_Adjustment_TypeDef;

typedef struct{
	uint8_t backup_request;
	EEPROM_Zero_Adjustment_TypeDef zero_adjustment;
}EEPROM_Sensor_TypeDef;


typedef enum{
	CC_FULL_FUNCTION = (uint8_t)0,
	CC_GENERAL_FUNCTION = (uint8_t)1,
	CC_WITHOUT_INVERTING_FUNCTION = (uint8_t)2,
	CC_WITHOUT_OUTPUT_OPTION_FUNCTION = (uint8_t)3,
	CC_WITHOUT_OUTPUT_COMPARISON_FUNCTION = (uint8_t)4,
}Customer_Code_TypeDef;

typedef struct{
	uint8_t setting_status_flag;	//0b0001：書込み済み 0b0010：検査済み 0b0100:通信設定済み
	uint8_t maintenance_mode;		//0:通常モード 1:メンテナンスモード
	uint8_t software_version;		//0～255
	uint8_t hardware_version;
	uint8_t mechanism_version;
	uint32_t production_number;		//0850001
	uint8_t customer_code;			//もともとの型定義
	uint8_t pressure_range_id;		//別紙参照
	uint8_t point_position;			//0：0000 1：000.0 2：00.00 3：0.000
	uint8_t scale_type;				//0：片圧 1：両圧 2：負圧
	uint16_t fullscale_plus;
	uint16_t fullscale_minus;
	uint8_t scale_margin;			//±10%
	uint16_t output_type;			//0b0000：なし 0b0001：接点出力 0b0010：アナログ出力 0b0100：デジタル出力
	uint8_t output_phy_protocol;	//別紙参照
	uint8_t comparison_count;
	uint8_t analog_count;
	uint8_t digital_count;
}EEPROM_Information_TypeDef;

typedef struct{
	uint16_t zero_offset_fine;	//TIM3_CH1
	uint16_t zero_offset_rough;	//TIM3_CH3/CH4
	uint16_t correction_count;	//0～9
	int16_t inspection_temperature; //-32768 ～ +32768
	int16_t inspection_temperature_offset;
	int16_t temperature_reference;	//-32768 ～ +32768
	uint16_t temperature_reference_adc;
	int16_t temperature_zero;	// +-ppm/deg
	int16_t temperature_span;	// +-ppm/deg
	uint16_t temperature_count;	//0～11
	uint16_t correction[11];
	uint16_t temperature[11];
	uint16_t dac_min;
	uint16_t dac_max;
	uint16_t dac_center;
}EEPROM_Correction_Value_TypeDef;



typedef struct{
	uint8_t backup_request;
	EEPROM_Information_TypeDef information;
	EEPROM_Correction_Value_TypeDef correction_value;
}EEPROM_System_TypeDef;

typedef struct{
	uint8_t backup_paused;
	uint8_t backup_request_all;
	uint8_t load_request;
	EEPROM_User_TypeDef user;
	EEPROM_Sensor_TypeDef sensor;
	EEPROM_System_TypeDef system;

}EEPROM_TypeDef;



typedef struct{
	uint8_t all : 1;
	uint8_t any : 1;
	uint8_t ent : 1;
	uint8_t up : 1;
	uint8_t down : 1;
	uint8_t mode : 1;
}RAM_Button_Very_Long_To_Released;

typedef struct{
	uint8_t all : 1;
	uint8_t any : 1;
	uint8_t ent : 1;
	uint8_t up : 1;
	uint8_t down : 1;
	uint8_t mode : 1;
}RAM_Button_Very_Long_Pressed;

typedef struct{
	uint8_t all : 1;
	uint8_t any : 1;
	uint8_t ent : 1;
	uint8_t up : 1;
	uint8_t down : 1;
	uint8_t mode : 1;
}RAM_Button_Continuously_Pressed;

//今押しているボタン
typedef struct{
	uint8_t all : 1;
	uint8_t any : 1;
	uint8_t ent : 1;
	uint8_t up : 1;
	uint8_t down : 1;
	uint8_t mode : 1;
}RAM_Button_Now_Pressed;


//押して離した直後に押されていたボタン
typedef struct{
	uint8_t all : 1;
	uint8_t any : 1;
	uint8_t ent : 1;
	uint8_t up : 1;
	uint8_t down : 1;
	uint8_t mode : 1;
}RAM_Button_Clicked;



typedef struct{
	uint8_t enabled : 1;
	RAM_Button_Continuously_Pressed continuously;
	RAM_Button_Very_Long_Pressed very_long;
	RAM_Button_Now_Pressed now;
	RAM_Button_Clicked clicked;
	RAM_Button_Very_Long_To_Released released;
}RAM_Button_TypeDef;


typedef struct{
	uint8_t channel;
	uint8_t state : 1;
	uint8_t delayed_state : 1;
	uint8_t output_transistor_state : 1;
	uint8_t state_changed : 1;
	EEPROM_Comparison_TypeDef *setting;
}RAM_Comparison_TypeDef;

typedef struct{
	int16_t percentage_display_raw; //100.00% -> 10000
	int16_t percentage_output_raw; //100.00% -> 10000
	int16_t percentage_display_adjusted; //100.00% -> 10000
	int16_t percentage_output_adjusted; //100.00% -> 10000
	int16_t pascal_display; //5.0000kpa -> 50000, 500.00pa -> 50000, 50.000pa -> 50000, +-100.00pa -> -10000 ~ +10000
	int16_t pascal_output;
	uint8_t overflowed_display;
	uint8_t underflowed_display;

	uint8_t overflowed_output;
	uint8_t underflowed_output;

	uint32_t adc_display;
	uint32_t adc_output;
	int16_t percentage_calibrated_output;
	int16_t percentage_calibrated_display;
	uint16_t adc_temperature;
	uint16_t adc_vdd;

	int16_t pressure_max;
	int16_t pressure_min;

	uint8_t osc_activated;
	uint16_t osc_period;
	uint16_t osc_pwm_value;
}RAM_Pressure_TypeDef;

typedef struct{
	uint8_t rx_buffer[UART_BUFFER_SIZE];
	uint8_t tx_buffer[UART_BUFFER_SIZE];
	uint8_t rx_buffer_size;
	uint8_t tx_buffer_size;
}RAM_Uart_TypeDef;

typedef struct{

	int16_t temperature;
	uint16_t vdd;
	uint8_t flag_calibration;

}RAM_Temperature_TypeDef;

typedef struct{

	ERROR_CODE error_code;
	STATUS_CODE status_code;

	uint8_t mode_state;//0:pressure 1:max-min 2:test
	uint8_t zero_adjustment_flag;
	uint8_t clear_max_min_flag;
	uint8_t clear_max_flag;
	uint8_t clear_min_flag;
	uint8_t clear_error_flag;
	uint8_t reboot_flag;
	uint8_t factory_reset_flag;


	uint8_t comparison1_on_delay_flag;
	uint8_t comparison1_off_delay_flag;
	uint8_t comparison1_power_on_delay_flag;
	uint8_t comparison2_on_delay_flag;
	uint8_t comparison2_off_delay_flag;
	uint8_t comparison2_power_on_delay_flag;

}RAM_Status_And_Flags_TypeDef;

typedef struct{
	uint8_t enabled : 1;
	uint8_t segments[4];
	uint8_t leds[2];
}RAM_Display_Test_Mode_TypeDef;

typedef struct{
	RAM_Comparison_TypeDef comparison1;
	RAM_Comparison_TypeDef comparison2;
	RAM_Pressure_TypeDef pressure;
	RAM_Button_TypeDef button;
	RAM_Uart_TypeDef uart;
	RAM_Temperature_TypeDef temperature;
	RAM_Status_And_Flags_TypeDef status_and_flags;
	RAM_Display_Test_Mode_TypeDef display_test_mode;
	uint8_t opening_finished : 1;
	uint8_t os_started : 1;
}RAM_TypeDef;

typedef struct{
	EEPROM_TypeDef eeprom;
	RAM_TypeDef ram;
}Global_TypeDef;



#endif /* GLOBAL_TYPEDEF_H_ */
