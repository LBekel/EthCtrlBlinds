/*
 * dio.h
 *
 *  Created on: Jun 10, 2021
 *      Author: LBekel
 */

#ifndef INC_DIO_H_
#define INC_DIO_H_

#include <stdbool.h>
#include "FreeRTOS.h"

typedef enum {
	blinddirection_off,
	blinddirection_up,
	blinddirection_down,
	blinddirection_angle_up,
	blinddirection_angle_down,
}blinddirection_t;

typedef enum {
    blindlearn_finished,
    blindlearn_start,
    blindlearn_up1,
    blindlearn_down,
    blindlearn_up2
}blindlearn_t;

struct blind_s{
	uint8_t channel;
	uint16_t downRelay_Pin;
	GPIO_TypeDef * downRelay_Port;
	uint16_t upRelay_Pin;
	GPIO_TypeDef * upRelay_Port;
	blinddirection_t blinddirection;
	int32_t position_actual; //ms
	int32_t position_target;//ms
	bool position_changed;
	bool angle_function_active;
	int32_t angle_actual; //ms
	int32_t angle_target; //ms
	TickType_t angle_movingtime; //ms
	bool angle_changed;
	TickType_t position_movingtimeup; //ms
	TickType_t position_movingtimedown; //ms
	TickType_t starttime; //ms
	blindlearn_t blindlearn;
	uint8_t position_50; //50percent position
};

typedef enum {
	inputdirection_off,
	inputdirection_up,
	inputdirection_down,
	inputdirection_up_end,
	inputdirection_down_end
}inputdirection_t;

struct doubleswitch_s{
	uint8_t channel;
	uint16_t downInput_Pin;
	GPIO_TypeDef * downInput_Port;
	float downdebounce;
	uint16_t upInput_Pin;
	GPIO_TypeDef * upInput_Port;
    float updebounce;
	inputdirection_t inputdirection;
	bool changed;
	TickType_t downInput_starttime; //ms
	TickType_t upInput_starttime; //ms
};


/* Definitions of environment analog values */
  /* Value of analog reference voltage (Vref+), connected to analog voltage   */
  /* supply Vdda (unit: mV).                                                  */
  #define VDDA_APPLI                       ((uint32_t)3300)

/* Definitions of data related to this example */
  /* Full-scale digital value with a resolution of 12 bits (voltage range     */
  /* determined by analog voltage references Vref+ and Vref-,                 */
  /* refer to reference manual).                                              */
  #define DIGITAL_SCALE_12BITS             ((uint32_t) 0xFFF)

  /* Init variable out of ADC expected conversion data range */
  #define VAR_CONVERTED_DATA_INIT_VALUE    (DIGITAL_SCALE_12BITS + 1)

/* Private macro -------------------------------------------------------------*/

/**
  * @brief  Macro to calculate the voltage (unit: mVolt)
  *         corresponding to a ADC conversion data (unit: digital value).
  * @note   ADC measurement data must correspond to a resolution of 12bits
  *         (full scale digital value 4095). If not the case, the data must be
  *         preliminarily rescaled to an equivalent resolution of 12 bits.
  * @note   Analog reference voltage (Vref+) must be known from
  *         user board environment.
  * @param  __VREFANALOG_VOLTAGE__ Analog reference voltage (unit: mV)
  * @param  __ADC_DATA__ ADC conversion data (resolution 12 bits)
  *                       (unit: digital value).
  * @retval ADC conversion data equivalent voltage value (unit: mVolt)
  */
#define __ADC_CALC_DATA_VOLTAGE(__VREFANALOG_VOLTAGE__, __ADC_DATA__)  \
  ((__ADC_DATA__) * (__VREFANALOG_VOLTAGE__) / DIGITAL_SCALE_12BITS)

#define num_relay_ch 16
#define num_input_ch 18
#define num_blinds 8
#define num_doubleswitches 9
struct blind_s blinds[num_blinds];
struct doubleswitch_s doubleswitches[num_doubleswitches];

uint16_t Relay_Pins[num_relay_ch];
GPIO_TypeDef * Relay_Ports[num_relay_ch];
uint16_t Input_Pins[num_input_ch];
GPIO_TypeDef * Input_Ports[num_input_ch];

void initBlinds(void);
void setBlindsMovingTimeUp(uint32_t * blindsmovingtime);
void setBlindsMovingTimeDown(uint32_t *blindsmovingtime);
void setBlindsPos50(uint8_t *blindspos50);
void setRaffstore(bool *raffstore);
void setRaffstoreMovingtime(uint16_t *raffmovingtime);
void setBlindDirection(struct blind_s *blind);
void initDoubleswitches(void);
void readDoubleswitches(void);
void readDoubleswitch(struct doubleswitch_s *doubleswitch);
void setBlindcurrentThreshold(int16_t value);
uint16_t getBlindcurrentThreshold(void);
void StartScanInputsTask(void *argument);
uint8_t calc_real_position(struct blind_s *blind);
void calc_position(uint8_t percent, struct blind_s *blind);

#endif /* INC_DIO_H_ */
