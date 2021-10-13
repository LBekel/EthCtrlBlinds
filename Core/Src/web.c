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
const char* LearnCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* BootloaderCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
uint16_t mySSIHandler(int iIndex, char *pcInsert, int iInsertLen);

// in our HTML file <form method="get" action="/relay.cgi">
const tCGI RelayCGI = {"/relay.cgi", RelayCGIhandler};
const tCGI MqttCGI = {"/mqtt.cgi", MqttCGIhandler};
const tCGI LearnCGI = {"/learn.cgi", LearnCGIhandler};
const tCGI BootloaderCGI = {"/bootloader.cgi", BootloaderCGIhandler};
struct ee_storage_s eemqtttopic;
extern struct ee_storage_s eemqtthost;

#define theCGItableSize 4
tCGI theCGItable[theCGItableSize];

#define SSITAGS C(blind1)C(blind2)C(blind3)C(blind4)C(blind5)C(blind6)C(blind7)C(blind8)C(mqtttopic)C(mqtthost)C(current)C(time1)C(time2)C(time3)C(time4)C(time5)C(time6)C(time7)C(time8)C(pos1)C(pos2)C(pos3)C(pos4)C(pos5)C(pos6)C(pos7)C(pos8)C(compiled)
#define C(x) x,
enum eSSItags { SSITAGS numSSItags };
#undef C

#define C(x) #x,
const char *const theSSItags[] = { SSITAGS };

// function to initialize CGI
void myCGIinit(void)
{
    theCGItable[0] = RelayCGI;
    theCGItable[1] = MqttCGI;
    theCGItable[2] = LearnCGI;
    theCGItable[3] = BootloaderCGI;
    //give the table to the HTTP server
    http_set_cgi_handlers(theCGItable, theCGItableSize);
}

// function to initialize SSI
void mySSIinit(void)
{
    http_set_ssi_handler(mySSIHandler, (char const**) theSSItags, numSSItags);
}

// the actual function for handling CGI
const char* RelayCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    for(uint8_t var = 0; var < iNumParams; var++)
    {
        if(strncmp(pcParam[var], "blind", 5) == 0)
        {
            if(strcmp(pcValue[var], "up") == 0)
            {
                blinds[var].blinddirection = blinddirection_up;
                blinds[var].position_target = 0;
            }
            else if(strcmp(pcValue[var], "down") == 0)
            {
                blinds[var].blinddirection = blinddirection_down;
                blinds[var].position_target = blinds[var].position_movingtime;

            }
            else
            {
                blinds[var].blinddirection = blinddirection_off;
            }
        }
    }

    for(uint8_t var = 0; var < iNumParams; ++var)
    {
        setBlindDirection(&blinds[var]);
        publish_blinddir_stat(&blinds[var]);
    }

    return "/return.html";

}

const char* MqttCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{

    for(uint8_t var = 0; var < iNumParams; var++)
    {
        if(strcmp(pcParam[var], theSSItags[mqtttopic]) == 0)
        {
            sprintf((char*) eemqtttopic.pData, pcValue[var]);
            EE_WriteStorage(&eemqtttopic);
            setMQTTTopic((char*) pcValue[var]);
        }
        if(strcmp(pcParam[var], theSSItags[mqtthost]) == 0)
        {
            ip_addr_t mqtt_host_addr;
            ipaddr_aton((char* )pcValue[var], &mqtt_host_addr);
            //printf("New Host IP Address: %s\r\n", ipaddr_ntoa(&mqtt_host_addr));
            memcpy(eemqtthost.pData, &mqtt_host_addr, 4);
            EE_WriteStorage(&eemqtthost);
            setMQTTHost(&mqtt_host_addr);
        }
    }
    return "/return.html";
}

const char* LearnCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    for(uint8_t var = 0; var < iNumParams; var++)
    {
        if(strcmp(pcParam[var], theSSItags[current]) == 0)
        {
            sscanf(pcValue[var], "%"PRIu16"", &currentthreshold);
            EE_WriteStorage(&eecurrentthreshold);
            printf("Current threshold: %d\r\n", currentthreshold);
            setBlindcurrentThreshold(currentthreshold);
        }
        if(strncmp(pcParam[var], "blind", 5) == 0)
        {
            uint8_t channel = 0;
            sscanf(pcValue[var], "%"PRIu8"", &channel);
            channel--;
            blinds[channel].blindlearn = blindlearn_start;
        }
    }
    return "/return.html";
}

const char* BootloaderCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    setReset();
    printf("Reset\r\n");

    return "/startapp.html";
}

// the actual function for SSI
uint16_t mySSIHandler(int iIndex, char *pcInsert, int iInsertLen)
{
    char myStr[LWIP_HTTPD_MAX_TAG_INSERT_LEN];
    if((iIndex >= blind1) && (iIndex <= blind8))
    {
        if(blinds[iIndex].blinddirection == blinddirection_off)
        {
            sprintf(myStr,
                    "<select name=\"blind%d\" id=\"blind%d\">"
			         "<option value=\"up\">up</option>"
			         "<option value=\"down\">down</option>"
			         "<option selected value=\"off\">off</option>"
			         "</select>",
                    iIndex + 1, iIndex + 1);
            strcpy(pcInsert, myStr);
            return strlen(myStr);
        }
        else if(blinds[iIndex].blinddirection == blinddirection_up)
        {
            sprintf(myStr,
                    "<select name=\"blind%d\" id=\"blind%d\">"
			         "<option selected value=\"up\">up</option>"
			         "<option value=\"down\">down</option>"
			         "<option value=\"off\">off</option>"
			         "</select>",
                    iIndex + 1, iIndex + 1);
            strcpy(pcInsert, myStr);
            return strlen(myStr);
        }
        else if(blinds[iIndex].blinddirection == blinddirection_down)
        {
            sprintf(myStr,
                    "<select name=\"blind%d\" id=\"blind%d\">"
			         "<option value=\"up\">up</option>"
			         "<option selected value=\"down\">down</option>"
			         "<option value=\"off\">off</option>"
			         "</select>",
                    iIndex + 1, iIndex + 1);
            strcpy(pcInsert, myStr);
            return strlen(myStr);
        }
    }
    if((iIndex >= time1) && (iIndex <= time8))
    {
        sprintf(myStr, "%ldms", blindmovingtime[iIndex - time1]);
        strcpy(pcInsert, myStr);
        return strlen(myStr);
    }
    if((iIndex >= pos1) && (iIndex <= pos8))
    {
        sprintf(myStr, "%ldms", blinds[iIndex - pos1].position_actual);
        strcpy(pcInsert, myStr);
        return strlen(myStr);
    }
    if(iIndex == mqtttopic)
    {
        char tempTopic[27];
        getMQTTTopic(tempTopic);
        sprintf(myStr, "<input value=\"%s\" name=\"mqtttopic\" type=\"text\" id=\"mqtttopic\" size=\"50\">", tempTopic);
        strcpy(pcInsert, myStr);
        return strlen(myStr);
    }
    if(iIndex == mqtthost)
    {
        ip_addr_t mqtt_host_addr;
        getMQTTHost(&mqtt_host_addr);
        sprintf(myStr, "<input value=\"%s\" name=\"mqtthost\" type=\"text\" id=\"mqtthost\" size=\"50\">",
                ipaddr_ntoa(&mqtt_host_addr));
        strcpy(pcInsert, myStr);
        return strlen(myStr);
    }
    if(iIndex == current)
    {
        sprintf(myStr, "<input value=\"%d\" name=\"current\" type=\"text\" id=\"current\" size=\"10\">",
                getBlindcurrentThreshold());
        strcpy(pcInsert, myStr);
        return strlen(myStr);
    }
    if(iIndex == compiled)
    {
        sprintf(myStr, __DATE__ " " __TIME__);
        strcpy(pcInsert, myStr);
        return strlen(myStr);
    }
    return 0;
}
