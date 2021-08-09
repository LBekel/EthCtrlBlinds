/*
 * mqtt_client.h
 *
 *  Created on: Jan 11, 2021
 *      Author: LBekel
 */



#ifndef TRIGGER_MQTT_CLIENT_H_
#define TRIGGER_MQTT_CLIENT_H_

#include "lwip.h"
#include "dio.h"

typedef enum {
	inpub_unknown,
	inpub_blindcmnd
}inpub_t;

void StartmqttTask(void *argument);
void publish_doubleswitch_states(void);
void publish_doubleswitch_state(struct doubleswitch_s *doubleswitch);
void publish_ip_mac(void);
void publish_blind_state(struct blind_s *blind);
void publish_blind_position(struct blind_s *blind);
void publish_blind_cmd(struct blind_s *blind);
void publish_current(void);
void getMQTTTopic(char * topic);
void setMQTTTopic(char * topic);
void getMQTTHost(ip_addr_t * mqtt_host_addr);
void setMQTTHost(ip_addr_t * mqtt_host_addr);
void setCurrent(int16_t _current);

#endif /* TRIGGER_MQTT_CLIENT_H_ */
