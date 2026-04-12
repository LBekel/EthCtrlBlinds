/*
 * dio.c
 *
 *  Created on: Jun 10, 2021
 *      Author: LBekel
 */

#include "main.h"
#include "dio.h"
#include "MqttClient.h"
#include <stdbool.h>
#include "math.h"

struct blind_s blinds[num_blinds];
struct doubleswitch_s doubleswitches[num_doubleswitches];


uint16_t Relay_Pins[num_relay_ch] = {
OUT01DOWN_Pin, OUT01UP_Pin,
OUT02DOWN_Pin, OUT02UP_Pin,
OUT03DOWN_Pin, OUT03UP_Pin,
OUT04DOWN_Pin, OUT04UP_Pin,
OUT05DOWN_Pin, OUT05UP_Pin,
OUT06DOWN_Pin, OUT06UP_Pin,
OUT07DOWN_Pin, OUT07UP_Pin,
OUT08DOWN_Pin, OUT08UP_Pin};

GPIO_TypeDef *Relay_Ports[num_relay_ch] = {
OUT01DOWN_GPIO_Port, OUT01UP_GPIO_Port,
OUT02DOWN_GPIO_Port, OUT02UP_GPIO_Port,
OUT03DOWN_GPIO_Port, OUT03UP_GPIO_Port,
OUT04DOWN_GPIO_Port, OUT04UP_GPIO_Port,
OUT05DOWN_GPIO_Port, OUT05UP_GPIO_Port,
OUT06DOWN_GPIO_Port, OUT06UP_GPIO_Port,
OUT07DOWN_GPIO_Port, OUT07UP_GPIO_Port,
OUT08DOWN_GPIO_Port, OUT08UP_GPIO_Port};

uint16_t Input_Pins[num_input_ch] = {
IN01_Pin, IN02_Pin, IN03_Pin, IN04_Pin, IN05_Pin,
IN06_Pin, IN07_Pin, IN08_Pin, IN09_Pin, IN10_Pin,
IN11_Pin, IN12_Pin, IN13_Pin, IN14_Pin, IN15_Pin,
IN16_Pin, IN17_Pin, IN18_Pin};

GPIO_TypeDef *Input_Ports[num_input_ch] = {
IN01_GPIO_Port, IN02_GPIO_Port, IN03_GPIO_Port, IN04_GPIO_Port,
IN05_GPIO_Port, IN06_GPIO_Port, IN07_GPIO_Port, IN08_GPIO_Port,
IN09_GPIO_Port, IN10_GPIO_Port, IN11_GPIO_Port, IN12_GPIO_Port,
IN13_GPIO_Port, IN14_GPIO_Port, IN15_GPIO_Port, IN16_GPIO_Port,
IN17_GPIO_Port, IN18_GPIO_Port};


#define MAXMOVINGTIME 120000//ms
#define MINLEARNDELAY 1000//ms
#define ENDMOVEDELAY 1000 //ms
#define RAFFSTOREDELAY 3000 //ms
#define DEBOUNCE_CYCLE (5.0f)
#define BLINDCURRENT_BUF_SIZE 10
#define MOTORSTARTDELAY 500

int16_t blindcurrent_threshold = 200;//mA

/* Definition of ADCx conversions data table size */
#define ADC_CONVERTED_DATA_BUFFER_SIZE   ((uint32_t)  545)   /* Size of array aADCxConvertedData[] */
//to fit one 50Hz periode into buffer
//495Cycles/108Mhz*8=36.66us
//20ms/36.66us = 545samples
uint16_t adc_offset = 0; //counts

/* Variable containing ADC conversions data */
ALIGN_32BYTES(static uint16_t aADCxConvertedData[ADC_CONVERTED_DATA_BUFFER_SIZE]);

void dioCheckBlindPosition(uint8_t channel);
void dioTransferDoubleswitch2Blind(uint8_t inputchannel);
void dioPublishCentralDoubleswitchTopic(void);
GPIO_PinState dioGpioReadUpDebounced(struct doubleswitch_s *doubleswitch);
GPIO_PinState dioGpioReadDownDebounced(struct doubleswitch_s *doubleswitch);
void dioReadDoubleswitch(struct doubleswitch_s *doubleswitch);
int16_t dioGetCurrentADC(void);
int16_t dioMovingAvg(int16_t blindcurrent);
void dioLearnBlindMovingTime(struct blind_s *blind, int16_t blindcurrent);

void dioReadDoubleswitch(struct doubleswitch_s *doubleswitch)
{
    if(dioGpioReadUpDebounced(doubleswitch) == GPIO_PIN_SET)
    {
        TickType_t timeelapsed;
        timeelapsed = xTaskGetTickCount() - doubleswitch->upInput_starttime;
        if(doubleswitch->inputdirection == inputdirection_up_end)
        {
            //nothing to do?
        }
        else if(doubleswitch->inputdirection == inputdirection_up)
        {
            if(timeelapsed > ENDMOVEDELAY) //long button press
            {
                doubleswitch->inputdirection = inputdirection_up_end;
                doubleswitch->angle_target = 0;
                doubleswitch->changed = true;
            }
        }
        else
        {
            doubleswitch->upInput_starttime = xTaskGetTickCount(); //store current time
            doubleswitch->inputdirection = inputdirection_up;
            MqttClient_PublishDoubleswitchStat(doubleswitch);
            doubleswitch->changed = true;
        }
    }
    else if(dioGpioReadDownDebounced(doubleswitch) == GPIO_PIN_SET)
    {
        TickType_t timeelapsed;
        timeelapsed = xTaskGetTickCount() - doubleswitch->downInput_starttime;
        if(doubleswitch->inputdirection == inputdirection_down)
        {
            if(timeelapsed > ENDMOVEDELAY) //long button press
            {
                doubleswitch->inputdirection = inputdirection_down_end;
                doubleswitch->angle_target = 100;
                doubleswitch->changed = true;
            }
        }
        else if(doubleswitch->inputdirection == inputdirection_down_end)
        {
            if(timeelapsed > RAFFSTOREDELAY) //long button press
            {
                doubleswitch->inputdirection = inputdirection_down_end_angle;
                doubleswitch->angle_target = 30;
                doubleswitch->changed = true;
            }
        }
        else if (doubleswitch->inputdirection == inputdirection_down_end_angle)
        {
            //do nothing?
        }
        else
        {
            doubleswitch->downInput_starttime = xTaskGetTickCount(); //store current time
            doubleswitch->inputdirection = inputdirection_down;
            doubleswitch->angle_target = 100;
            doubleswitch->changed = true;
            MqttClient_PublishDoubleswitchStat(doubleswitch);
        }
    }
    else
    {
        if(doubleswitch->inputdirection != inputdirection_off) //if state changed
        {
            switch(doubleswitch->inputdirection)
            {
                case inputdirection_down_end_angle:
                    doubleswitch->inputdirection = inputdirection_off;
                    break;
                case inputdirection_down_end:
                    doubleswitch->inputdirection = inputdirection_off;
                    break;
                case inputdirection_up_end:
                    doubleswitch->inputdirection = inputdirection_off;
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
            MqttClient_PublishDoubleswitchStat(doubleswitch);
        }
    }
}

void dioCheckBlindPosition(uint8_t channel)
{
	double factor;
	int32_t deltaPosition;
    switch(blinds[channel].blinddirection)
    {
        case blinddirection_angle_up:
            blinds[channel].angle_actual -= xTaskGetTickCount() - blinds[channel].starttime; //timeposition in ms
            blinds[channel].starttime = xTaskGetTickCount();
            blinds[channel].angle_changed = true;
            if(blinds[channel].angle_actual <= blinds[channel].angle_target) //angle reached
            {
                blinds[channel].angle_actual = blinds[channel].angle_target;
                blinds[channel].blinddirection = blinddirection_off;
                Dio_SetBlindDirection(&blinds[channel]);
                MqttClient_PublishBlindDirStat(&blinds[channel]);
                MqttClient_PublishBlindDirCmd(&blinds[channel]);
            }
            break;
        case blinddirection_angle_down:
            blinds[channel].angle_actual += xTaskGetTickCount() - blinds[channel].starttime; //timeposition in ms
            blinds[channel].starttime = xTaskGetTickCount();
            blinds[channel].angle_changed = true;
            if(blinds[channel].angle_actual >= blinds[channel].angle_target) //angle reached
            {
                blinds[channel].angle_actual = blinds[channel].angle_target;
                blinds[channel].blinddirection = blinddirection_off;
                Dio_SetBlindDirection(&blinds[channel]);
                MqttClient_PublishBlindDirStat(&blinds[channel]);
                MqttClient_PublishBlindDirCmd(&blinds[channel]);
            }
            break;
        case blinddirection_up:
        	//moving up is slower

            if(blinds[channel].angle_function_active)
            {
                if(blinds[channel].angle_actual>0) //increment angle if not at end position
                {
                    blinds[channel].angle_actual -= xTaskGetTickCount() - blinds[channel].starttime; //timeposition in ms
                    blinds[channel].angle_changed = true;
                }
                else if(blinds[channel].angle_actual==0)
                {
                    blinds[channel].angle_actual = 0;
                }
                else
                {
                    blinds[channel].angle_actual = 0;
                    blinds[channel].angle_changed = true;
                }
            }

            deltaPosition = xTaskGetTickCount() - blinds[channel].starttime;
            if(deltaPosition > 0)
            {
                blinds[channel].position_actual -= xTaskGetTickCount() - blinds[channel].starttime; //timeposition in ms;
                blinds[channel].starttime = xTaskGetTickCount();
                blinds[channel].position_changed = true;
                if(blinds[channel].position_actual <= blinds[channel].position_target)
                {
                    //Target position reached
                    if(blinds[channel].position_target < 0)
                    {
                        blinds[channel].position_target = 0;
                    }
                    blinds[channel].position_actual = blinds[channel].position_target;

                    if(blinds[channel].angle_function_active)
                    {
                        blinds[channel].blinddirection = blinddirection_angle_down;
                        Dio_SetBlindDirection(&blinds[channel]);
                    }
                    else
                    {
                        blinds[channel].blinddirection = blinddirection_off;
                        Dio_SetBlindDirection(&blinds[channel]);
                        MqttClient_PublishBlindDirStat(&blinds[channel]);
                        MqttClient_PublishBlindDirCmd(&blinds[channel]);
                    }
                }
            }
            break;
        case blinddirection_down:
        	//moving down is faster, so add a factor to the real time
            if(blinds[channel].angle_function_active)
            {
                if(blinds[channel].angle_actual<blinds[channel].angle_movingtime) //increment angle if not at end position
                {
                    blinds[channel].angle_actual += xTaskGetTickCount() - blinds[channel].starttime; //timeposition in ms
                    blinds[channel].angle_changed = true;
                }
                else if(blinds[channel].angle_actual == blinds[channel].angle_movingtime)
                {
                    blinds[channel].angle_actual = blinds[channel].angle_movingtime;
                }
                else
                {
                    blinds[channel].angle_actual = blinds[channel].angle_movingtime;
                    blinds[channel].angle_changed = true;
                }
            }

            deltaPosition = xTaskGetTickCount() - blinds[channel].starttime;
            if(deltaPosition > 0)
            {
                factor = (double) (blinds[channel].position_movingtimeup) / (double) (blinds[channel].position_movingtimedown);
                deltaPosition = round(factor * deltaPosition);
                blinds[channel].position_actual += deltaPosition;

                blinds[channel].starttime = xTaskGetTickCount();
                blinds[channel].position_changed = true;
                if(blinds[channel].position_actual >= blinds[channel].position_target)
                {
                    //Target position reached
                    if(blinds[channel].position_target > blinds[channel].position_movingtimeup)
                    {
                        blinds[channel].position_target = blinds[channel].position_movingtimeup;
                    }
                    blinds[channel].position_actual = blinds[channel].position_target;

                    if(blinds[channel].angle_function_active)
                    {
                        blinds[channel].blinddirection = blinddirection_angle_up;
                        Dio_SetBlindDirection(&blinds[channel]);
                    }
                    else
                    {
                        blinds[channel].blinddirection = blinddirection_off;
                        Dio_SetBlindDirection(&blinds[channel]);
                        MqttClient_PublishBlindDirStat(&blinds[channel]);
                        MqttClient_PublishBlindDirCmd(&blinds[channel]);
                    }
                }
            }

            break;
        default:
            break;
    }
}

void dioTransferDoubleswitch2Blind(uint8_t inputchannel)
{
    if(doubleswitches[inputchannel].changed == true)
    {
        doubleswitches[inputchannel].changed = false;
        for (uint8_t blindchannel = 0; blindchannel < num_blinds; blindchannel++)
        {
            if((blindinputmatrix[blindchannel]>>inputchannel)&1)
            {
                int32_t angletime = (double) blinds[blindchannel].angle_movingtime / (double) 100 * doubleswitches[inputchannel].angle_target;
                switch(doubleswitches[inputchannel].inputdirection)
                {
                    case inputdirection_up_end:
                        break;
                    case inputdirection_down_end:
                        break;
                    case inputdirection_down_end_angle:
                        blinds[blindchannel].angle_target = angletime;
                        break;
                    case inputdirection_up:
                        if(blinds[blindchannel].blinddirection != blinddirection_up) //if not set do it now
                        {
                            blinds[blindchannel].blinddirection = blinddirection_up;
                            blinds[blindchannel].position_target = 0 - 1000;
                            blinds[blindchannel].angle_target = 0;
                            if(blinds[blindchannel].position_function_active == false)
                            {
                                blinds[blindchannel].position_actual = blinds[blindchannel].position_movingtimeup;
                            }
                            Dio_SetBlindDirection(&blinds[blindchannel]);
                            MqttClient_PublishBlindDirStat(&blinds[blindchannel]);
                        }
                        break;
                    case inputdirection_down:
                        if(blinds[blindchannel].blinddirection != blinddirection_down) //if not set do it now
                        {
                            blinds[blindchannel].blinddirection = blinddirection_down;
                            blinds[blindchannel].position_target = blinds[blindchannel].position_movingtimeup + 1000;
                            blinds[blindchannel].angle_target = blinds[blindchannel].angle_movingtime;
                            if(blinds[blindchannel].position_function_active == false)
                            {
                                blinds[blindchannel].position_actual = 0;
                            }
                            Dio_SetBlindDirection(&blinds[blindchannel]);
                            MqttClient_PublishBlindDirStat(&blinds[blindchannel]);
                        }
                        break;
                    default:
                        if(blinds[blindchannel].blinddirection != blinddirection_off)
                        {
                            blinds[blindchannel].blinddirection = blinddirection_off;
                            Dio_SetBlindDirection(&blinds[blindchannel]);
                            MqttClient_PublishBlindDirStat(&blinds[blindchannel]);
                        }
                        break;
                }
            }
        }
    }
}


GPIO_PinState dioGpioReadUpDebounced(struct doubleswitch_s *doubleswitch)
{
    GPIO_PinState pinstate = HAL_GPIO_ReadPin(doubleswitch->upInput_Port, doubleswitch->upInput_Pin);

    // do a moving average of the digital input... result button between 0 and (2 * DEBOUNCE_CYCLE)
    doubleswitch->updebounce = (doubleswitch->updebounce * ((2 * DEBOUNCE_CYCLE)-1) + (float)pinstate * 2 * DEBOUNCE_CYCLE) / (2 * DEBOUNCE_CYCLE);

    if(doubleswitch->updebounce > DEBOUNCE_CYCLE)
    {
        return GPIO_PIN_SET;
    }
    else
    {
        return GPIO_PIN_RESET;
    }

}

GPIO_PinState dioGpioReadDownDebounced(struct doubleswitch_s *doubleswitch)
{
    GPIO_PinState pinstate = HAL_GPIO_ReadPin(doubleswitch->downInput_Port, doubleswitch->downInput_Pin);

    // do a moving average of the digital input... result button between 0 and (2 * DEBOUNCE_CYCLE)
    doubleswitch->downdebounce = (doubleswitch->downdebounce * ((2 * DEBOUNCE_CYCLE)-1) + (float)pinstate * 2 * DEBOUNCE_CYCLE) / (2 * DEBOUNCE_CYCLE);

    if(doubleswitch->downdebounce > DEBOUNCE_CYCLE)
    {
        return GPIO_PIN_SET;
    }
    else
    {
        return GPIO_PIN_RESET;
    }

}

void dioPublishCentralDoubleswitchTopic(void)
{
    //if central button is pressed this function sends a mqtt cmd for all channel
    //Todo: make the mqtt topic dynamic via web interface
    if(doubleswitches[8].changed == true)
    {
        doubleswitches[8].changed = false;
        switch(doubleswitches[8].inputdirection)
        {
            case inputdirection_down_end_angle:
                break;
            case inputdirection_up_end:
                break;
            case inputdirection_down_end:
                break;
            case inputdirection_up:
                for(int var = 0; var < num_doubleswitches - 1; ++var)
                {
                    struct blind_s tempblind;
                    tempblind.channel = var + 1;
                    tempblind.blinddirection = blinddirection_up;
                    MqttClient_PublishBlindDirCmd(&tempblind);
                }
                break;
            case inputdirection_down:
                for(int var = 0; var < num_doubleswitches - 1; ++var)
                {
                    struct blind_s tempblind;
                    tempblind.channel = var + 1;
                    tempblind.blinddirection = blinddirection_down;
                    MqttClient_PublishBlindDirCmd(&tempblind);
                }
                break;
            default:
                for(int var = 0; var < num_doubleswitches - 1; ++var)
                {
                    struct blind_s tempblind;
                    tempblind.channel = var + 1;
                    tempblind.blinddirection = blinddirection_off;
                    MqttClient_PublishBlindDirCmd(&tempblind);
                }
                break;
        }
    }
}

int16_t dioGetCurrentADC(void)
{
    //HAL_ADC_PollForConversion((ADC_HandleTypeDef*)argument, 1);
    /* Retrieve ADC conversion data */
    //uhADCxConvertedData = HAL_ADC_GetValue((ADC_HandleTypeDef*)argument);
    uint32_t avg = 0;
    uint32_t min = aADCxConvertedData[0];
    uint32_t max = aADCxConvertedData[0];
    int16_t value = 0;
    for(int var = 0; var < ADC_CONVERTED_DATA_BUFFER_SIZE; ++var)
    {
        avg += aADCxConvertedData[var];
        if(aADCxConvertedData[var]<min)
        {
            min = aADCxConvertedData[var];
        }
        if(aADCxConvertedData[var]>max)
        {
            max = aADCxConvertedData[var];
        }
    }
    avg /= ADC_CONVERTED_DATA_BUFFER_SIZE;
    /* Computation of ADC conversions raw data to physical values           */
    /* using helper macro.                                                  */
    //uhADCxConvertedData_Voltage_mVolt = __ADC_CALC_DATA_VOLTAGE(VDDA_APPLI, avg) / 1;
    //return (avg * (uint32_t) 50000 / DIGITAL_SCALE_12BITS) - (uint32_t) 25000;
    value = ((max * (uint32_t) 50000 / DIGITAL_SCALE_12BITS) - (uint32_t) 25000)*0.707 - adc_offset;
    return value;
}


void dioLearnBlindMovingTime(struct blind_s *blind, int16_t blindcurrent)
{
    static TickType_t movingtime = 0;
    static TickType_t movinguptime = 0;
    static TickType_t movingdowntime = 0;
    if(movingtime + MINLEARNDELAY < xTaskGetTickCount())
    {
        switch(blind->blindlearn)
        {
            case blindlearn_start:
                blind->position_movingtimeup = MAXMOVINGTIME; //max value
                blind->position_movingtimedown = MAXMOVINGTIME; //max value
                blind->position_actual = MAXMOVINGTIME; //first move will go up, so set position to bottom
                blind->position_target = 0; //set to max value to disable automatic off function;
                blind->blinddirection = blinddirection_up;
                Dio_SetBlindDirection(blind);
                movingtime = blind->starttime; //store the time for MINLEARNDELAY function
                blind->blindlearn = blindlearn_up1;
                break;
            case blindlearn_up1:
                if(blindcurrent < blindcurrent_threshold) //top position reached
                {
                    blind->position_actual = 0; //next move will go down, so set position to top max
                    blind->position_target = MAXMOVINGTIME; //set to max value to disable automatic off function;
                    blind->blinddirection = blinddirection_down;
                    Dio_SetBlindDirection(blind);
                    movingtime = blind->starttime; //store the time of start moving
                    blind->blindlearn = blindlearn_down;
                }
                break;
            case blindlearn_down:
                if(blindcurrent < blindcurrent_threshold) //bottom position reached
                {
                    movingdowntime = xTaskGetTickCount() - movingtime; //store delta time
                    printf("movingdowntime: %ld\r\n", movingdowntime);
                    blind->position_movingtimedown = movingdowntime;

                    blindmovingtimedown[blind->channel-1] = blind->position_movingtimedown;
                    EE_WriteStorage(&eeblindmovingtimedown);

                    blind->position_actual = MAXMOVINGTIME; //next move will go up, so set position to bottom
                    blind->position_target = 0; //set to min value to disable automatic off function;
                    blind->blinddirection = blinddirection_up;
                    Dio_SetBlindDirection(blind);
                    movingtime = blind->starttime; //store the time of start moving
                    blind->blindlearn = blindlearn_up2;
                }
                break;
            case blindlearn_up2:
                if(blindcurrent < blindcurrent_threshold) //top position reached
                {
                    movinguptime = xTaskGetTickCount() - movingtime; //store delta time
                    printf("movinguptime: %ld\r\n", movinguptime);
                    blind->position_movingtimeup = movinguptime;

                    blindmovingtimeup[blind->channel-1] = blind->position_movingtimeup;
                    EE_WriteStorage(&eeblindmovingtimeup);

                    blind->position_actual = 0;
                    blind->position_changed = true;
                    blind->blindlearn = blindlearn_finished;
                    blind->blinddirection = blinddirection_off;
                    Dio_SetBlindDirection(blind);

                }
            default:
                break;
        }
    }
}

int16_t dioMovingAvg(int16_t blindcurrent)
{
    static uint8_t i = 0;
    static int16_t blindcurrent_buf[BLINDCURRENT_BUF_SIZE];

    blindcurrent_buf[i] = blindcurrent;
    i++;
    if(i>=BLINDCURRENT_BUF_SIZE)
    {
        i=0;
    }
    int32_t avg = 0;
    for(int var = 0; var < BLINDCURRENT_BUF_SIZE; ++var)
    {
        avg += blindcurrent_buf[var];
    }

    return avg / BLINDCURRENT_BUF_SIZE;
}

void Dio_StartScanInputTask(void *argument)
{
    int16_t blindcurrent_avg = 0;
    int16_t blindcurrent = 0;
    TickType_t xTicks = xTaskGetTickCount();
    if(HAL_ADC_Start_DMA((ADC_HandleTypeDef*) argument, (uint32_t*) aADCxConvertedData,
    ADC_CONVERTED_DATA_BUFFER_SIZE) != HAL_OK)
    {
        /* ADC conversion start error */
        Error_Handler();
    }

    //all relays are off, get ADC Offset
    for (int8_t var = 0; var < BLINDCURRENT_BUF_SIZE; ++var)
    {
        blindcurrent = dioGetCurrentADC();
        blindcurrent_avg = dioMovingAvg(blindcurrent);
        osDelayUntil(xTicks+10);//100Hz
        xTicks = xTaskGetTickCount();
    }
    adc_offset = blindcurrent_avg;


    /* Infinite loop */
    for(;;)
    {
        for(int var = 0; var < num_doubleswitches; var++)
        {
            dioLearnBlindMovingTime(&blinds[var], blindcurrent_avg);
            dioReadDoubleswitch(&doubleswitches[var]);
            dioTransferDoubleswitch2Blind(var);
            dioCheckBlindPosition(var);
        }

        blindcurrent = dioGetCurrentADC();
        if((blinds[0].blinddirection == blinddirection_down)||(blinds[0].blinddirection == blinddirection_angle_down))
        {
            blindcurrent -= 300;
        }
        blindcurrent_avg = dioMovingAvg(blindcurrent);
        MqttClient_SetMQTTCurrent(blindcurrent_avg);

        osDelayUntil(xTicks+10);//100Hz
        xTicks = xTaskGetTickCount();
    }
}

void Dio_InitBlinds()
{
    for(int var = 0; var < num_blinds; ++var)
    {
        blinds[var].channel = var + 1;
        blinds[var].downRelay_Pin = Relay_Pins[var * 2];
        blinds[var].downRelay_Port = Relay_Ports[var * 2];
        blinds[var].upRelay_Pin = Relay_Pins[var * 2 + 1];
        blinds[var].upRelay_Port = Relay_Ports[var * 2 + 1];
        blinds[var].blinddirection = blinddirection_off;
        blinds[var].position_function_active = true;
        blinds[var].position_actual = 0;
        blinds[var].position_changed = true;
        blinds[var].blindlearn = blindlearn_finished;
        blinds[var].angle_function_active = false;
        blinds[var].angle_actual = 0;
        blinds[var].angle_movingtime = 1000;
    }
}


void Dio_SetBlindsMovingTimeUp(uint32_t *blindsmovingtime)
{
    for(int var = 0; var < num_blinds; ++var)
    {
        blinds[var].position_movingtimeup = (uint32_t) blindsmovingtime[var];
    }
}

void Dio_SetBlindsMovingTimeDown(uint32_t *blindsmovingtime)
{
    for(int var = 0; var < num_blinds; ++var)
    {
        blinds[var].position_movingtimedown = (uint32_t) blindsmovingtime[var];
    }
}

void Dio_SetBlindsPos50(uint8_t *blindspos50)
{
    for(int var = 0; var < num_blinds; var++)
    {
        blinds[var].position_50 = blindspos50[var];
    }
}

void Dio_SetRaffstore(bool *raffstore)
{
    for(int var = 0; var < num_blinds; var++)
    {
        blinds[var].angle_function_active = raffstore[var];
    }
}
void Dio_SetRaffstoreMovingtime(uint16_t *raffmovingtime)
{
    for(int var = 0; var < num_blinds; ++var)
    {
        blinds[var].angle_movingtime = (uint16_t) raffmovingtime[var];
    }
}
void Dio_SetBlindInputMatrix(uint16_t *blindinputmatrix)
{
    for(int var = 0; var < num_blinds; ++var)
    {
        blinds[var].inputmatrix = (uint16_t) blindinputmatrix[var];
    }
}
void Dio_SetPositionFunction(bool *position_function_active)
{
    for(int var = 0; var < num_blinds; var++)
    {
        blinds[var].position_function_active = position_function_active[var];
    }
}

void Dio_SetBlindDirection(struct blind_s *blind)
{
    switch(blind->blinddirection)
    {
        case blinddirection_angle_up:
            if(blind->angle_actual > blind->angle_target) // move up
            {
                HAL_GPIO_WritePin(blind->downRelay_Port, blind->downRelay_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(blind->upRelay_Port, blind->upRelay_Pin, GPIO_PIN_SET);
                blind->starttime = xTaskGetTickCount();
            }
            break;
        case blinddirection_angle_down:
            if (blind->angle_actual < blind->angle_target) //move down
            {
                HAL_GPIO_WritePin(blind->downRelay_Port, blind->downRelay_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(blind->upRelay_Port, blind->upRelay_Pin, GPIO_PIN_SET);
                blind->starttime = xTaskGetTickCount();
            }
            break;
        case blinddirection_up:
            if(blind->position_actual > blind->position_target) //check if we are not on the top position
            {
                HAL_GPIO_WritePin(blind->downRelay_Port, blind->downRelay_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(blind->upRelay_Port, blind->upRelay_Pin, GPIO_PIN_SET);
                blind->starttime = xTaskGetTickCount();// + MOTORSTARTDELAY;
            }
            else
            {
                blind->blinddirection = blinddirection_off;
                HAL_GPIO_WritePin(blind->downRelay_Port, blind->downRelay_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(blind->upRelay_Port, blind->upRelay_Pin, GPIO_PIN_RESET);
                blind->starttime = 0;
            }
            break;
        case blinddirection_down:

            if(blind->position_actual < blind->position_target) //check if we are not on the bottom position
            {
                HAL_GPIO_WritePin(blind->downRelay_Port, blind->downRelay_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(blind->upRelay_Port, blind->upRelay_Pin, GPIO_PIN_SET);
                blind->starttime = xTaskGetTickCount();// + MOTORSTARTDELAY;
            }
            else
            {
                blind->blinddirection = blinddirection_off;
                HAL_GPIO_WritePin(blind->downRelay_Port, blind->downRelay_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(blind->upRelay_Port, blind->upRelay_Pin, GPIO_PIN_RESET);
                blind->starttime = 0;
            }
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

void Dio_InitDoubleswitches(void)
{
    for(uint8_t var = 0; var < num_doubleswitches; ++var)
    {
        doubleswitches[var].channel = var + 1;
        doubleswitches[var].downInput_Pin = Input_Pins[var * 2];
        doubleswitches[var].downInput_Port = Input_Ports[var * 2];
        doubleswitches[var].upInput_Pin = Input_Pins[var * 2 + 1];
        doubleswitches[var].upInput_Port = Input_Ports[var * 2 + 1];
        doubleswitches[var].inputdirection = blinddirection_off;
        doubleswitches[var].changed = false;
        doubleswitches[var].updebounce = 0;
        doubleswitches[var].downdebounce = 0;
    }
}

struct blind_s *Dio_GetBlinds(void)
{
    return blinds;
}

struct doubleswitch_s *Dio_GetDoubleswitches(void)
{
    return doubleswitches;
}

void Dio_SetBlindcurrentThreshold(int16_t value)
{
    blindcurrent_threshold = value;
}
uint16_t Dio_GetBlindcurrentThreshold(void)
{
    return blindcurrent_threshold;
}

/**
 * @brief  Conversion complete callback in non-blocking mode
 * @param  hadc: ADC handle
 * @retval None
 */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
    /* Invalidate Data Cache to get the updated content of the SRAM on the first half of the ADC converted data buffer: 32 bytes */
    SCB_InvalidateDCache_by_Addr((uint32_t*) &aADCxConvertedData[0], ADC_CONVERTED_DATA_BUFFER_SIZE);
}

/**
 * @brief  Conversion DMA half-transfer callback in non-blocking mode
 * @param  hadc: ADC handle
 * @retval None
 */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    /* Invalidate Data Cache to get the updated content of the SRAM on the second half of the ADC converted data buffer: 32 bytes */
    SCB_InvalidateDCache_by_Addr((uint32_t*) &aADCxConvertedData[ADC_CONVERTED_DATA_BUFFER_SIZE / 2],
    ADC_CONVERTED_DATA_BUFFER_SIZE);
}