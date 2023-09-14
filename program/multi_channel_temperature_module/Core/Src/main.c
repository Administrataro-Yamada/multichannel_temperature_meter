/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "cmsis_os.h"
#include "adc.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"

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

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// external function


typedef struct{
	uint16_t counter_period;
	uint8_t finished : 1;
	uint8_t enabled : 1;
	uint8_t paused : 1;
	TIM_HandleTypeDef *htim;
}Communication_Interval_Timer_TypeDef;

static Communication_Interval_Timer_TypeDef interval_timer = {0};

void UART_Reflesh(UART_HandleTypeDef *huart, uint8_t *buffer, uint8_t size){
	HAL_UART_AbortReceive_IT(huart);
	HAL_UART_Receive_DMA(huart, buffer, size);
}

void UART_Wtite(UART_HandleTypeDef *huart, uint8_t *data, uint8_t len){
	HAL_UART_Transmit_DMA(huart, data, len);
}

void UART_Reset_Error(UART_HandleTypeDef *huart){
	if (
		__HAL_UART_GET_FLAG(huart, UART_FLAG_ORE) ||
		__HAL_UART_GET_FLAG(huart, UART_FLAG_NE) ||
		__HAL_UART_GET_FLAG(huart, UART_FLAG_FE) ||
		__HAL_UART_GET_FLAG(huart, UART_FLAG_PE)
		){
		UART_Functions->Reflesh((uint8_t*)uart_rx_buffer, UART_BUFFER_SIZE);
	}
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){

}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart){
	if(huart->Instance==USART1){
		UART_Functions->Reflesh((uint8_t*)uart_rx_buffer, UART_BUFFER_SIZE);
	}
}


void UART_Config_Reflesh(UART_HandleTypeDef *huart, uint8_t baudrate){


	//0x00:1200 0x01:2400 0x02:4800 0x03:9600 0x04:14400 0x05:19200
	//0x06:38600 0x07:57600 0x08:115200 0x09:230400 0x0A:460800 0x0B:921600 baud
	if(baudrate <= 0x0b){
		huart->Init.BaudRate = baudrate];
		Interval_Timer_Functions->Set_Period(INTERVAL_TIME[baudrate]);
	}
	else{
		huart->Init.BaudRate = BAUDRATE[3];//9600baud
		Interval_Timer_Functions->Set_Period(INTERVAL_TIME[3]);
	}


	huart->Init.WordLength = UART_WORDLENGTH_8B;
	huart->Init.StopBits = UART_STOPBITS_1;
	huart->Init.Parity = UART_PARITY_NONE;
	huart->Init.Mode = UART_MODE_TX_RX;
	huart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart->Init.OverSampling = UART_OVERSAMPLING_16;
	huart->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	huart->Init.ClockPrescaler = UART_PRESCALER_DIV1;
	huart->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;


	if (HAL_RS485Ex_Init(huart, UART_DE_POLARITY_HIGH, 0, 0) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_UARTEx_SetTxFifoThreshold(huart, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_UARTEx_SetRxFifoThreshold(huart, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_UARTEx_DisableFifoMode(huart) != HAL_OK)
	{
		Error_Handler();
	}
}

//3.5T Counter Period
static const uint16_t INTERVAL_TIME[] = {
	//0x00:1200 0x01:2400 0x02:4800 0x03:9600 0x04:14400 0x05:19200
	//0x06:38400 0x07:57600 0x08:115200 0x09:230400 0x0A:460800 0x0B:921600 baud
	29167,
	14584,
	7292,
	3646,
	2431,
	1823,
	912,
	608,
	304,
	152,
	76,
	38,
};

static const uint32_t BAUDRATE[] = {
	//0x00:1200 0x01:2400 0x02:4800 0x03:9600 0x04:14400 0x05:19200
	//0x06:38400 0x07:57600 0x08:115200 0x09:230400 0x0A:460800 0x0B:921600 baud
	1200,
	2400,
	4800,
	9600,
	14400,
	19200,
	38400,
	57600,
	115200,
	230400,
	460800,
	921600,
};





static void Interval_Timer_Start(){
	if(!interval_timer.enabled){
		interval_timer.enabled = 1;
		//__HAL_TIM_SET_COUNTER(interval_timer.htim, 0);
		__HAL_TIM_CLEAR_FLAG(interval_timer.htim, TIM_FLAG_UPDATE);
		HAL_TIM_Base_Start_IT(interval_timer.htim);
	}
}

void Interval_Timer_Timeout(){
	interval_timer.enabled = 0;
	interval_timer.finished = 1;
	HAL_TIM_Base_Stop_IT(interval_timer.htim);
}


static void Interval_Timer_Set_Period(uint16_t counter_period){
	interval_timer.counter_period = counter_period;
	interval_timer.htim->Init.Period = counter_period;
	if ( (interval_timer.htim) != HAL_OK)
	{
		Error_Handler();
	}
	if (HAL_TIM_OnePulse_Init(interval_timer.htim, TIM_OPMODE_SINGLE) != HAL_OK)
	{
		Error_Handler();
	}
	TIM_MasterConfigTypeDef sMasterConfig = {0};
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(interval_timer.htim, &sMasterConfig) != HAL_OK)
	{
		Error_Handler();
	}
}

static uint8_t Interval_Timer_IsFinished(){
	uint8_t b = interval_timer.finished;
	if(b){
		interval_timer.finished = 0;
	}
	return b;
}



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Call init function for freertos objects (in freertos.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();
  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSIDiv = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 8;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM14 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM14) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
