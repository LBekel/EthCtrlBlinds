/*
 *  Created on: Jan 11, 2021
 *      Author: LBekel
 */



#ifndef MQTT_CLIENT_H_
#define MQTT_CLIENT_H_

#include "lwip.h"
#include "dio.h"

typedef enum {
	inpub_unknown,
	inpub_blindcmnd,
	inpud_blindposcmnd,
    inpud_blindangcmnd
}inpub_t;

void MqttClient_StartTask(void *argument);
void MqttClient_PublishDoubleswitchStat(struct doubleswitch_s *doubleswitch);
void MqttClient_PublishBlindDirStat(struct blind_s *blind);
void MqttClient_PublishBlindDirCmd(struct blind_s *blind);
void MqttClient_PublishCurrent(void);
void MqttClient_GetMQTTTopic(char * topic);
void MqttClient_SetMQTTTopic(char * topic);
void MqttClient_GetMQTTHost(ip_addr_t * mqtt_host_addr);
void MqttClient_SetMQTTHost(ip_addr_t * mqtt_host_addr);
void MqttClient_GetMQTTHostString(char *host);
void MqttClient_SetMQTTHostString(const char *host);
void MqttClient_SetMQTTCurrent(int16_t _current);

#endif
