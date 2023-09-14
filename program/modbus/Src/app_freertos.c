/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "button.h"
#include "analog.h"
#include "communication.h"
#include "display.h"
#include "display_app.h"
#include "eeprom.h"
#include "sensor.h"
#include "comparison.h"
#include "global_typedef.h"
#include "procedure.h"
#include "compile_config.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
extern IWDG_HandleTypeDef hiwdg;
extern UART_HandleTypeDef huart1;
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */


Global_TypeDef *global_struct;
/* USER CODE END Variables */
osThreadId button_taskHandle;
osThreadId display_taskHandle;
osThreadId sensor_taskHandle;
osThreadId comparison_taskHandle;
osThreadId eeprom_taskHandle;
osThreadId communicationHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
   
/* USER CODE END FunctionPrototypes */

void RTOS_Button_Task(void const * argument);
void RTOS_Display_Task(void const * argument);
void RTOS_Sensor_Task(void const * argument);
void RTOS_Comparison_Task(void const * argument);
void RTOS_EEPROM_Task(void const * argument);
void RTOS_Communication_Task(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);

/* USER CODE BEGIN 4 */
//スタ�?クオーバ�?�フローでリセ�?�?
__weak void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
   /* Run time stack overflow checking is performed if
   configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2. This hook function is
   called if a stack overflow is detected. */
	HAL_NVIC_SystemReset();
}
/* USER CODE END 4 */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
       
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of button_task */
  osThreadDef(button_task, RTOS_Button_Task, osPriorityIdle, 0, 128);
  button_taskHandle = osThreadCreate(osThread(button_task), NULL);

  /* definition and creation of display_task */
  osThreadDef(display_task, RTOS_Display_Task, osPriorityHigh, 0, 128);
  display_taskHandle = osThreadCreate(osThread(display_task), NULL);

  /* definition and creation of sensor_task */
  osThreadDef(sensor_task, RTOS_Sensor_Task, osPriorityNormal, 0, 128);
  sensor_taskHandle = osThreadCreate(osThread(sensor_task), NULL);

  /* definition and creation of comparison_task */
  osThreadDef(comparison_task, RTOS_Comparison_Task, osPriorityIdle, 0, 128);
  comparison_taskHandle = osThreadCreate(osThread(comparison_task), NULL);

  /* definition and creation of eeprom_task */
  osThreadDef(eeprom_task, RTOS_EEPROM_Task, osPriorityIdle, 0, 128);
  eeprom_taskHandle = osThreadCreate(osThread(eeprom_task), NULL);

  /* definition and creation of communication */
  osThreadDef(communication, RTOS_Communication_Task, osPriorityHigh, 0, 128);
  communicationHandle = osThreadCreate(osThread(communication), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
	global_struct->ram.os_started = 1;
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_RTOS_Button_Task */
/**
  * @brief  Function implementing the button_task thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_RTOS_Button_Task */
void RTOS_Button_Task(void const * argument)
{
  /* USER CODE BEGIN RTOS_Button_Task */

  /* Infinite loop */
  for(;;)
  {
	  HAL_IWDG_Refresh(&hiwdg);

	  Display_Process_Cyclic();
	  Button_Process_Cyclic();
	  osDelay(10);
  }
  /* USER CODE END RTOS_Button_Task */
}

/* USER CODE BEGIN Header_RTOS_Display_Task */
/**
* @brief Function implementing the display_task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_RTOS_Display_Task */
void RTOS_Display_Task(void const * argument)
{
  /* USER CODE BEGIN RTOS_Display_Task */


	const uint8_t opening_index[] = {
		ANIMATION_FULLSCALE_MINUS,
		ANIMATION_FULLSCALE_PLUS,
		ANIMATION_LIGHTS_ALL,
		ANIMATION_LIGHTS_DISAPPEARING,
		ANIMATION_DISPLAY_OFF,
		ANIMATION_DISPLAY_ON,
		ANIMATION_FINISH,
	};
	const uint8_t opening_wait_time_10ms[] = {
		140,
		140,
		100,
		150,
		50,
		1,
		1,
	};

	global_struct->ram.opening_finished = 0;

	uint16_t time = 0;
	uint8_t index = 0;

	/* Infinite loop */
	for(;;){

		if(global_struct->ram.display_test_mode.enabled){
			Display_Test_App_Process_Cyclic(10);
		}
		else{
			if(!global_struct->ram.opening_finished){
				//opening animation
				global_struct->ram.opening_finished = Display_App_Show_Openning(opening_index[index]);

				if(time < opening_wait_time_10ms[index]){
					time++;
				}
				else{
					time = 0;
					index++;
				}
			}
			else{
				//main process
				Display_App_Process_Cyclic();
				Procedure_Process_Cyclic(10);

			}

		}
		osDelay(10);

	}
  /* USER CODE END RTOS_Display_Task */
}

/* USER CODE BEGIN Header_RTOS_Sensor_Task */
/**
* @brief Function implementing the sensor_task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_RTOS_Sensor_Task */
void RTOS_Sensor_Task(void const * argument)
{
  /* USER CODE BEGIN RTOS_Sensor_Task */
  /* Infinite loop */
  for(;;)
  {
	  Analog_Process_Cyclic();
	  Temperature_Process_Cyclic();
	  Sensor_Process_Cyclic();
	  osDelay(1);
  }
  /* USER CODE END RTOS_Sensor_Task */
}

/* USER CODE BEGIN Header_RTOS_Comparison_Task */
/**
* @brief Function implementing the comparison_task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_RTOS_Comparison_Task */
void RTOS_Comparison_Task(void const * argument)
{
  /* USER CODE BEGIN RTOS_Comparison_Task */
  /* Infinite loop */
  for(;;)
  {

	  if(global_struct->ram.display_test_mode.enabled){
		  Comparison_Test_Process_Cyclic();
	  }
	  else{
		  if(global_struct->ram.opening_finished){
			  Comparison_Main_Process_Cyclic(10);
		  }
	  }

	  osDelay(10);
  }
  /* USER CODE END RTOS_Comparison_Task */
}

/* USER CODE BEGIN Header_RTOS_EEPROM_Task */
/**
* @brief Function implementing the eeprom_task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_RTOS_EEPROM_Task */
void RTOS_EEPROM_Task(void const * argument)
{
  /* USER CODE BEGIN RTOS_EEPROM_Task */
  /* Infinite loop */
  for(;;)
  {
	  EEPROM_Process_Cyclic();
	  osDelay(100);
  }
  /* USER CODE END RTOS_EEPROM_Task */
}

/* USER CODE BEGIN Header_RTOS_Communication_Task */
/**
* @brief Function implementing the communication thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_RTOS_Communication_Task */
void RTOS_Communication_Task(void const * argument)
{
  /* USER CODE BEGIN RTOS_Communication_Task */
  /* Infinite loop */
	osDelay(500);
	Button_Maintenunce_Mode();
  for(;;)
  {
	  if(global_struct->ram.opening_finished){
		  Communication_Process_Cyclic();
	  }
	  osDelay(1);
  }
  /* USER CODE END RTOS_Communication_Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void RTOS_Init(Global_TypeDef *_global_struct){
	global_struct = _global_struct;

}
/* USER CODE END Application */

