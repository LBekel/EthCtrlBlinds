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
	uint16_t downRelay_Pin;
	GPIO_TypeDef * downRelay_Port;
	uint16_t upRelay_Pin;
	GPIO_TypeDef * upRelay_Port;
	blinddirection_t blinddirection;
	uint8_t blindposition;
};

#define num_relay_ch 16
#define num_input_ch 18
bool RelayStates[num_relay_ch];
bool InputStates[num_input_ch];
struct blind_s blinds[8];

uint16_t Relay_Pins[num_relay_ch];
GPIO_TypeDef * Relay_Ports[num_relay_ch];
uint16_t Input_Pins[num_input_ch];
GPIO_TypeDef * Input_Ports[num_input_ch];

void setBlindsDirection(struct blind_s *blind);

#endif /* INC_DIO_H_ */
