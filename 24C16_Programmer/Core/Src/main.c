/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include "ee24c16.h"
#include "virtualcomport.h"
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
I2C_HandleTypeDef hi2c2;

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;

/* USER CODE BEGIN PV */
volatile char VCP_TX_BUFF[EE_TOTAL_SIZE];
volatile char VCP_RX_BUFF[VCP_BUFF_CMD_SRCH + EE_TOTAL_SIZE];

volatile VCP_FlagsTypeDef	VcpFlags;

VCP_AnswerTypeDef 	VcpAnswer;
EE_StatusTypeDef  	EEAnswer;
VCP_CommandTypeDef 	VcpCommands[VCP_COMMAND_COUNT];

const char* const EE_StatusNumToStr[] =
{
		[EE_OK]             				= "EE_OK",
		[EE_ERR_NULLPTR]        		= "EE_ERR_NULLPTR",
		[EE_ERR_EXCEED_TOTAL_SIZE]  = "EE_ERR_EXCEED_TOTAL_SIZE",
		[EE_ERR_NULLSIZE]       		= "EE_ERR_NULLSIZE",
		[EE_ERR_WP_MISCONFIG]   		= "EE_ERR_WP_MISCONFIG",
		[EE_ERR_TIMEOUT]        		= "EE_ERR_TIMEOUT",
		[EE_ERR_HAL_I2C]       			= "EE_ERR_HAL_I2C",
		[EE_ERR_DEVICE_I2C]     		= "EE_ERR_DEVICE_I2C"
};

const char* const VcpUserMessages[] =
{
		"\n\nIncorrect Input/Syntax/Command Try Again\n\
			\nCWrite={StartAddr} {Data}\
			\nCErase={StartAddr}-{StopAddr}\
			\nCRead={StartAddr}-{StopAddr}\n\n",
		"\n\nClosing... EEPROM Error Code:",
		"\n\nIDLE, Device Can Respond\n\n",
		"\n\nWrite Success\n\n",
		"\n\nErase Success\n\n"
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C2_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
static void VCP_InitCommands(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_I2C2_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  /* Assign commands */
  VCP_InitCommands();

  /* Enable Write */
  EE_WpControl(GPIO_PIN_RESET);

  /* Start listening terminal */
  if (HAL_UARTEx_ReceiveToIdle_DMA(&huart1, (uint8_t*)VCP_RX_BUFF, VCP_BUFF_CMD_SRCH + EE_TOTAL_SIZE) != HAL_OK)
  	Error_Handler();

  __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);

  /* Inform user to enter commands*/
	if (HAL_UART_Transmit_DMA(&huart1, (uint8_t*)VcpUserMessages[2], strlen(VcpUserMessages[2])) != HAL_OK)
		Error_Handler();
	else
		VcpFlags.SendingData = TRUE;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

  	/* Something was sent from terminal and this data copied to VCP_RX_BUFF */
  	if (VcpFlags.DataReceived)
  	{
  		/* Search for command in terminal data */
  		VcpAnswer = VCP_SearchCommand((char*)VCP_RX_BUFF, VcpFlags.CurrVcpRxSize, VcpCommands);

			switch (VcpAnswer.CmdName)
			{
			case (CWrite):
				/* VcpAnswer.StartAddr defines memory address from where write operation will start in EEPROM
				 * VcpAnswer.StartofDataInRxBuff is Offset value to exclude command and its parameter in VCP_RX_BUFF terminal data
				 * So "VCP_RX_BUFF + VcpAnswer.StartofDataInRxBuff" is starting point in array for copying meaningful data
				 * "VcpFlags.CurrVcpRxSize - VcpAnswer.StartofDataInRxBuff" is length of meaningful data
				 *
				 * Write to EEPROM
				 */
				EEAnswer = EE_Write(&hi2c2, VcpAnswer.StartAddr, (uint8_t*)(VCP_RX_BUFF + VcpAnswer.StartofDataInRxBuff),
						(VcpFlags.CurrVcpRxSize - VcpAnswer.StartofDataInRxBuff), EE_NORMALWRITE, EE_DEF_TIMEOUT);

				if (EEAnswer != EE_OK)
					Error_Handler();

	  	  while (VcpFlags.SendingData) {}

	  	  /* Infor user that write operation finished */
				if (HAL_UART_Transmit_DMA(&huart1, (uint8_t*) VcpUserMessages[3], strlen(VcpUserMessages[3])) != HAL_OK)
					Error_Handler();
				else
					VcpFlags.SendingData = TRUE;

				break;

			case (CErase):
				/* Erase EEPROM memory in line with given start and stop memory addresses */
				EEAnswer = EE_Write(&hi2c2, VcpAnswer.StartAddr, NULL, (VcpAnswer.StopAddr - VcpAnswer.StartAddr), EE_ERASEWRITE, EE_DEF_TIMEOUT);

				if (EEAnswer != EE_OK)
					Error_Handler();

				while (VcpFlags.SendingData) {}

				/* Inform user that erase operation finished */
				if (HAL_UART_Transmit_DMA(&huart1, (uint8_t*) VcpUserMessages[4], strlen(VcpUserMessages[4])) != HAL_OK)
					Error_Handler();
				else
					VcpFlags.SendingData = TRUE;

				break;

			case (CRead):
				/* Read EEPROM memory in line with given start and stop memory addresses */
			  EEAnswer = EE_Read(&hi2c2, VcpAnswer.StartAddr, (uint8_t*)VCP_TX_BUFF, (VcpAnswer.StopAddr - VcpAnswer.StartAddr), EE_DEF_TIMEOUT);
				if (EEAnswer != EE_OK)
					Error_Handler();

				while (VcpFlags.SendingData) {}

				/* Send the read data to user from terminal */
				if (HAL_UART_Transmit_DMA(&huart1, (uint8_t*)VCP_TX_BUFF, (VcpAnswer.StopAddr - VcpAnswer.StartAddr)) != HAL_OK)
					Error_Handler();
				else
					VcpFlags.SendingData = TRUE;

			  break;

			default:
				/* No command found */
				while (VcpFlags.SendingData) {}

				/* Inform user that program could not find command */
				if (HAL_UART_Transmit_DMA(&huart1, (uint8_t*)VcpUserMessages[0], strlen(VcpUserMessages[0])) != HAL_OK)
					Error_Handler();
				else
					VcpFlags.SendingData = TRUE;

				break;
			}

			/* Start listening terminal again */
  		VcpFlags.DataReceived = FALSE;
  	  if (HAL_UARTEx_ReceiveToIdle_DMA(&huart1, (uint8_t*)VCP_RX_BUFF, VCP_BUFF_CMD_SRCH + EE_TOTAL_SIZE) != HAL_OK)
  	  	Error_Handler();

  	  __HAL_DMA_DISABLE_IT(&hdma_usart1_rx, DMA_IT_HT);

  	  while (VcpFlags.SendingData) {}

  	  /* Inform user to enter commands*/
  		if (HAL_UART_Transmit_DMA(&huart1, (uint8_t*)VcpUserMessages[2], strlen(VcpUserMessages[2])) != HAL_OK)
  			Error_Handler();
  		else
  			VcpFlags.SendingData = TRUE;
  	}

  	/* Do Other Stuff */
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 90;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 400000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream2_IRQn);
  /* DMA2_Stream7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/**
 *
 */
static void VCP_InitCommands(void)
{
	VcpCommands[CWrite].vcp_command = "CWrite=";
	VcpCommands[CErase].vcp_command = "CErase=";
	VcpCommands[CRead].vcp_command = "CRead=";

	for (uint8_t i = 0; i < VCP_COMMAND_COUNT; i++) {
		VcpCommands[i].size = strlen(VcpCommands[i].vcp_command);
	}
}

/**
 *
 * @param huart
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	Error_Handler();
}

/**
 *
 * @param huart
 * @param Size
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if (huart->Instance == USART1)
	{
		VCP_RX_BUFF[Size] = '\0';

		if (!VcpFlags.DataReceived)
		{
			/* Receive line is free */
			VcpFlags.DataReceived = TRUE;

			/* Store size of terminal data */
			VcpFlags.CurrVcpRxSize = Size;
		}
		else
		{
			Error_Handler();
		}
	}
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart->Instance == USART1)
	{
		/* Transmit line is free */
		VcpFlags.SendingData = FALSE;
	}
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
	if (EEAnswer != EE_OK)
	{
		/* Send EEPROM error code */
		HAL_UART_Transmit(&huart1, (uint8_t*)VcpUserMessages[1], strlen(VcpUserMessages[1]), 1000);
		HAL_UART_Transmit(&huart1, (uint8_t*)EE_StatusNumToStr[EEAnswer], strlen(EE_StatusNumToStr[EEAnswer]), 1000);
	}
	else
	{
		char* msg = "Program Related Error, Not EEPROM";
		HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 1000);
	}

	//__disable_irq();
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
