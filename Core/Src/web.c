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
const char* SettingCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* BootloaderCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* PositionCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
uint16_t mySSIHandler(int iIndex, char *pcInsert, int iInsertLen);

// in our HTML file <form method="get" action="/relay.cgi">
const tCGI RelayCGI = {"/relay.cgi", RelayCGIhandler};
const tCGI MqttCGI = {"/mqtt.cgi", MqttCGIhandler};
const tCGI LearnCGI = {"/learn.cgi", LearnCGIhandler};
const tCGI SettingCGI = {"/setting.cgi", SettingCGIhandler};
const tCGI BootloaderCGI = {"/bootloader.cgi", BootloaderCGIhandler};
const tCGI PositionCGI = {"/position.cgi", PositionCGIhandler};
struct ee_storage_s eemqtttopic;
extern struct ee_storage_s eemqtthost;

#define theCGItableSize 6
tCGI theCGItable[theCGItableSize];

#define SSITAGS C(blind1)C(blind2)C(blind3)C(blind4)C(blind5)C(blind6)C(blind7)C(blind8)C(mqtttopic)C(mqtthost)C(current)C(timeup1)C(timeup2)C(timeup3)C(timeup4)C(timeup5)C(timeup6)C(timeup7)C(timeup8)C(timedo1)C(timedo2)C(timedo3)C(timedo4)C(timedo5)C(timedo6)C(timedo7)C(timedo8)C(pos1)C(pos2)C(pos3)C(pos4)C(pos5)C(pos6)C(pos7)C(pos8)C(compiled)C(per50_1)C(per50_2)C(per50_3)C(per50_4)C(per50_5)C(per50_6)C(per50_7)C(per50_8)C(raff1)C(raff2)C(raff3)C(raff4)C(raff5)C(raff6)C(raff7)C(raff8)C(rafftim1)C(rafftim2)C(rafftim3)C(rafftim4)C(rafftim5)C(rafftim6)C(rafftim7)C(rafftim8)C(angle1)C(angle2)C(angle3)C(angle4)C(angle5)C(angle6)C(angle7)C(angle8)
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
    theCGItable[3] = SettingCGI;
    theCGItable[4] = BootloaderCGI;
    theCGItable[5] = PositionCGI;
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
            uint8_t channel = 0;
            sscanf(pcParam[var]+5, "%"PRIu8"", &channel);
            channel--;

            if(strcmp(pcValue[channel], "up") == 0)
            {
                blinds[channel].blinddirection = blinddirection_up;
                blinds[channel].position_target = 0 - 1000;
                blinds[channel].angle_target = 0;
            }
            else if(strcmp(pcValue[channel], "down") == 0)
            {
                blinds[channel].blinddirection = blinddirection_down;
                blinds[channel].position_target = blinds[var].position_movingtimeup + 1000;
                blinds[channel].angle_target = blinds[channel].angle_movingtime;
            }
            else
            {
                blinds[channel].blinddirection = blinddirection_off;
            }
        }
    }

    for(uint8_t var = 0; var < num_blinds; var++)
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

const char* SettingCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    bool raffstoretemp[] = {false,false,false,false,false,false,false,false};
    for(uint8_t var = 0; var < iNumParams; var++)
    {
        uint8_t channel = 0;
        uint8_t value8 = 0;
        uint16_t value16 = 0;
        uint32_t value32 = 0;
        if(strncmp(pcParam[var], theSSItags[timeup1], 6) == 0)
        {
            sscanf(pcParam[var]+6, "%"PRIu8"", &channel);
            channel--;
            sscanf(pcValue[var], "%"PRIu32"", &value32);
            //only write to eeprom if value has changed
            if (value32!=blindmovingtimeup[channel])
            {
                blindmovingtimeup[channel] = value32;
                EE_WriteStorage(&eeblindmovingtimeup);
                setBlindsMovingTimeUp((uint32_t*) &blindmovingtimeup);
            }
        }

        if(strncmp(pcParam[var], theSSItags[timedo1], 6) == 0)
        {
            sscanf(pcParam[var]+6, "%"PRIu8"", &channel);
            channel--;
            sscanf(pcValue[var], "%"PRIu32"", &value32);
            //only write to eeprom if value has changed
            if (value32!=blindmovingtimedown[channel])
            {
                blindmovingtimedown[channel] = value32;
                EE_WriteStorage(&eeblindmovingtimedown);
                setBlindsMovingTimeDown((uint32_t*) &blindmovingtimedown);
            }
        }
        if(strncmp(pcParam[var], theSSItags[per50_1], 5) == 0)
        {
            sscanf(pcParam[var]+6, "%"PRIu8"", &channel);
            channel--;
            sscanf(pcValue[var], "%2"PRIu8"", &value8);
            if (value8!=blindpos50[channel])
            {
                blindpos50[channel] = value8;
                EE_WriteStorage(&eeblindpos50);
                setBlindsPos50((uint8_t*)&blindpos50);
            }
        }
        if(strncmp(pcParam[var], theSSItags[raff1], 4) == 0)
        {
            sscanf(pcParam[var]+4, "%"PRIu8"", &channel);
            channel--;
            raffstoretemp[channel] = true;
        }

        if(strncmp(pcParam[var], theSSItags[rafftim1], 7) == 0)
        {
            sscanf(pcParam[var]+7, "%"PRIu8"", &channel);
            channel--;
            sscanf(pcValue[var], "%"PRIu16"", &value16);
            //only write to eeprom if value has changed
            if (value16!=raffmovingtime[channel])
            {
                raffmovingtime[channel] = value16;
                EE_WriteStorage(&eeraffmovingtime);
                setRaffstoreMovingtime((uint16_t*) &raffmovingtime);
            }
        }
    }

    for (uint8_t channel = 0; channel < num_blinds; channel++)
    {
        if (raffstoretemp[channel]!=raffstore[channel])
        {
            raffstore[channel] = raffstoretemp[channel];
            EE_WriteStorage(&eeraffstore);
            setRaffstore((bool*)&raffstore);
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


const char* PositionCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    static uint8_t channel = 0;
    uint8_t value = 0;
    for(uint8_t var = 0; var < iNumParams; var++)
    {
        if(strncmp(pcParam[var], theSSItags[per50_1],5) == 0)
        {

            sscanf(pcValue[var], "%2"PRIu8"", &value);
            blindpos50[channel] = value;
            EE_WriteStorage(&eeblindpos50);
            setBlindsPos50((uint8_t*)&blindpos50);
        }
        if(strncmp(pcParam[var], "blind", 5) == 0)
        {
            sscanf(pcValue[var], "%1"PRIu8"", &channel);
            channel--;
        }
    }
    return "/return.html";
}

// the actual function for SSI
uint16_t mySSIHandler(int iIndex, char *pcInsert, int iInsertLen)
{
    char myStr[LWIP_HTTPD_MAX_TAG_INSERT_LEN];
    if((iIndex >= blind1) && (iIndex <= blind8))
    {
        if(blinds[iIndex].blinddirection == blinddirection_up)
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
        else
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
    }
    if((iIndex >= timeup1) && (iIndex <= timeup8))
    {
        uint8_t channel = iIndex - timeup1;
        sprintf(myStr, "<input value=\"%ld\" name=\"timeup%d\" type=\"text\" id=\"timeup%d\" size=\"10\" maxlength=\"6\">",blindmovingtimeup[channel],channel+1,channel+1);
        strcpy(pcInsert, myStr);
        return strlen(myStr);
    }
    if((iIndex >= timedo1) && (iIndex <= timedo8))
    {
        uint8_t channel = iIndex - timedo1;
        sprintf(myStr, "<input value=\"%ld\" name=\"timedo%d\" type=\"text\" id=\"timedo%d\" size=\"10\" maxlength=\"6\">",blindmovingtimedown[channel],channel+1,channel+1);
        strcpy(pcInsert, myStr);
        return strlen(myStr);
    }
    if((iIndex >= per50_1) && (iIndex <= per50_8))
    {
        uint8_t channel = iIndex - per50_1;
        sprintf(myStr, "<input value=\"%d\" name=\"per50_%d\" type=\"text\" id=\"per50_%d\" size=\"4\" maxlength=\"2\">",blindpos50[channel],channel+1,channel+1);
        strcpy(pcInsert, myStr);
        return strlen(myStr);
    }
    if((iIndex >= raff1) && (iIndex <= raff8))
    {
        if(blinds[iIndex-raff1].angle_function_active == true)
        {
            sprintf(myStr, "<input name=\"raff%d\" type=\"checkbox\" id=\"raff%d\" checked/>",iIndex - raff1 + 1,iIndex - raff1 + 1);
        }
        else
        {
            sprintf(myStr, "<input name=\"raff%d\" type=\"checkbox\" id=\"raff%d\"/>",iIndex - raff1 + 1,iIndex - raff1 + 1);
        }
        strcpy(pcInsert, myStr);
        return strlen(myStr);
    }
    if((iIndex >= rafftim1) && (iIndex <= rafftim8))
    {
        uint8_t channel = iIndex - rafftim1;
        sprintf(myStr, "<input value=\"%d\" name=\"rafftim%d\" type=\"text\" id=\"rafftim%d\" size=\"6\" maxlength=\"4\">",raffmovingtime[channel],channel+1,channel+1);
        strcpy(pcInsert, myStr);
        return strlen(myStr);
    }
    if((iIndex >= pos1) && (iIndex <= pos8))
    {
        sprintf(myStr, "%ldms", blinds[iIndex - pos1].position_actual);
        strcpy(pcInsert, myStr);
        return strlen(myStr);
    }
    if((iIndex >= angle1) && (iIndex <= angle8))
    {
        sprintf(myStr, "%ldms", blinds[iIndex - angle1].angle_actual);
        strcpy(pcInsert, myStr);
        return strlen(myStr);
    }
    if(iIndex == mqtttopic)
    {
        char tempTopic[27];
        getMQTTTopic(tempTopic);
        sprintf(myStr, "<input value=\"%s\" name=\"mqtttopic\" type=\"text\" id=\"mqtttopic\" size=\"25\" maxlength=\"10\">", tempTopic);
        strcpy(pcInsert, myStr);
        return strlen(myStr);
    }
    if(iIndex == mqtthost)
    {
        ip_addr_t mqtt_host_addr;
        getMQTTHost(&mqtt_host_addr);
        sprintf(myStr, "<input value=\"%s\" name=\"mqtthost\" type=\"text\" id=\"mqtthost\" size=\"25\">",
                ipaddr_ntoa(&mqtt_host_addr));
        strcpy(pcInsert, myStr);
        return strlen(myStr);
    }
    if(iIndex == current)
    {
        sprintf(myStr, "<input value=\"%d\" name=\"current\" type=\"text\" id=\"current\" size=\"10\" maxlength=\"4\">",
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
