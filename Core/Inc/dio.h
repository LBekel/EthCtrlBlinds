/*
 * dio.h
 *
 *  Created on: Jun 10, 2021
 *      Author: LBekel
 */

#ifndef INC_DIO_H_
#define INC_DIO_H_

#include <stdbool.h>

typedef enum {
	blindsdirection_off,
	blindsdirection_up,
	blindsdirection_down
}blinddirection_t;

struct blind_s{
	uint8_t channel;
	uint16_t downRelay_Pin;
	GPIO_TypeDef * downRelay_Port;
	uint16_t upRelay_Pin;
	GPIO_TypeDef * upRelay_Port;
	blinddirection_t blinddirection;
	uint8_t blindposition;
};

struct doubleswitch_s{
	uint8_t channel;
	uint16_t downInput_Pin;
	GPIO_TypeDef * downInput_Port;
	uint16_t upInput_Pin;
	GPIO_TypeDef * upInput_Port;
	blinddirection_t inputdirection;
	bool changed;
};

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
void setBlindsDirection(struct blind_s *blind);
void initDoubleswitches(void);
void readDoubleswitches(void);
void readDoubleswitch(struct doubleswitch_s *doubleswitch);
void StartScanInputsTask(void *argument);

#endif /* INC_DIO_H_ */
