/*
 * util.c
 *
 *  Created on: Dec 13, 2023
 *      Author: fico
 */
#include "util.h"
#include "stm32f4xx_hal.h"
#include "ili9341.h"
#include "fonts.h"
#include <math.h>
#include <string.h>

#define FS 50000000			//fs za formulu koja pretvara odabranu frekvenciju u oblic pogodan za prijenos USART-om na SDR
#define N 32

#define KEY_H 37
#define KEY_L 73

static int buttonNum = 4;	//broj tipki koje nisu dio tipkovnice
location buttons[] = {
	{15, 106, 150, 60, 'S'},
	{ILI9341_WIDTH  - 150, 106, 150, 60, '+'},
	{15, 170, 150, 60, 's'},
	{ILI9341_WIDTH  - 150, 170, 150, 60, '-'},
};

static int keysNum = 12;
location keys[] = {
		{15, 106, KEY_L, KEY_H, 1},
		{15, 106 + 41, KEY_L, KEY_H, 4},
		{15, 106 + 82, KEY_L, KEY_H, 7},

		{15 + 77, 106, KEY_L, KEY_H, 2},
		{15 + 77, 106 + 41, KEY_L, KEY_H, 5},
		{15 + 77, 106 + 82, KEY_L, KEY_H, 8},

		{15 + 154, 106, KEY_L, KEY_H, 3}, // (169, 240), (106, 143)
		{15 + 154, 106 + 41, KEY_L, KEY_H, 6},
		{15 + 154, 106 + 82, KEY_L, KEY_H, 9},

		{15 + 231, 106, KEY_L, KEY_H, 10},
		{15 + 231, 106 + 41, KEY_L, KEY_H, 11},
		{15 + 231, 106 + 82, KEY_L, KEY_H, 0}
};

static char *keyboard[3][4] = {{"1", "2", "3", "X"},
			     {"4", "5", "6", "AC"},
			     {"7", "8", "9", "0"}};

void calculateOutputFrequency(volatile uint32_t *frequency,
			      uint8_t *frequencyBuffer[])
{
	uint32_t delta = floor(*frequency * pow(2,N) / FS + 0.5);

	for(int i = 0; i < 4; i++)
		(*frequencyBuffer)[i] = (delta >> (i * 8));
}

void sendchar_USART2(uint8_t c)
{
	while (!(USART2->SR & 0x0080));
		USART2->DR = (c & 0xFF);
}

void sendSerialFreq(uint8_t *buffer)
{
	for(int i = 0; i < 4; i++)
		sendchar_USART2(buffer[i]);
}

char *charToString(char c)
{
	switch (c) {
		case('0'):
			return "0";
		case('1'):
			return "1";
		case('2'):
			return "2";
		case('3'):
			return "3";
		case('4'):
			return "4";
		case('5'):
			return "5";
		case('6'):
			return "6";
		case('7'):
			return "7";
		case('8'):
			return "8";
		case('9'):
			return "9";
		case('.'):
			return ".";
		default:
			return " ";
	}
}

void printKeyboard(void)
{
	ILI9341_FillRectangle(16, 106 + 37, 4*73 + 3*4, 4, ILI9341_BLACK);
	ILI9341_FillRectangle(16, 106 + 2*37 + 4, 4*73 + 3*4, 4, ILI9341_BLACK);
	ILI9341_FillRectangle(16 + 73, 106, 4, 3*37 + 2*4, ILI9341_BLACK);
	ILI9341_FillRectangle(16 + 3*73 + 2*4, 106, 4, 3*37 + 2*4, ILI9341_BLACK);
	ILI9341_FillRectangle(16, 106 + 3*37 + 2*4, 304, 224 - 106 + 3*37 + 2*4, ILI9341_BLACK);

	for (int i = 0; i < 4; i++)
		for (int j = 0; j < 3; j++) {
			ILI9341_FillRectangle(16 + i*(73 + 4),
					     106 + j*(37 + 4),
					     73, 37, ILI9341_YELLOW);
			if (strcmp(keyboard[j][i], "AC"))
				ILI9341_WriteString(16 + 31 + i*(73 + 4),
					    106 + 10 + j*(37 + 4),
					    keyboard[j][i],
					    Font_11x18, ILI9341_BLACK,
					    ILI9341_YELLOW);
			else
				ILI9341_WriteString(16 + 31 + i*(73 + 4) - 6,
					    106 + 10 + j*(37 + 4),
					    keyboard[j][i],
					    Font_11x18, ILI9341_BLACK,
					    ILI9341_YELLOW);
		}

}

static void printBorders(void)
{
	ILI9341_FillRectangle(16, 106, 150, 3, ILI9341_RED);
	ILI9341_FillRectangle(16, 106 + 57, 150, 3, ILI9341_RED);
	ILI9341_FillRectangle(16, 106 + 3, 3, 54, ILI9341_RED);
	ILI9341_FillRectangle(16 + 147, 106 + 3, 3, 54, ILI9341_RED);

	ILI9341_FillRectangle(ILI9341_WIDTH  - 150, 106, 150, 3, ILI9341_RED);
	ILI9341_FillRectangle(ILI9341_WIDTH  - 150, 106 + 57, 150, 3, ILI9341_RED);
	ILI9341_FillRectangle(ILI9341_WIDTH  - 150, 106 + 3, 3, 54, ILI9341_RED);
	ILI9341_FillRectangle(ILI9341_WIDTH  - 3, 106 + 3, 3, 54, ILI9341_RED);

	ILI9341_FillRectangle(16, 170, 150, 3, ILI9341_RED);
	ILI9341_FillRectangle(16, 170 + 57, 150, 3, ILI9341_RED);
	ILI9341_FillRectangle(16, 170 + 3, 3, 54, ILI9341_RED);
	ILI9341_FillRectangle(16 + 147, 170 + 3, 3, 54, ILI9341_RED);

	ILI9341_FillRectangle(ILI9341_WIDTH  - 150, 170, 150, 3, ILI9341_RED);
	ILI9341_FillRectangle(ILI9341_WIDTH  - 150, 170 + 57, 150, 3, ILI9341_RED);
	ILI9341_FillRectangle(ILI9341_WIDTH  - 150, 170 + 3, 3, 54, ILI9341_RED);
	ILI9341_FillRectangle(ILI9341_WIDTH  - 3, 170 + 3, 3, 54, ILI9341_RED);
}

void printButtons(void)
{
	ILI9341_FillRectangle(16, 106 + 60, 304, 4, ILI9341_BLACK);

	printBorders();

	ILI9341_FillRectangle(16 + 3, 106 + 3, 150 - 3*2, 60 - 3*2, ILI9341_YELLOW);
	ILI9341_FillRectangle(ILI9341_WIDTH - 150 + 3, 106 + 3, 150 - 3*2, 60 - 3*2, ILI9341_YELLOW);
	ILI9341_FillRectangle(16 + 3, 170 + 3, 150 - 3*2, 60 - 3*2, ILI9341_YELLOW);
	ILI9341_FillRectangle(ILI9341_WIDTH - 150 + 3, 170 + 3, 150 - 3*2, 60 - 3*2, ILI9341_YELLOW);

	ILI9341_WriteString(15 + 3 + 36, 105 + 3 + 18, "step++", Font_11x18, ILI9341_BLACK, ILI9341_YELLOW);
	ILI9341_WriteString(ILI9341_WIDTH - 85, 105 + 3 + 18, "+", Font_16x26, ILI9341_BLACK, ILI9341_YELLOW);
	ILI9341_WriteString(15 + 3 + 36, 170 + 3 + 18, "step--", Font_11x18, ILI9341_BLACK, ILI9341_YELLOW);
	ILI9341_WriteString(ILI9341_WIDTH - 85, 170 + 3 + 18, "-", Font_16x26, ILI9341_BLACK, ILI9341_YELLOW);
}

char resolveButtonPress(uint16_t x, uint16_t y)
{
	for (int i = 0; i < buttonNum; i++) {
		if (x >= buttons[i].x && x <= (buttons[i].x + buttons[i].x_offset)
		    && y >= buttons[i].y && y <= (buttons[i].y + buttons[i].y_offset))
			return buttons[i].command;
	}

	return 'N';
}

uint8_t resolveKeyboardPress(uint16_t x,uint16_t y)
{
	for (int i = 0; i < keysNum; i++) {
		if (x >= keys[i].x && x <= (keys[i].x + keys[i].x_offset)
		    && y >= keys[i].y && y <= (keys[i].y + keys[i].y_offset))
			return keys[i].command;
	}

	return 99;
}
