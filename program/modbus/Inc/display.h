#ifndef DISPLAY_H_
#define DISPLAY_H_

#include "main.h"

//PIN Defines
#define DSP_D5		GPIOC, GPIO_PIN_15
#define DSP_DATA	GPIOC, GPIO_PIN_14
#define DSP_CLK		GPIOB, GPIO_PIN_9


#define D5_LOW()		(HAL_GPIO_WritePin(DSP_D5, 0))
#define D5_HIGH()		(HAL_GPIO_WritePin(DSP_D5, 1))
#define DATA_LOW()		(HAL_GPIO_WritePin(DSP_DATA, 0))
#define DATA_HIGH()		(HAL_GPIO_WritePin(DSP_DATA, 1))
#define CLK_LOW()		(HAL_GPIO_WritePin(DSP_CLK, 0))
#define CLK_HIGH()		(HAL_GPIO_WritePin(DSP_CLK, 1))


#define SEGMENT_NUM_0	(uint8_t)0b11111100			/* 0x30, ����0	 	*/
#define SEGMENT_SYMBOL_HYPHEN	(uint8_t)0b00000010	/* 0x2D, �L��- 		*/
#define SEGMENT_SYMBOL_UNDERBAR	(uint8_t)0b00010000	/* 0x5F, �L��_		*/
#define SEGMENT_SYMBOL_OVERBAR	(uint8_t)0B10000000	/* 		, �� "-"		*/
#define SEGMENT_BLANK	(uint8_t)0b00000000			/* 0x20, �u�����N		*/


static const uint8_t _led_alphabet_segment_data[] = {
	0b11101110,	/* 0x41,A 	*/
	0b00111110,	/* 0x42,B 	*/
	0b10011100,	/* 0x43,C 	*/
	0b01111010,	/* 0x44,D 	*/
	0b10011110,	/* 0x45,E 	*/
	0b10001110,	/* 0x46,F 	*/
	0b10111100,	/* 0x47,G 	*/
	0b01101110,	/* 0x48,H 	*/
	0b00001000,	/* 0x49,I 	*/
	0b10110000,	/* 0x4A,J 	*/
	0b01001110,	/* 0x4B,K 	*/
	0b00011100,	/* 0x4C,L 	*/
	0b10101010,	/* 0x4D,M 	*/
	0b00101010,	/* 0x4E,N 	*/
	0b00111010,	/* 0x4F,O 	*/
	0b11001110,	/* 0x50,P 	*/
	0b11100110,	/* 0x51,Q 	*/
	0b00001010,	/* 0x52,R 	*/
	0b10110110,	/* 0x53,S 	*/
	0b10001100,	/* 0x54,T 	*/
	0b01111100,	/* 0x55,U 	*/
	0b00111000,	/* 0x56,V 	*/
	0b01010110,	/* 0x57,W 	*/
	0b01101100,	/* 0x58,X 	*/
	0b01110110,	/* 0x59,Y 	*/
	0b11011000,	/* 0x5A,Z 	*/
};

static const uint8_t _led_num_segment_data[] = {
	0b11111100,	/* 0,0 	*/
	0b01100000,	/* 1,1 	*/
	0b11011010,	/* 2,2 	*/
	0b11110010,	/* 3,3 	*/
	0b01100110,	/* 4,4 	*/
	0b10110110,	/* 5,5 	*/
	0b10111110,	/* 6,6 	*/
	0b11100100,	/* 7,7 	*/
	0b11111110,	/* 8,8 	*/
	0b11110110,	/* 9,9 	*/
	0b11101110,	/* 10,A */
	0b00111110,	/* 11,B */
	0b10011100,	/* 12,C */
	0b01111010,	/* 13,D */
	0b10011110,	/* 14,E */
	0b10001110,	/* 15,F */
};

typedef enum {
	DISPLAY_PLUS,
	DISPLAY_MINUS
}DISPLAY_PLUS_MINUS;

typedef enum {
	DISPLAY_MODE_HEX = 1,
	DISPLAY_MODE_INT_DECIMAL = 1<<1,
	DISPLAY_MODE_UINT_DECIMAL = 1<<2,
	DISPLAY_MODE_STRING = 1<<3,
	DISPLAY_MODE_ZERO_FILL = 1<<4,
	DISPLAY_MODE_BLANK  = 1<<5,
	DISPLAY_MODE_PROCESSBAR = 1<<6,
	DISPLAY_MODE_ERROR = 1<<7,
	DISPLAY_MODE_TOP_STRING_AND_HEX = 1<<8,
	DISPLAY_MODE_TOP_STRING_AND_UINT_DECIMAL = 1<<9,
	DISPLAY_MODE_STRING_WITHOUT_DOT = 1<<10,
	DISPLAY_MODE_DISPLAY_TEST_MODE = 1<<11,
}DISPLAY_SHOW_MODE;


typedef struct{
	uint8_t period_ms;
	uint8_t enabled;
	uint8_t times;
	uint8_t finished;
}Display_Blink_TypeDef;

typedef struct{
	uint8_t period_ms;
	uint8_t enabled;
	uint8_t finished;
}Display_Lit_TypeDef;

typedef struct{
	uint8_t enabled;
	uint8_t counter;
}Display_ProcessBar_TypeDef;


typedef struct{
	/*
	 * 	dot_position
	 * 		0:x x x x 1:x x x.x 2:x x.x x 3:x x.x.x
	 * 		4:x.x x x 5:x.x x.x 6:x.x.x x 7:x.x.x.x
	 *  show_digit
	 *  	4:hi_lo_led
	 *  	0 ~ 3:digits
	 */

	uint8_t led[2];
	uint8_t dot_position;
	uint8_t eneble_digits;
	uint8_t show_digit_counter;
	uint8_t digit_buffer[4];
	uint8_t str_buffer[4];
	int16_t value;
	uint8_t effective_digits;

	DISPLAY_PLUS_MINUS plus_minus;
	DISPLAY_SHOW_MODE show_mode;
	uint8_t eco_mode;

	uint8_t brightness;



	Display_Blink_TypeDef blink;
	Display_Lit_TypeDef lit;
	Display_ProcessBar_TypeDef process_bar;
}Display_IO_TypeDef;


static const uint8_t _led_process_bar[3][4] = {
	" ---",
	"  --",
	"   -"
};


typedef enum {
	TEST,
}DISPLAY_APP_MODE_STATE;


#endif
