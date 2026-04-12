/*
 * web.c
 *
 *  Created on: Jun 8, 2021
 *      Author: LBekel
 */

#include "web.h"
#include "main.h"
#include "MqttClient.h"
#include "lwip/apps/httpd.h"
#include "string.h"
#include <stdio.h>
#include <stdbool.h>
#include "dio.h"

const char* webBlindsCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* webMqttCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* webLearnCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* webSettingCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* webBootloaderCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
const char* webPositionCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[]);
uint16_t webSSIHandler(int iIndex, char *pcInsert, int iInsertLen);

static bool webIsDnsHostname(const char *host)
{
    size_t len = strlen(host);
    if((len == 0U) || (len > MQTT_HOST_STORAGE_LEN))
    {
        return false;
    }

    size_t label_len = 0U;

    for(size_t i = 0U; i < len; ++i)
    {
        char c = host[i];

        if(!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || (c == '-')))
        {
            return false;
        }

        if((label_len == 0U) && (c == '-'))
        {
            return false;
        }

        ++label_len;
    }

    if((label_len == 0U) || (label_len > 63U) || (host[len - 1U] == '-'))
    {
        return false;
    }

    return true;
}

const tCGI BlindsCGI = {"/blinds.cgi", webBlindsCGIhandler};
const tCGI MqttCGI = {"/mqtt.cgi", webMqttCGIhandler};
const tCGI LearnCGI = {"/learn.cgi", webLearnCGIhandler};
const tCGI SettingCGI = {"/setting.cgi", webSettingCGIhandler};
const tCGI BootloaderCGI = {"/bootloader.cgi", webBootloaderCGIhandler};
const tCGI PositionCGI = {"/position.cgi", webPositionCGIhandler};

struct blind_s *webBlinds_pst;

extern struct ee_storage_s eemqtttopic;
extern struct ee_storage_s eemqtthost;

#define theCGItableSize 6
tCGI theCGItable[theCGItableSize];

#define SSITAGS C(blind1)C(blind2)C(blind3)C(blind4)C(blind5)C(blind6)C(blind7)C(blind8)\
                C(mqtttopic)C(mqtthost)C(current)\
                C(pfunc1)C(pfunc2)C(pfunc3)C(pfunc4)C(pfunc5)C(pfunc6)C(pfunc7)C(pfunc8)\
                C(timeup1)C(timeup2)C(timeup3)C(timeup4)C(timeup5)C(timeup6)C(timeup7)C(timeup8)\
                C(timedo1)C(timedo2)C(timedo3)C(timedo4)C(timedo5)C(timedo6)C(timedo7)C(timedo8)\
                C(pos1)C(pos2)C(pos3)C(pos4)C(pos5)C(pos6)C(pos7)C(pos8)\
                C(compiled)\
                C(per50_1)C(per50_2)C(per50_3)C(per50_4)C(per50_5)C(per50_6)C(per50_7)C(per50_8)\
                C(raff1)C(raff2)C(raff3)C(raff4)C(raff5)C(raff6)C(raff7)C(raff8)\
                C(rafftim1)C(rafftim2)C(rafftim3)C(rafftim4)C(rafftim5)C(rafftim6)C(rafftim7)C(rafftim8)\
                C(angle1)C(angle2)C(angle3)C(angle4)C(angle5)C(angle6)C(angle7)C(angle8)\
                C(input1)C(input2)C(input3)C(input4)C(input5)C(input6)C(input7)C(input8)
#define C(x) x,
enum eSSItags { SSITAGS numSSItags };
#undef C

#define C(x) #x,
const char *const theSSItags[] = { SSITAGS };

// the actual function for handling CGI
const char* webBlindsCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    for(uint8_t var = 0; var < iNumParams; var++)
    {
        if(strncmp(pcParam[var], "blind", 5) == 0)
        {
            uint16_t channel = 0;
            sscanf(pcParam[var]+5, "%"SCNu16"", &channel);
            channel--;

            if(strcmp(pcValue[channel], "up") == 0)
            {
                webBlinds_pst[channel].blinddirection = blinddirection_up;
                webBlinds_pst[channel].position_target = 0 - 1000;
                webBlinds_pst[channel].angle_target = 0;
                if(webBlinds_pst[channel].position_function_active == false)
                {
                    webBlinds_pst[channel].position_actual = webBlinds_pst[channel].position_movingtimeup;
                }
            }
            else if(strcmp(pcValue[channel], "down") == 0)
            {
                webBlinds_pst[channel].blinddirection = blinddirection_down;
                webBlinds_pst[channel].position_target = webBlinds_pst[var].position_movingtimeup + 1000;
                webBlinds_pst[channel].angle_target = webBlinds_pst[channel].angle_movingtime;
                if(webBlinds_pst[channel].position_function_active == false)
                {
                    webBlinds_pst[channel].position_actual = 0;
                }
            }
            else
            {
                webBlinds_pst[channel].blinddirection = blinddirection_off;
            }
        }
    }

    for(uint8_t var = 0; var < num_blinds; var++)
    {
        Dio_SetBlindDirection(&webBlinds_pst[var]);
        MqttClient_PublishBlindDirStat(&webBlinds_pst[var]);
    }

    return "/return.html";

}

/**
 * @brief CGI handler for MQTT settings
 * @param iIndex unused index of the CGI handler
 * @param iNumParams number of parameters sent by the client
 * @param pcParam
 * @param pcValue
 * @return
 */
const char* webMqttCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{

    for(uint8_t var = 0; var < iNumParams; var++)
    {
        if(strcmp(pcParam[var], theSSItags[mqtttopic]) == 0)
        {
            sprintf((char*) eemqtttopic.pData, pcValue[var]);
            EE_WriteStorage(&eemqtttopic);
            MqttClient_SetMQTTTopic((char*) pcValue[var]);
            netif_set_hostname(netif_default, pcValue[var]);
        }
        if(strcmp(pcParam[var], theSSItags[mqtthost]) == 0)
        {
            ip_addr_t mqtt_host_addr;
            if((ipaddr_aton((char*)pcValue[var], &mqtt_host_addr) == 1) || webIsDnsHostname((char*)pcValue[var]))
            {
                snprintf((char*)eemqtthost.pData, MQTT_HOST_STORAGE_LEN, "%s", pcValue[var]);
                EE_WriteStorage(&eemqtthost);
                MqttClient_SetMQTTHostString((char*)eemqtthost.pData);
            }
            else
            {
                printf("ERROR: mqtt host invalid (not IPv4 or DNS): %s\r\n", pcValue[var]);
            }
        }
    }
    return "/return.html";
}

const char* webLearnCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    for(uint8_t var = 0; var < iNumParams; var++)
    {
        if(strcmp(pcParam[var], theSSItags[current]) == 0)
        {
            sscanf(pcValue[var], "%"SCNu16"", &currentthreshold);
            EE_WriteStorage(&eecurrentthreshold);
            printf("Current threshold: %d\r\n", currentthreshold);
            Dio_SetBlindcurrentThreshold(currentthreshold);
        }
        if(strncmp(pcParam[var], "blind", 5) == 0)
        {
            uint16_t channel = 0;
            sscanf(pcValue[var], "%"SCNu16"", &channel);
            channel--;
            webBlinds_pst[channel].blindlearn = blindlearn_start;
        }
    }
    return "/return.html";
}

const char* webSettingCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    bool temp_raffstore = false;
    uint16_t temp_blindinput = 0;
    bool temp_posfunc = false;
    uint16_t inputchannel = 0;
    uint16_t blindchannel = 0;

    for(uint8_t var = 0; var < iNumParams; var++)
    {
        uint16_t value8 = 0;
        uint16_t value16 = 0;
        uint32_t value32 = 0;

        if(strncmp(pcParam[var], theSSItags[input1],5) == 0)
        {
            sscanf(pcParam[var]+7, "%"SCNu16"", &blindchannel);
            blindchannel--;
            sscanf(pcParam[var]+5, "%"SCNu16"", &inputchannel);
            inputchannel--;
            temp_blindinput += 1<<inputchannel;
            continue;
        }

        if(strncmp(pcParam[var], theSSItags[timeup1], 6) == 0)
        {
            sscanf(pcParam[var]+6, "%"SCNu16"", &blindchannel);
            blindchannel--;
            sscanf(pcValue[var], "%"SCNu32"", &value32);
            //only write to eeprom if value has changed
            if (value32!=blindmovingtimeup[blindchannel])
            {
                blindmovingtimeup[blindchannel] = value32;
                EE_WriteStorage(&eeblindmovingtimeup);
                Dio_SetBlindsMovingTimeUp((uint32_t*) &blindmovingtimeup);
            }
            continue;
        }

        if(strncmp(pcParam[var], theSSItags[timedo1], 6) == 0)
        {
            sscanf(pcParam[var]+6, "%"SCNu16"", &blindchannel);
            blindchannel--;
            sscanf(pcValue[var], "%"SCNu32"", &value32);
            //only write to eeprom if value has changed
            if (value32!=blindmovingtimedown[blindchannel])
            {
                blindmovingtimedown[blindchannel] = value32;
                EE_WriteStorage(&eeblindmovingtimedown);
                Dio_SetBlindsMovingTimeDown((uint32_t*) &blindmovingtimedown);
            }
            continue;
        }
        if(strncmp(pcParam[var], theSSItags[per50_1], 5) == 0)
        {
            sscanf(pcParam[var]+6, "%"SCNu16"", &blindchannel);
            blindchannel--;
            sscanf(pcValue[var], "%2"SCNu16"", &value8);
            if (value8!=blindpos50[blindchannel])
            {
                blindpos50[blindchannel] = value8;
                EE_WriteStorage(&eeblindpos50);
                Dio_SetBlindsPos50((uint8_t*)&blindpos50);
            }
            continue;
        }

        if(strncmp(pcParam[var], theSSItags[rafftim1], 7) == 0)
        {
            sscanf(pcParam[var]+7, "%"SCNu16"", &blindchannel);
            blindchannel--;
            sscanf(pcValue[var], "%"SCNu16"", &value16);
            //only write to eeprom if value has changed
            if (value16!=raffmovingtime[blindchannel])
            {
                raffmovingtime[blindchannel] = value16;
                EE_WriteStorage(&eeraffmovingtime);
                Dio_SetRaffstoreMovingtime((uint16_t*) &raffmovingtime);
            }
            continue;
        }

        if(strncmp(pcParam[var], theSSItags[raff1], 4) == 0)
        {
            sscanf(pcParam[var]+4, "%"SCNu16"", &blindchannel);
            blindchannel--;
            temp_raffstore = true;
            continue;
        }

        if(strncmp(pcParam[var], theSSItags[pfunc1], 5) == 0)
        {
            sscanf(pcParam[var]+5, "%"SCNu16"", &blindchannel); //
            blindchannel--;
            temp_posfunc = true;
            continue;
        }
    }

    //only write to eeprom if value has changed
    if(position_function_active[blindchannel] != temp_posfunc)
    {
        position_function_active[blindchannel] = temp_posfunc;
        EE_WriteStorage(&eeposition_function_active);
        Dio_SetPositionFunction((bool*) &position_function_active);
    }

    //only write to eeprom if value has changed
    if(raffstore[blindchannel] != temp_raffstore)
    {
        raffstore[blindchannel] = temp_raffstore;
        EE_WriteStorage(&eeraffstore);
        Dio_SetRaffstore((bool*) &raffstore);
    }

    //only write to eeprom if value has changed
    if(blindinputmatrix[blindchannel] != temp_blindinput)
    {
        blindinputmatrix[blindchannel] = temp_blindinput;
        EE_WriteStorage(&eeblindinputmatrix);
        Dio_SetBlindInputMatrix((uint16_t*)&blindinputmatrix);
    }

    return "/return.html";
}

const char* webBootloaderCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    setReset();
    printf("Reset\r\n");

    return "/startapp.html";
}

const char* webPositionCGIhandler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    static uint16_t channel = 0;
    uint16_t value = 0;
    for(uint8_t var = 0; var < iNumParams; var++)
    {
        if(strncmp(pcParam[var], theSSItags[per50_1],5) == 0)
        {
            sscanf(pcParam[var]+6, "%"SCNu16"", &channel);
            channel--;
            sscanf(pcValue[var], "%2"SCNu16"", &value);
            blindpos50[channel] = value;
            EE_WriteStorage(&eeblindpos50);
            Dio_SetBlindsPos50((uint8_t*)&blindpos50);
        }
        if(strncmp(pcParam[var], "blind", 5) == 0)
        {
            sscanf(pcValue[var], "%1"SCNu16"", &channel);
            channel--;
        }
    }
    return "/return.html";
}

// the actual function for SSI
uint16_t webSSIHandler(int iIndex, char *pcInsert, int iInsertLen)
{
    char myStr[LWIP_HTTPD_MAX_TAG_INSERT_LEN];
    if((iIndex >= blind1) && (iIndex <= blind8))
    {
        if(webBlinds_pst[iIndex].blinddirection == blinddirection_up)
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
        else if(webBlinds_pst[iIndex].blinddirection == blinddirection_down)
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
        if(webBlinds_pst[iIndex-raff1].angle_function_active == true)
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
        sprintf(myStr, "%ldms", webBlinds_pst[iIndex - pos1].position_actual);
        strcpy(pcInsert, myStr);
        return strlen(myStr);
    }
    if((iIndex >= angle1) && (iIndex <= angle8))
    {
        sprintf(myStr, "%ldms", webBlinds_pst[iIndex - angle1].angle_actual);
        strcpy(pcInsert, myStr);
        return strlen(myStr);
    }
    if(iIndex == mqtttopic)
    {
        char tempTopic[27];
        MqttClient_GetMQTTTopic(tempTopic);
        sprintf(myStr, "<input value=\"%s\" name=\"mqtttopic\" type=\"text\" id=\"mqtttopic\" size=\"25\" maxlength=\"10\">", tempTopic);
        strcpy(pcInsert, myStr);
        return strlen(myStr);
    }
    if(iIndex == mqtthost)
    {
        char host[MQTT_HOST_STORAGE_LEN];
        MqttClient_GetMQTTHostString(host);
        sprintf(myStr, "<input value=\"%s\" name=\"mqtthost\" type=\"text\" id=\"mqtthost\" size=\"25\">", host);
        strcpy(pcInsert, myStr);
        return strlen(myStr);
    }
    if(iIndex == current)
    {
        sprintf(myStr, "<input value=\"%d\" name=\"current\" type=\"text\" id=\"current\" size=\"10\" maxlength=\"4\">",
                Dio_GetBlindcurrentThreshold());
        strcpy(pcInsert, myStr);
        return strlen(myStr);
    }
    if(iIndex == compiled)
    {
        sprintf(myStr, __DATE__ " " __TIME__);
        strcpy(pcInsert, myStr);
        return strlen(myStr);
    }
    if((iIndex >= input1) && (iIndex <= input8))
    {
        uint8_t blind = iIndex - input1 + 1;
        char checked[8];
        char name[10];
        int16_t position = 0;

        for (uint8_t input = 0; input < 9; input++)
        {
            //printf("Checked %d\r\n",(blindinputmatrix[blind-1]>>input)&1);
            if((blindinputmatrix[blind-1]>>input)&1)
            {
                sprintf(checked,"checked");
            }
            else
            {
                sprintf(checked," ");
            }
            sprintf(name,"input%d_%d,",input+1,blind);
            position += sprintf(myStr+position, "<td><input name=\"%s\"type=\"checkbox\" id=\"%s\" %s/></td>",name,name,checked);
        }
        strcpy(pcInsert, myStr);
        return strlen(myStr);
    }
    if((iIndex >= pfunc1) && (iIndex <= pfunc8))
    {
        uint8_t blind = iIndex - pfunc1 + 1;
        char checked[8];
        char name[10];
        if(webBlinds_pst[blind-1].position_function_active)
        {
            sprintf(checked,"checked");
        }
        else
        {
            sprintf(checked," ");
        }
        sprintf(name,"pfunc%d,",blind);
        sprintf(myStr, "<input name=\"%s\" type=\"checkbox\" id=\"%s\" %s/>",name,name,checked);
        strcpy(pcInsert, myStr);
        return strlen(myStr);
    }
    return 0;
}

// function to initialize CGI
void Web_CGIinit(void)
{
    theCGItable[0] = BlindsCGI;
    theCGItable[1] = MqttCGI;
    theCGItable[2] = LearnCGI;
    theCGItable[3] = SettingCGI;
    theCGItable[4] = BootloaderCGI;
    theCGItable[5] = PositionCGI;
    //give the table to the HTTP server
    http_set_cgi_handlers(theCGItable, theCGItableSize);
}

// function to initialize SSI
void Web_SSIinit(void)
{
    webBlinds_pst = Dio_GetBlinds();
    http_set_ssi_handler(webSSIHandler, (char const**) theSSItags, numSSItags);
}