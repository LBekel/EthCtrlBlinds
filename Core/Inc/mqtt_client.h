/*
 * mqtt_client.h
 *
 *  Created on: Jan 11, 2021
 *      Author: LBekel
 */



#ifndef TRIGGER_MQTT_CLIENT_H_
#define TRIGGER_MQTT_CLIENT_H_

#include "lwip.h"

void StartmqttTask(void *argument);
void publish_relay_states(void);
void getMQTTTopic(char * topic);
void setMQTTTopic(char * topic);

#endif /* TRIGGER_MQTT_CLIENT_H_ */
