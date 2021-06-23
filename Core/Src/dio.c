/*
 * dio.c
 *
 *  Created on: Jun 10, 2021
 *      Author: LBekel
 */

#include "main.h"
#include "dio.h"
#include <stdbool.h>

uint16_t Relay_Pins[] = {
		OUT01DOWN_Pin, OUT01UP_Pin,
		OUT02DOWN_Pin, OUT02UP_Pin,
		OUT03DOWN_Pin, OUT03UP_Pin,
		OUT04DOWN_Pin, OUT04UP_Pin,
		OUT05DOWN_Pin, OUT05UP_Pin,
		OUT06DOWN_Pin, OUT06UP_Pin,
		OUT07DOWN_Pin, OUT07UP_Pin,
		OUT08DOWN_Pin, OUT08UP_Pin };

GPIO_TypeDef * Relay_Ports[] = {
		OUT01DOWN_GPIO_Port, OUT01UP_GPIO_Port,
		OUT02DOWN_GPIO_Port, OUT02UP_GPIO_Port,
		OUT03DOWN_GPIO_Port, OUT03UP_GPIO_Port,
		OUT04DOWN_GPIO_Port, OUT04UP_GPIO_Port,
		OUT05DOWN_GPIO_Port, OUT05UP_GPIO_Port,
		OUT06DOWN_GPIO_Port, OUT06UP_GPIO_Port,
		OUT07DOWN_GPIO_Port, OUT07UP_GPIO_Port,
		OUT08DOWN_GPIO_Port, OUT08UP_GPIO_Port };


uint16_t Input_Pins[] = {
		IN01_Pin, IN02_Pin, IN03_Pin, IN04_Pin, IN05_Pin,
		IN06_Pin, IN07_Pin, IN08_Pin, IN09_Pin, IN10_Pin,
		IN11_Pin, IN12_Pin,	IN13_Pin, IN14_Pin, IN15_Pin,
		IN16_Pin, IN17_Pin, IN18_Pin };

GPIO_TypeDef *Input_Ports[] = {
		IN01_GPIO_Port, IN02_GPIO_Port, IN03_GPIO_Port,	IN04_GPIO_Port,
		IN05_GPIO_Port, IN06_GPIO_Port, IN07_GPIO_Port,	IN08_GPIO_Port,
		IN09_GPIO_Port, IN10_GPIO_Port, IN11_GPIO_Port,	IN12_GPIO_Port,
		IN13_GPIO_Port, IN14_GPIO_Port, IN15_GPIO_Port, IN16_GPIO_Port,
		IN17_GPIO_Port, IN18_GPIO_Port };

bool RelayStates[num_relay_ch] = {false};
bool InputStates[num_input_ch] = {false};

void initBlinds()
{
	for (int var = 0; var < 8; ++var)
	{
		blinds[var].downRelay_Pin = Relay_Pins[var*2];
		blinds[var].downRelay_Port = Relay_Ports[var*2];
		blinds[var].upRelay_Pin = Relay_Pins[var*2+1];
		blinds[var].upRelay_Port = Relay_Ports[var*2+1];;
		blinds[var].blinddirection = blindsdirection_off;
		blinds[var].blindposition = 0;
	}
}


void setBlindsDirection(struct blind_s *blind)
{
	switch (blind->blinddirection) {
		case blindsdirection_up:
			HAL_GPIO_WritePin(blind->downRelay_Port, blind->downRelay_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(blind->upRelay_Port, blind->upRelay_Pin, GPIO_PIN_SET);
			break;
		case blindsdirection_down:
			HAL_GPIO_WritePin(blind->downRelay_Port, blind->downRelay_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(blind->upRelay_Port, blind->upRelay_Pin, GPIO_PIN_SET);
			break;
		case blindsdirection_off:
			HAL_GPIO_WritePin(blind->downRelay_Port, blind->downRelay_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(blind->upRelay_Port, blind->upRelay_Pin, GPIO_PIN_RESET);
			break;
		default:
			break;
	}
}
