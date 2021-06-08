/*
 * mqtt_client.h
 *
 *  Created on: Jan 11, 2021
 *      Author: LBekel
 */



#ifndef TRIGGER_MQTT_CLIENT_H_
#define TRIGGER_MQTT_CLIENT_H_

#include <stdbool.h>

#define num_relay_ch 16
#define num_input_ch 18

bool RelayStates[num_relay_ch];
bool InputStates[num_input_ch];
uint16_t Relay_Pins[num_relay_ch];
GPIO_TypeDef * Relay_Ports[num_relay_ch];
uint16_t Input_Pins[num_input_ch];
GPIO_TypeDef * Input_Ports[num_input_ch];

void StartmqttTask(void *argument);
void publish_relay_states(void);

#endif /* TRIGGER_MQTT_CLIENT_H_ */
