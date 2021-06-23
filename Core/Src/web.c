/*
 * web.c
 *
 *  Created on: Jun 8, 2021
 *      Author: LBekel
 */

#include "main.h"
#include "mqtt_client.h"
#include "lwip/apps/httpd.h"
#include "string.h"
#include <stdio.h>
#include <stdbool.h>
#include "dio.h"

const char* RelayCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* MqttCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
u16_t mySSIHandler(int iIndex, char *pcInsert, int iInsertLen);

bool RelayStatesTemp[num_relay_ch];

// in our HTML file <form method="get" action="/relay.cgi">
const tCGI RelayCGI = { "/relay.cgi", RelayCGIhandler };
// in our HTML file <form method="get" action="/mqtt.cgi">
const tCGI MqttCGI = { "/mqtt.cgi", MqttCGIhandler };
struct ee_storage_s topic;

#define theCGItableSize 2

tCGI theCGItable[theCGItableSize];


// [* SSI #2 *]
#define numSSItags 18

// [* SSI #3 *]
char const *theSSItags[numSSItags] = { "tag1", "tag2", "tag3", "tag4", "tag5",
		"tag6", "tag7", "tag8", "tag9", "tag10", "tag11", "tag12", "tag13", "tag14",
		"tag15", "tag16", "mqtttopic", "mqtthost"};

// function to initialize CGI
void myCGIinit(void)
{
	//add LED control CGI to the table
	theCGItable[0] = RelayCGI;
	theCGItable[1] = MqttCGI;
	//give the table to the HTTP server
	http_set_cgi_handlers(theCGItable, theCGItableSize);
}

// function to initialize SSI
void mySSIinit(void)
{
	http_set_ssi_handler(mySSIHandler, (char const**) theSSItags, numSSItags);
}



// the actual function for handling CGI
const char* RelayCGIhandler(int iIndex, int iNumParams, char *pcParam[],
		char *pcValue[])
{
	for (uint8_t var = 0; var < sizeof(RelayStatesTemp); var++)
	{
		RelayStatesTemp[var] = false;
	}
	for (uint8_t var = 0; var < iNumParams; var++)
	{
		if (strcmp(pcParam[var], "relay") == 0)
		{
			uint8_t channel = 0;
			sscanf(pcValue[var], "%"PRIu8"", &channel);
			channel--;
			RelayStatesTemp[channel] = true;
		}
	}

	for (uint8_t var = 0; var < num_relay_ch; ++var)
	{
		HAL_GPIO_WritePin(Relay_Ports[var], Relay_Pins[var], RelayStatesTemp[var]);
		RelayStates[var] = RelayStatesTemp[var];
	}
	//publish_relay_states();
	// the extension .shtml for SSI to work
	return "/index.shtml";

}


const char* MqttCGIhandler(int iIndex, int iNumParams, char *pcParam[],
		char *pcValue[]) {

	uint32_t var = 0;

	for (var = 0; var < iNumParams; var++) {
		if (strcmp(pcParam[var], "mqtttopic") == 0)
		{
			printf("topic %s\r\n",pcValue[var]);
			sprintf((char*)topic.pData, pcValue[var]);
			EE_WriteStorage(&topic);
			setMQTTTopic(*pcValue);
			printf("topic %s\r\n",(char*)topic.pData);
		}
		if (strcmp(pcParam[var], "mqtthost") == 0)
		{

		}
	}
	return "/index.shtml";

}

// the actual function for SSI
u16_t mySSIHandler(int iIndex, char *pcInsert, int iInsertLen) {
	if (iIndex < num_relay_ch) //tag 1-16
	{
		if (RelayStates[iIndex] == false) {
			char myStr[64];
			sprintf(myStr,	"<input value=\"%d\" name=\"relay\" type=\"checkbox\">",iIndex + 1);
			strcpy(pcInsert, myStr);
			return strlen(myStr);
		} else if (RelayStates[iIndex] == true) {
			char myStr[64];
			sprintf(myStr,	"<input value=\"%d\" name=\"relay\" type=\"checkbox\" checked>", iIndex + 1);
			strcpy(pcInsert, myStr);
			return strlen(myStr);
		}
	}
	if(iIndex == 16) //tag "mqtttopic"
	{
		char myStr[100];// ="<input value=\"Test\" name=\"topic\" type=\"text\" id=\"topic\">";
		char tempTopic[27];
		getMQTTTopic(tempTopic);
		printf("TempTopic: %s\r\n",tempTopic);
		sprintf(myStr,	"<input value=\"%s\" name=\"mqtttopic\" type=\"text\" id=\"mqtttopic\" size=\"50\">", tempTopic);
		strcpy(pcInsert, myStr);
		return strlen(myStr);
	}
	if(iIndex == 17) //tag "mqtthost"
	{
		char myStr[94];
		char tempHost[27] = "\0";
		sprintf(myStr,	"<input value=\"%s\" name=\"mqtthost\" type=\"text\" id=\"mqtthost\" size=\"50\">", tempHost);
		strcpy(pcInsert, myStr);
		return strlen(myStr);
	}
	return 0;
}
