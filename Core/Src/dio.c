/*
 * dio.c
 *
 *  Created on: Jun 10, 2021
 *      Author: LBekel
 */

#include "main.h"
#include "dio.h"
#include "mqtt_client.h"
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

/* Variables for ADC conversion data */
__IO   uint16_t   uhADCxConvertedData = VAR_CONVERTED_DATA_INIT_VALUE; /* ADC group regular conversion data */
/* Variables for ADC conversion data computation to physical values */
uint16_t   uhADCxConvertedData_Voltage_mVolt = 0;  /* Value of voltage calculated from ADC conversion data (unit: mV) */


void checkBlindPosition(uint8_t channel);
void transferDoubleswitch2Blind(uint8_t channel);
void publishCentralDoubleswitchTopic(void);

void initBlinds()
{
	for (int var = 0; var < 8; ++var)
	{
		blinds[var].channel = var+1;
		blinds[var].downRelay_Pin = Relay_Pins[var*2];
		blinds[var].downRelay_Port = Relay_Ports[var*2];
		blinds[var].upRelay_Pin = Relay_Pins[var*2+1];
		blinds[var].upRelay_Port = Relay_Ports[var*2+1];;
		blinds[var].blinddirection = blinddirection_off;
		blinds[var].position_actual = 0;
		blinds[var].position_changed = true;
		//blinds[var].movingtime = 10000;
	}
}

void setBlindsMovingTime(uint16_t * blindsmovingtime)
{
	for (int var = 0; var < 8; ++var)
	{
		blinds[var].movingtime = (uint32_t)blindsmovingtime[var];
	}
}

void setBlindDirection(struct blind_s *blind)
{
	switch (blind->blinddirection) {
		case blinddirection_up:
			HAL_GPIO_WritePin(blind->downRelay_Port, blind->downRelay_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(blind->upRelay_Port, blind->upRelay_Pin, GPIO_PIN_SET);
			blind->position_target = blind->movingtime; //to the end
			blind->starttime = xTaskGetTickCount();
			break;
		case blinddirection_down:
			HAL_GPIO_WritePin(blind->downRelay_Port, blind->downRelay_Pin, GPIO_PIN_SET);
			HAL_GPIO_WritePin(blind->upRelay_Port, blind->upRelay_Pin, GPIO_PIN_SET);
			blind->position_target = 0; //to the end
			blind->starttime = xTaskGetTickCount();
			break;
		case blinddirection_off:
			HAL_GPIO_WritePin(blind->downRelay_Port, blind->downRelay_Pin, GPIO_PIN_RESET);
			HAL_GPIO_WritePin(blind->upRelay_Port, blind->upRelay_Pin, GPIO_PIN_RESET);
			blind->starttime = 0;
			break;
		default:
			break;
	}
}

void initDoubleswitches(void)
{
	for (uint8_t var = 0; var < num_doubleswitches; ++var)
	{
		doubleswitches[var].channel = var+1;
		doubleswitches[var].downInput_Pin = Input_Pins[var*2];
		doubleswitches[var].downInput_Port = Input_Ports[var*2];
		doubleswitches[var].upInput_Pin = Input_Pins[var*2+1];
		doubleswitches[var].upInput_Port = Input_Ports[var*2+1];
		doubleswitches[var].inputdirection = blinddirection_off;
		doubleswitches[var].changed = false;
	}
}

void readDoubleswitches(void)
{
    for(uint8_t var = 0; var < num_doubleswitches; ++var)
    {
    	readDoubleswitch(&doubleswitches[var]);
    }
}

void readDoubleswitch(struct doubleswitch_s *doubleswitch)
{
	if (HAL_GPIO_ReadPin(doubleswitch->upInput_Port, doubleswitch->upInput_Pin) == GPIO_PIN_SET)
	{
		if (doubleswitch->inputdirection != inputdirection_up && doubleswitch->inputdirection != inputdirection_up_end)//if state changed
		{
			doubleswitch->upInput_starttime = xTaskGetTickCount(); //store current time
			doubleswitch->inputdirection = inputdirection_up;
			publish_doubleswitch_state(doubleswitch);
			doubleswitch->changed = true;
		}
		else //button is still pressed
		{
			TickType_t timeelapsed;
			timeelapsed = xTaskGetTickCount() - doubleswitch->upInput_starttime;
			if(timeelapsed > 3000)//button pressed 3sec
			{
				doubleswitch->inputdirection = inputdirection_up_end;
				doubleswitch->changed = true;
			}
		}
	}
	else if (HAL_GPIO_ReadPin(doubleswitch->downInput_Port, doubleswitch->downInput_Pin) == GPIO_PIN_SET)
	{

		if (doubleswitch->inputdirection != inputdirection_down && doubleswitch->inputdirection != inputdirection_down_end)//if state changed
		{
			doubleswitch->downInput_starttime = xTaskGetTickCount(); //store current time
			doubleswitch->inputdirection = inputdirection_down;
			publish_doubleswitch_state(doubleswitch);
			doubleswitch->changed = true;
		}
		else //button is still pressed
		{
			TickType_t timeelapsed;
			timeelapsed = xTaskGetTickCount() - doubleswitch->downInput_starttime;
			if(timeelapsed > 3000)//button pressed 3sec
			{
				doubleswitch->inputdirection = inputdirection_down_end;
				doubleswitch->changed = true;
			}
		}
	}
	else
	{
		if (doubleswitch->inputdirection != inputdirection_off)//if state changed
		{
			switch (doubleswitch->inputdirection)
			{
				case inputdirection_up_end:
					break;
				case inputdirection_down_end:
					break;
				case inputdirection_up:
					doubleswitch->inputdirection = inputdirection_off;
					doubleswitch->changed = true;
					break;
				case inputdirection_down:
					doubleswitch->inputdirection = inputdirection_off;
					doubleswitch->changed = true;
					break;
				default:
					break;
			}
			publish_doubleswitch_state(doubleswitch);
		}
	}
}
void StartScanInputTask(void *argument) {
	/* Infinite loop */
	/* Start ADC group regular conversion */
	if (HAL_ADC_Start((ADC_HandleTypeDef*) argument) != HAL_OK) {
		/* ADC conversion start error */
		Error_Handler();
	}
	for (;;) {

		readDoubleswitches();
		for (int var = 0; var < num_doubleswitches - 1; ++var)
		{
			transferDoubleswitch2Blind(var);
			checkBlindPosition(var);
			publishCentralDoubleswitchTopic();
		}
		osDelay(10);
		/* Wait till conversion is done */
		if (HAL_ADC_PollForConversion((ADC_HandleTypeDef*)argument, 10) != HAL_OK)
		{
		  /* End Of Conversion flag not set on time */
		  Error_Handler();
		}
		else
		{
		  /* Retrieve ADC conversion data */
		  uhADCxConvertedData = HAL_ADC_GetValue((ADC_HandleTypeDef*)argument);

		  /* Computation of ADC conversions raw data to physical values           */
		  /* using helper macro.                                                  */
		  uhADCxConvertedData_Voltage_mVolt = __ADC_CALC_DATA_VOLTAGE(VDDA_APPLI, uhADCxConvertedData)/1;
		}
	}
}

void checkBlindPosition(uint8_t channel)
{
	switch (blinds[channel].blinddirection) {
	case blinddirection_up:
		blinds[channel].position_actual += xTaskGetTickCount()
				- blinds[channel].starttime; //timeposition in ms;
		blinds[channel].starttime = xTaskGetTickCount();
		blinds[channel].position_changed = true;
		if (blinds[channel].position_actual
				>= blinds[channel].position_target) {
			blinds[channel].position_actual = blinds[channel].position_target;
			blinds[channel].blinddirection = blinddirection_off;
			publish_blind_state(&blinds[channel]);
			publish_blind_cmd(&blinds[channel]);
			doubleswitches[channel].inputdirection = inputdirection_off;
			doubleswitches[channel].changed = true;
			publish_doubleswitch_state(&doubleswitches[channel]);
		}
		break;
	case blinddirection_down:
		blinds[channel].position_actual -= xTaskGetTickCount()
				- blinds[channel].starttime; //timeposition in ms
		blinds[channel].starttime = xTaskGetTickCount();
		blinds[channel].position_changed = true;
		if (blinds[channel].position_actual
				<= blinds[channel].position_target) {
			blinds[channel].position_actual = blinds[channel].position_target;
			blinds[channel].blinddirection = blinddirection_off;
			publish_blind_state(&blinds[channel]);
			publish_blind_cmd(&blinds[channel]);
			doubleswitches[channel].inputdirection = inputdirection_off;
			doubleswitches[channel].changed = true;
			publish_doubleswitch_state(&doubleswitches[channel]);
		}
		break;
	default:
		break;
	}
}

void transferDoubleswitch2Blind(uint8_t channel)
{
	if (doubleswitches[channel].changed == true)
	{
		doubleswitches[channel].changed = false;
		switch (doubleswitches[channel].inputdirection)
		{
		case inputdirection_up_end:
			break;
		case inputdirection_down_end:
			break;
		case inputdirection_up:
			if (blinds[channel].blinddirection != blinddirection_up) //if not set do it now
					{
				if (blinds[channel].position_actual
						< blinds[channel].movingtime) //check if we are on the top position

				{
					blinds[channel].blinddirection = blinddirection_up;
					setBlindDirection(&blinds[channel]);
					publish_blind_state(&blinds[channel]);
					publish_blind_cmd(&blinds[channel]);
				} else
				{
					blinds[channel].blinddirection = blinddirection_off;
					setBlindDirection(&blinds[channel]);
					publish_blind_state(&blinds[channel]);
					publish_blind_cmd(&blinds[channel]);
				}
			}
			break;
		case inputdirection_down:
			if (blinds[channel].blinddirection != blinddirection_down) //if not set do it now
					{
				if (blinds[channel].position_actual > 0) //check if we are on the bottom position
						{
					blinds[channel].blinddirection = blinddirection_down;
					setBlindDirection(&blinds[channel]);
					publish_blind_state(&blinds[channel]);
					publish_blind_cmd(&blinds[channel]);
				} else {
					blinds[channel].blinddirection = blinddirection_off;
					setBlindDirection(&blinds[channel]);
					publish_blind_state(&blinds[channel]);
					publish_blind_cmd(&blinds[channel]);
				}
			}
			break;
		default:
			if (blinds[channel].blinddirection != blinddirection_off) {
				blinds[channel].blinddirection = blinddirection_off;
				setBlindDirection(&blinds[channel]);
				publish_blind_state(&blinds[channel]);
				publish_blind_cmd(&blinds[channel]);
			}
			break;
		}
	}
}
void publishCentralDoubleswitchTopic(void)
{
	//if central button is pressed this function sends a mqtt cmd for all channel
	//Todo: make the mqtt topic dynamic via web interface
	if (doubleswitches[8].changed == true) {
		doubleswitches[8].changed = false;
		switch (doubleswitches[8].inputdirection) {
		case inputdirection_up_end:
			break;
		case inputdirection_down_end:
			break;
		case inputdirection_up:
			for (int var = 0; var < num_doubleswitches - 1; ++var) {
				struct blind_s tempblind;
				tempblind.channel = var + 1;
				tempblind.blinddirection = blinddirection_up;
				publish_blind_cmd(&tempblind);
			}
			break;
		case inputdirection_down:
			for (int var = 0; var < num_doubleswitches - 1; ++var) {
				struct blind_s tempblind;
				tempblind.channel = var + 1;
				tempblind.blinddirection = blinddirection_down;
				publish_blind_cmd(&tempblind);
			}
			break;
		default:
			for (int var = 0; var < num_doubleswitches - 1; ++var) {
				struct blind_s tempblind;
				tempblind.channel = var + 1;
				tempblind.blinddirection = blinddirection_off;
				publish_blind_cmd(&tempblind);
			}
			break;
		}
	}
}
