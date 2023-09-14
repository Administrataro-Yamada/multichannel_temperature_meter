/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
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

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
osThreadId analogHandle;
osThreadId communicationHandle;
osThreadId memoryHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void Analog_Task(void const * argument);
void Communication_Task(void const * argument);
void Memory_Task(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

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
  /* definition and creation of analog */
  osThreadDef(analog, Analog_Task, osPriorityIdle, 0, 128);
  analogHandle = osThreadCreate(osThread(analog), NULL);

  /* definition and creation of communication */
  osThreadDef(communication, Communication_Task, osPriorityIdle, 0, 128);
  communicationHandle = osThreadCreate(osThread(communication), NULL);

  /* definition and creation of memory */
  osThreadDef(memory, Memory_Task, osPriorityIdle, 0, 128);
  memoryHandle = osThreadCreate(osThread(memory), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_Analog_Task */
/**
  * @brief  Function implementing the analog thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_Analog_Task */
void Analog_Task(void const * argument)
{
  /* USER CODE BEGIN Analog_Task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Analog_Task */
}

/* USER CODE BEGIN Header_Communication_Task */
/**
* @brief Function implementing the communication thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Communication_Task */
void Communication_Task(void const * argument)
{
  /* USER CODE BEGIN Communication_Task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Communication_Task */
}

/* USER CODE BEGIN Header_Memory_Task */
/**
* @brief Function implementing the memory thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_Memory_Task */
void Memory_Task(void const * argument)
{
  /* USER CODE BEGIN Memory_Task */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END Memory_Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

