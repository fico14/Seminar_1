/*
 * util.h
 *
 *  Created on: Dec 13, 2023
 *      Author: fico
 */

#ifndef INC_UTIL_H_
#define INC_UTIL_H_

#include <stdint.h>

#define AC 11
#define X 10

typedef struct location {
	int y;
	int x;
	int y_offset;
	int x_offset;
	char command;
} location;


void sendchar_USART2(uint8_t c);
void sendSerialFreq(uint8_t *buffer);
char *charToString(char c);
void printButtons();
void calculateOutputFrequency(volatile uint32_t *frequency,
			      uint8_t *frequencyBuffer[]);
void printKeyboard(void);
char resolveButtonPress(uint16_t x, uint16_t y);

#endif /* INC_UTIL_H_ */
