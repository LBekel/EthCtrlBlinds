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
struct ee_storage_s eemqtttopic;
extern struct ee_storage_s eemqtthost;

#define theCGItableSize 2
tCGI theCGItable[theCGItableSize];

#define SSITAGS C(tag1)C(tag2)C(tag3)C(tag4)C(tag5)C(tag6)C(tag7)C(tag8)C(tag9)C(tag10)C(tag11)C(tag12)C(tag13)C(tag14)C(tag15)C(tag16)C(mqtttopic)C(mqtthost)C(blind1)
#define C(x) x,
enum eSSItags { SSITAGS numSSItags };
#undef C

#define C(x) #x,
const char * const theSSItags[] = { SSITAGS };

// function to initialize CGI
void myCGIinit(void)
{
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
	for (uint8_t var = 0; var < iNumParams; var++)
	{
		if (strncmp(pcParam[var], "blind",5) == 0)
		{
			//uint8_t channel = 0;
			//sscanf(pcValue[var], "%"PRIu8"", &channel);
			//channel--;
			if (strcmp(pcValue[var], "off") == 0)
			{
				blinds[var].blinddirection = blinddirection_off;
			}
			else if (strcmp(pcValue[var], "up") == 0)
			{
				blinds[var].blinddirection = blinddirection_up;
			}
			else if (strcmp(pcValue[var], "down") == 0)
			{
				blinds[var].blinddirection = blinddirection_down;
			}
		}
	}

	for (uint8_t var = 0; var < iNumParams; ++var)
	{
		setBlindDirection(&blinds[var]);
		publish_blind_state(&blinds[var]);
	}

	return "/index.shtml";

}


const char* MqttCGIhandler(int iIndex, int iNumParams, char *pcParam[],
		char *pcValue[]) {

	for (uint8_t var = 0; var < iNumParams; var++) {
		if (strcmp(pcParam[var], theSSItags[mqtttopic]) == 0)
		{
			sprintf((char*)eemqtttopic.pData, pcValue[var]);
			EE_WriteStorage(&eemqtttopic);
			setMQTTTopic((char*)pcValue[var]);
		}
		if (strcmp(pcParam[var], theSSItags[mqtthost]) == 0)
		{
			ip_addr_t mqtt_host_addr;
			ipaddr_aton((char*)pcValue[var],&mqtt_host_addr);
			//printf("New Host IP Address: %s\r\n", ipaddr_ntoa(&mqtt_host_addr));
			memcpy(eemqtthost.pData,&mqtt_host_addr,4);
			EE_WriteStorage(&eemqtthost);
			setMQTTHost(&mqtt_host_addr);
		}
	}
	return "/index.shtml";
}

// the actual function for SSI
u16_t mySSIHandler(int iIndex, char *pcInsert, int iInsertLen) {
	if (iIndex < 8)
	{
		if (blinds[iIndex].blinddirection == blinddirection_off) {
			char myStr[300];
			sprintf(myStr,"<select name=\"blind%d\" id=\"blind%d\">\
			  <option value=\"up\">up</option>\
			  <option value=\"down\">down</option>\
			  <option selected value=\"off\">off</option>\
			</select>",iIndex + 1,iIndex + 1);
			strcpy(pcInsert, myStr);
			return strlen(myStr);
		} else if (blinds[iIndex].blinddirection == blinddirection_up) {
			char myStr[300];
			sprintf(myStr,"<select name=\"blind%d\" id=\"blind%d\">\
			  <option selected value=\"up\">up</option>\
			  <option value=\"down\">down</option>\
			  <option value=\"off\">off</option>\
			</select>",iIndex + 1,iIndex + 1);
			strcpy(pcInsert, myStr);
			return strlen(myStr);
		} else if (blinds[iIndex].blinddirection == blinddirection_down) {
			char myStr[300];
			sprintf(myStr,"<select name=\"blind%d\" id=\"blind%d\">\
			  <option value=\"up\">up</option>\
			  <option selected value=\"down\">down</option>\
			  <option value=\"off\">off</option>\
			</select>",iIndex + 1,iIndex + 1);
			strcpy(pcInsert, myStr);
			return strlen(myStr);
		}
	}
	if(iIndex == mqtttopic)
	{
		char myStr[100];
		char tempTopic[27];
		getMQTTTopic(tempTopic);
		sprintf(myStr,	"<input value=\"%s\" name=\"mqtttopic\" type=\"text\" id=\"mqtttopic\" size=\"50\">", tempTopic);
		strcpy(pcInsert, myStr);
		return strlen(myStr);
	}
	if(iIndex == mqtthost)
	{
		char myStr[100];
		ip_addr_t mqtt_host_addr;
		getMQTTHost(&mqtt_host_addr);
		sprintf(myStr,	"<input value=\"%s\" name=\"mqtthost\" type=\"text\" id=\"mqtthost\" size=\"50\">", ipaddr_ntoa(&mqtt_host_addr));
		strcpy(pcInsert, myStr);
		return strlen(myStr);
	}
	return 0;
}
