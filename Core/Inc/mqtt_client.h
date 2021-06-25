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

void StartmqttTask(void *argument);
void publish_relay_states(void);
void publish_input_states(bool force);
void publish_ip_mac(void);
void publish_blind_state(struct blind_s *blind);
void publish_blind_cmd(struct blind_s *blind);
void getMQTTTopic(char * topic);
void setMQTTTopic(char * topic);
void getMQTTHost(ip_addr_t * mqtt_host_addr);
void setMQTTHost(ip_addr_t * mqtt_host_addr);

#endif /* TRIGGER_MQTT_CLIENT_H_ */
