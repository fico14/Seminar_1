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
#include "usb_host.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "util.h"
#include "fonts.h"
#include "ili9341.h"
#include "ili9341_touch.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MAX_STEP 8
#define FAST SPI_BAUDRATEPRESCALER_8
#define SLOW SPI_BAUDRATEPRESCALER_128
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi3;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
volatile uint8_t MHzInUse = 0;		//početno se frekvencija prikazuje u kHz
volatile uint32_t frequency = 100100000;//frekvencija mi je spremljena u Hz, ali će prikaz biti u kHz ili MHz
volatile uint8_t keyboardInUse = 0;
volatile uint8_t refreshScreen = 0;//mijenjanje vrijednosti frekvecije na grafičkom pokazniku
static uint32_t steps[] = { 10, 100, 1000, 10000, 100000, 1000000, 10000000,
				100000000, 1000000000 };
volatile uint8_t stepIndex = 2;
char frequencyString[12];		//frekvencija zapisana kao znakovni niz
char oldFrequencyString[13];
uint8_t frequencyBuffer[4];//spremnik u kojem se nalazi frekvencija u obliku pogodnom za prijenos do programski definiranog radija
//varijable za tipkovnicu
uint8_t znamenke[9] = { 0 };
uint8_t nextNum = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI3_Init(void);
static void MX_USART2_UART_Init(void);
void MX_USB_HOST_Process(void);

/* USER CODE BEGIN PFP */
static void incrementFreq(void);
static void decrementFreq(void);
static void incrementStep(void);
static void decrementStep(void);
static void toggle_kHz_MHz(void);
static void formatOutput(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void LCD_init()
{
	ILI9341_Unselect();
	ILI9341_TouchUnselect();
	ILI9341_Init();
}

void printStepPointer(void)
{
	int numberOfCharactersToShift = 1;

	if (MHzInUse) {
		if (stepIndex >= 5)
			numberOfCharactersToShift += (stepIndex + 1);
		else
			numberOfCharactersToShift += stepIndex;
	}
	else {
		if (stepIndex >= 2)
			numberOfCharactersToShift += (stepIndex + 1);
		else
			numberOfCharactersToShift += stepIndex;
	}

	ILI9341_FillRectangle(80, 50 + 26, 16 * 11, 26, ILI9341_BLACK);
	ILI9341_FillRectangle(80 + 10 * 16 - numberOfCharactersToShift * 16, 76,
				16, 7, ILI9341_RED);
}

void initStartUpScreen(void)
{
	printButtons();
	printStepPointer();

	//tipka za toggle tipkovnice
	ILI9341_FillRectangle(16, 35, 60, 45, ILI9341_RED);
	ILI9341_FillRectangle(16 + 3, 35 + 3, 60 - 3*2, 45 - 3*2, ILI9341_YELLOW);
	ILI9341_WriteString(16 + 3 + 5, 50, "Keys", Font_11x18, ILI9341_BLACK, ILI9341_YELLOW);


	//pocetna frekvencija
	formatOutput();
	ILI9341_WriteString(80, 50, frequencyString, Font_16x26, ILI9341_YELLOW,
				ILI9341_BLACK);
	ILI9341_WriteString(65 + 16 * 13, 55, (MHzInUse == 1 ? "MHz" : "kHz"),
				Font_11x18, ILI9341_YELLOW, ILI9341_BLACK);
}

void printWithoutStutter(void)
{
	for (int i = 10; i >= 0; i--) {
		if (oldFrequencyString[i] == frequencyString[i])
			continue;
		ILI9341_FillRectangle(80 + i * 16, 50, 16, 26, ILI9341_BLACK);
		ILI9341_WriteString(80 + i * 16, 50,
					charToString(frequencyString[i]),
					Font_16x26, ILI9341_YELLOW,
					ILI9341_BLACK);
	}
}

static void Set_SPI_Speed(uint32_t speed)
{
	if (HAL_SPI_DeInit(&hspi3) != HAL_OK) {
		printf("HAL_SPI_DeInit failed.");
		return;
	}

	hspi3.Init.BaudRatePrescaler = speed;

	if (HAL_SPI_Init(&hspi3) != HAL_OK) {
		printf("HAL_SPI_Init failed.");
		return;
	}
}

static void calcKeyboardFreq(void)
{
	frequency = 0;
	for (int i = 0; i < nextNum; i++) {
		frequency = frequency + znamenke[i] * steps[nextNum - 1 - i];
	}
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
	MX_I2C1_Init();
	MX_USB_HOST_Init();
	MX_SPI3_Init();
	MX_USART2_UART_Init();
	/* USER CODE BEGIN 2 */
	LCD_init();

//	printf("Pozdrav svima!\r\n");

	ILI9341_FillScreen(ILI9341_BLACK);
	initStartUpScreen();
	calculateOutputFrequency(&frequency, &frequencyBuffer);
	sendSerialFreq(frequencyBuffer);

	uint16_t x, y;

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		if (ILI9341_TouchGetCoordinates(&x, &y)) {
			printf("x: %d, y: %d \r\n", x, y);
			HAL_Delay(150);

			if (y >= 80 && y <= (80 + 16*13) &&
			    x >= 50 && x <= (50 + ILI9341_HEIGHT - 16*13)) {
				toggle_kHz_MHz();
				continue;
			} else if (x >= 10 && x <= (15 + 70) &&
			    y >= 35 && y <= (32 + 45)) {

				Set_SPI_Speed(SPI_BAUDRATEPRESCALER_2);

				if (keyboardInUse) {
					printButtons();
					nextNum = 0;
				} else {
					printKeyboard();
				}

				Set_SPI_Speed(SPI_BAUDRATEPRESCALER_8);
				keyboardInUse = 1 - keyboardInUse;
				continue;
			} else if (keyboardInUse == 0) {
				//tipkovnica se ne koristi
				char c = resolveButtonPress(x, y);
				switch (c) {
					case 'S':
						incrementStep();
						break;
					case 's':
						decrementStep();
						break;
					case '+':
						incrementFreq();
						break;
					case '-':
						decrementFreq();
						break;
					default:
						break;
				}
			} else {
				//TODO: tipkovnica se koristi ovde !!!!!
				uint8_t c = resolveKeyboardPress(x, y);
				switch (c) {
				case 0:
				case 1:
				case 2:
				case 3:
				case 4:
				case 5:
				case 6:
				case 7:
				case 8:
				case 9:
					if (nextNum > 8)
						break;
					znamenke[nextNum++] = c;
					calcKeyboardFreq();
					if (frequency > 4000000000) {
						znamenke[nextNum--] = 0;
						calcKeyboardFreq();
						break;
					}

					refreshScreen = 1;
					break;
				case AC:
					for (int i = 0; i < 10; i++)
						znamenke[i] = 0;
					nextNum = 0;
					frequency = 0;
					refreshScreen = 1;
					break;
				case X:
					if (nextNum > 0) {
						znamenke[nextNum--] = 0;
						calcKeyboardFreq();
						refreshScreen = 1;
					}

					break;

				default:
					break;
				}
			}

		}

		if (refreshScreen) {
			refreshScreen = 0;
			calculateOutputFrequency(&frequency, &frequencyBuffer);
			sendSerialFreq(frequencyBuffer);
			formatOutput();
			printWithoutStutter();
		}

		/* USER CODE END WHILE */
		MX_USB_HOST_Process();

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
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Configure the main internal regulator output voltage
	 */
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 336;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 7;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5)
			!= HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void)
{

	/* USER CODE BEGIN I2C1_Init 0 */

	/* USER CODE END I2C1_Init 0 */

	/* USER CODE BEGIN I2C1_Init 1 */

	/* USER CODE END I2C1_Init 1 */
	hi2c1.Instance = I2C1;
	hi2c1.Init.ClockSpeed = 100000;
	hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
	hi2c1.Init.OwnAddress1 = 0;
	hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c1.Init.OwnAddress2 = 0;
	hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN I2C1_Init 2 */

	/* USER CODE END I2C1_Init 2 */

}

/**
 * @brief SPI3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI3_Init(void)
{

	/* USER CODE BEGIN SPI3_Init 0 */

	/* USER CODE END SPI3_Init 0 */

	/* USER CODE BEGIN SPI3_Init 1 */

	/* USER CODE END SPI3_Init 1 */
	/* SPI3 parameter configuration*/
	hspi3.Instance = SPI3;
	hspi3.Init.Mode = SPI_MODE_MASTER;
	hspi3.Init.Direction = SPI_DIRECTION_2LINES;
	hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi3.Init.NSS = SPI_NSS_SOFT;
	hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
	hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi3.Init.CRCPolynomial = 10;
	if (HAL_SPI_Init(&hspi3) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN SPI3_Init 2 */

	/* USER CODE END SPI3_Init 2 */

}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void)
{

	/* USER CODE BEGIN USART2_Init 0 */

	/* USER CODE END USART2_Init 0 */

	/* USER CODE BEGIN USART2_Init 1 */

	/* USER CODE END USART2_Init 1 */
	huart2.Instance = USART2;
	huart2.Init.BaudRate = 9600;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart2) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN USART2_Init 2 */

	/* USER CODE END USART2_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(CS_I2C_SPI_GPIO_Port, CS_I2C_SPI_Pin, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(OTG_FS_PowerSwitchOn_GPIO_Port,
	OTG_FS_PowerSwitchOn_Pin,
				GPIO_PIN_SET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOD, LD4_Pin | LD3_Pin | LD5_Pin | LD6_Pin,
				GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(
			GPIOC,
			GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9,
			GPIO_PIN_RESET);

	/*Configure GPIO pin : CS_I2C_SPI_Pin */
	GPIO_InitStruct.Pin = CS_I2C_SPI_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(CS_I2C_SPI_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : PC13 */
	GPIO_InitStruct.Pin = GPIO_PIN_13;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/*Configure GPIO pins : OTG_FS_PowerSwitchOn_Pin PC6 PC7 PC8
	 PC9 */
	GPIO_InitStruct.Pin = OTG_FS_PowerSwitchOn_Pin | GPIO_PIN_6 | GPIO_PIN_7
			| GPIO_PIN_8 | GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/*Configure GPIO pin : PDM_OUT_Pin */
	GPIO_InitStruct.Pin = PDM_OUT_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
	HAL_GPIO_Init(PDM_OUT_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : B1_Pin */
	GPIO_InitStruct.Pin = B1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : I2S3_WS_Pin */
	GPIO_InitStruct.Pin = I2S3_WS_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
	HAL_GPIO_Init(I2S3_WS_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : SPI1_SCK_Pin SPI1_MISO_Pin SPI1_MOSI_Pin */
	GPIO_InitStruct.Pin = SPI1_SCK_Pin | SPI1_MISO_Pin | SPI1_MOSI_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pin : BOOT1_Pin */
	GPIO_InitStruct.Pin = BOOT1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(BOOT1_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : CLK_IN_Pin */
	GPIO_InitStruct.Pin = CLK_IN_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
	HAL_GPIO_Init(CLK_IN_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pins : LD4_Pin LD3_Pin LD5_Pin LD6_Pin */
	GPIO_InitStruct.Pin = LD4_Pin | LD3_Pin | LD5_Pin | LD6_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	/*Configure GPIO pin : MEMS_INT2_Pin */
	GPIO_InitStruct.Pin = MEMS_INT2_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(MEMS_INT2_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
static void incrementFreq(void)
{
	if (frequency + steps[stepIndex] < frequency)
		return;
	if (frequency + steps[stepIndex] <= 4000000000) {
		frequency = frequency + steps[stepIndex];
		refreshScreen = 1;
	} else
		return;
}

static void decrementFreq(void)
{
	if (frequency - steps[stepIndex] > frequency)
		return;
	if (frequency - steps[stepIndex] >= 10) {
		frequency = frequency - steps[stepIndex];
		refreshScreen = 1;
	} else
		return;
}
/**
 * Funkcija koja povećava vrijednost koraka za koji se frekvencija smanjuje/povećava
 */
static void incrementStep(void)
{
	if (stepIndex == MAX_STEP)
		return;
	stepIndex++;

	printStepPointer();
}

/**
 * Funkcija koja smanjuje vrijednost koraka za koji se frekvencija smanjuje/povećava
 */
static void decrementStep(void)
{
	if (stepIndex == 0)
		return;

	stepIndex--;
	printStepPointer();
}

static void toggle_kHz_MHz(void)
{
	MHzInUse = 1 - MHzInUse;

	printStepPointer();
	ILI9341_FillRectangle(65 + 16 * 13, 55, 11, 18, ILI9341_BLACK);
	ILI9341_WriteString(65 + 16 * 13, 55, (MHzInUse == 1 ? "M" : "k"),
				Font_11x18, ILI9341_YELLOW, ILI9341_BLACK);

	refreshScreen = 1;
}

PUTCHAR_PROTOTYPE
{
	HAL_UART_Transmit(&huart2, (uint8_t*) &ch, 1, 0xFFFF);
	return ch;
}

static void decimalPointShifter(uint8_t numOfDecimalSpaces)
{
	char tmp[12];
	sprintf(tmp, "%lu", frequency);
	int length = strlen(tmp);

	frequencyString[11] = '\0';
	int k = length - 1;

	for (int i = 10; i >= 0; i--) {
		if (i == 10 - numOfDecimalSpaces)
			frequencyString[i] = '.';
		else if (k < 0)
			frequencyString[i] = '0';
		else
			frequencyString[i] = tmp[k--];
	}

	int decimalPointIndex = 10 - numOfDecimalSpaces;

	//uklanjanje dvostrukih nula ispred decimalne točke u ispisu
	for (int i = 0; i <= decimalPointIndex; i++) {
		if (frequencyString[i] == '0' && frequencyString[i + 1] == '0')
			frequencyString[i] = ' ';
		else if (frequencyString[i] == '0'
				&& frequencyString[i + 1] != '.') {
			frequencyString[i] = ' ';
			break;
		}
		else
			break;
	}
}

static void formatOutput(void)
{
	for (int i = 10; i >= 0; i--)
		oldFrequencyString[i] = frequencyString[i];

	if (MHzInUse)
		decimalPointShifter(6);		//6 decimalnih mjesta
	else
		decimalPointShifter(3);		//3 decimalna mjesta
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
	__disable_irq();
	while (1) {
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
      printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
