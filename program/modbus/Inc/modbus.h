/*
 * modbus.h
 *
 *  Created on: 2021/10/26
 *      Author: staff
 */

#ifndef MODBUS_H_
#define MODBUS_H_



typedef enum{
	MA_COMPARISON1_OUTPUT_STATE,
	MA_COMPARISON2_OUTPUT_STATE,
}MODBUS_ADDRESS_0x01;

typedef enum{
	MA_BUTTON_STATUS,
}MODBUS_ADDRESS_0x02;

typedef enum{

	MA_PRESSURE,
	MA_PRESSURE_PERCENTAGE,
	MA_PRESSURE_MAX,
	MA_PRESSURE_MIN,
	MA_ERROR_CODE,
	MA_STATUS_CODE,//追加


	MA_SENSOR_ADC = 1000,
	MA_TEMPERATURE = 1002,
	MA_VDD,

}MODBUS_ADDRESS_0x04;

typedef enum{
	MA_ZERO_ADJUSTMENT,
	MA_CLEAR_MAX_MIN,
	MA_CLEAR_MAX,
	MA_CLEAR_MIN,
	MA_CLEAR_ERROR,

	MA_REBOOT = 100,
	MA_FACTORY_RESET = 200,

	MA_BACKUP_PAUSE = 9000,
	MA_BACKUP_FORCED_START,
	MA_DISPLAY_TEST_MODE,
	MA_OSC_ACTIVATED,
}MODBUS_ADDRESS_0x05;


typedef enum{
	//EEPROM
	MA_USER_MODE_ECO_MODE = 0,
	MA_USER_MODE_PROTECTED = 1,
	MA_USER_MODE_PRODUCTION_NUMBER = 2,

	MA_USER_MODE_PRESSURE_RANGE_ID = 4,
	MA_USER_DISPLAY_LOW_CUT = 5,
	MA_USER_DISPLAY_BRIGHTNESS = 6,
	MA_USER_DISPLAY_SIGN_INVERSION = 7,
	MA_USER_DISPLAY_AVERAGE_TIME = 8,
	MA_USER_OUTPUT_OUTPUT_ENABLED = 9,
	MA_USER_OUTPUT_AVERAGE_TIME = 10,
	MA_USER_OUTPUT_OUTPUT_INVERSION = 11,
	MA_USER_COMMUNICATION_MESSAGE_PROTOCOL = 12,
	MA_USER_COMMUNICATION_SLAVE_ADDRESS = 13,

	MA_USER_COMMUNICATION_BAUD_RATE = 15,
	MA_USER_COMMUNICATION_PARITY = 16,
	MA_USER_COMMUNICATION_STOP_BIT = 17,
	MA_USER_COMPARISON1_POWER_ON_DELAY = 18,
	MA_USER_COMPARISON1_OUTPUT_TYPE = 19,
	MA_USER_COMPARISON1_OFF_DELAY = 20,
	MA_USER_COMPARISON1_ON_DELAY = 21,
	MA_USER_COMPARISON1_MODE = 22,
	MA_USER_COMPARISON1_ENABLED = 23,
	MA_USER_COMPARISON1_HYS_P1 = 24,
	MA_USER_COMPARISON1_HYS_P2 = 25,
	MA_USER_COMPARISON1_WIN_HI = 26,
	MA_USER_COMPARISON1_WIN_LOW = 27,
	MA_USER_COMPARISON2_POWER_ON_DELAY = 28,
	MA_USER_COMPARISON2_OUTPUT_TYPE = 29,
	MA_USER_COMPARISON2_OFF_DELAY = 30,
	MA_USER_COMPARISON2_ON_DELAY = 31,
	MA_USER_COMPARISON2_MODE = 32,
	MA_USER_COMPARISON2_ENABLED = 33,
	MA_USER_COMPARISON2_HYS_P1 = 34,
	MA_USER_COMPARISON2_HYS_P2 = 35,
	MA_USER_COMPARISON2_WIN_HI = 36,
	MA_USER_COMPARISON2_WIN_LOW = 37,
	MA_SENSOR_ZERO_ADJUSTMENT_COUNT = 1000,

	MA_SENSOR_ZERO_ADJUSTMENT_DISPLAY = 1002,

	MA_SENSOR_ZERO_ADJUSTMENT_OUTPUT = 1004,

	MA_SYSTEM_INFORMATION_SETTING_STATUS_FLAG = 1006,
	MA_SYSTEM_INFORMATION_MAINTENANCE_MODE = 1007,
	MA_SYSTEM_INFORMATION_SOFTWARE_VERSION = 1008,
	MA_SYSTEM_INFORMATION_HARDWARE_VERSION = 1009,
	MA_SYSTEM_INFORMATION_MECHANISM_VERSION = 1010,
	MA_SYSTEM_INFORMATION_PRODUCTION_NUMBER = 1011,

	MA_SYSTEM_INFORMATION_CUSTOMER_CODE = 1013,
	MA_SYSTEM_INFORMATION_PRESSURE_RANGE_ID = 1014,
	MA_SYSTEM_INFORMATION_POINT_POSITION = 1015,
	MA_SYSTEM_INFORMATION_SCALE_TYPE = 1016,
	MA_SYSTEM_INFORMATION_FULLSCALE_PLUS = 1017,
	MA_SYSTEM_INFORMATION_FULLSCALE_MINUS = 1018,
	MA_SYSTEM_INFORMATION_SCALE_MARGIN = 1019,
	MA_SYSTEM_INFORMATION_OUTPUT_TYPE = 1020,
	MA_SYSTEM_INFORMATION_OUTPUT_PHY_PROTOCOL = 1021,
	MA_SYSTEM_INFORMATION_COMPARISON_COUNT = 1022,
	MA_SYSTEM_INFORMATION_ANALOG_COUNT = 1023,
	MA_SYSTEM_INFORMATION_DIGITAL_COUNT = 1024,
	MA_SYSTEM_CORRECTION_VALUE_ZERO_OFFSET_FINE = 1025,
	MA_SYSTEM_CORRECTION_VALUE_ZERO_OFFSET_ROUGH = 1026,
	MA_SYSTEM_CORRECTION_VALUE_CORRECTION_COUNT = 1027,
	MA_SYSTEM_CORRECTION_VALUE_INSPECTION_TEMPERATURE = 1028,
	MA_SYSTEM_CORRECTION_VALUE_INSPECTION_TEMPERATURE_OFFSET = 1029,
	MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_REFERENCE = 1030,
	MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_REFERENCE_ADC = 1031,
	MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_ZERO = 1032,
	MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_SPAN = 1033,
	MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_COUNT = 1034,
	MA_SYSTEM_CORRECTION_VALUE_CORRECTION_00 = 1035,
	MA_SYSTEM_CORRECTION_VALUE_CORRECTION_01 = 1036,
	MA_SYSTEM_CORRECTION_VALUE_CORRECTION_02 = 1037,
	MA_SYSTEM_CORRECTION_VALUE_CORRECTION_03 = 1038,
	MA_SYSTEM_CORRECTION_VALUE_CORRECTION_04 = 1039,
	MA_SYSTEM_CORRECTION_VALUE_CORRECTION_05 = 1040,
	MA_SYSTEM_CORRECTION_VALUE_CORRECTION_06 = 1041,
	MA_SYSTEM_CORRECTION_VALUE_CORRECTION_07 = 1042,
	MA_SYSTEM_CORRECTION_VALUE_CORRECTION_08 = 1043,
	MA_SYSTEM_CORRECTION_VALUE_CORRECTION_09 = 1044,
	MA_SYSTEM_CORRECTION_VALUE_CORRECTION_10 = 1045,
	MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_00 = 1046,
	MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_01 = 1047,
	MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_02 = 1048,
	MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_03 = 1049,
	MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_04 = 1050,
	MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_05 = 1051,
	MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_06 = 1052,
	MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_07 = 1053,
	MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_08 = 1054,
	MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_09 = 1055,
	MA_SYSTEM_CORRECTION_VALUE_TEMPERATURE_10 = 1056,
	MA_SYSTEM_CORRECTION_VALUE_DAC_MIN = 1057,
	MA_SYSTEM_CORRECTION_VALUE_DAC_MAX = 1058,
	MA_SYSTEM_CORRECTION_VALUE_DAC_CENTER = 1059,





	//RAM
	MA_OSC_PERIOD = 9000,
	MA_OSC_PWM_VALUE,
	MA_DISPLAY_TEST_DIGIT_00,
	MA_DISPLAY_TEST_DIGIT_01,
	MA_DISPLAY_TEST_DIGIT_02,
	MA_DISPLAY_TEST_DIGIT_03,
	MA_DISPLAY_TEST_LED,
}MODBUS_ADDRESS_0x03;

//
//
//typedef struct{
//	uint16_t *user_array[100];
//	uint8_t user_size_array[100];
//	uint16_t *system_array[100];
//	uint8_t system_size_array[100];
//}MODBUS_EEPROM_TypeDef;


#endif /* MODBUS_H_ */
