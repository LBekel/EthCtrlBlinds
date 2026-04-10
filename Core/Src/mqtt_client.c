/*
 * mqtt_client.c
 *
 *  Created on: Jan 11, 2021
 *      Author: LBekel
 */
#include "cmsis_os.h"
#include "string.h"
#include "mqtt.h"
#include "mqtt_client.h"
#include <lwip/dhcp.h>
#include <lwip/tcpip.h>

#include "lwip/apps/mqtt_priv.h"
#include <stdio.h>
#include "dio.h"
#include <stdbool.h>
#include <math.h>

#define BLINDDIRCMND "cmnd/%s/blinddir"
#define BLINDPOSCMND "cmnd/%s/blindpos"
#define BLINDANGCMND "cmnd/%s/blindang"
#define INPUTSTAT "stat/%s/input"
#define BLINDDIRSTAT "stat/%s/blinddir"
#define BLINDPOSSTAT "stat/%s/blindpos"
#define BLINDANGSTAT "stat/%s/blindang"
#define CURRENT "stat/%s/current"
#define LWTTELE "tele/%s/LWT"
#define IPTELE "tele/%s/IP"
#define MACTELE "tele/%s/MAC"

#define HA_DISCOVERY_PREFIX "homeassistant"

#define MQTT_PUBLISH_RETRY_MAX 8
#define MQTT_PUBLISH_RETRY_DELAY_MS 150

static mqtt_client_t client;
static ip_addr_t mqtt_server_ip_addr;
static volatile bool mqtt_bootstrap_in_progress = false;
static volatile bool mqtt_bootstrap_pending = false;
static volatile bool mqtt_blinddir_publish_pending = false;
const char *payload_up = "UP";
const char *payload_down = "DOWN";
const char *payload_off = "STOP";
static char mqttname[21];
static uint8_t inpub_id;
static uint8_t channel;
int16_t current;
struct blind_s *mqttBlinds_pst;
struct doubleswitch_s *mqttDoubleswitches_pst;

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);
static void mqtt_sub_request_cb(void *arg, err_t result);
static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len);
static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags);
static void mqtt_pub_request_cb(void *arg, err_t result);
void publish_blinddir_cmds(void);
void subscribe_blinddir_cmd(void);
void subscribe_blindangle_cmd(void);
void publish_blinddir_stats(void);
void publish_blindpos_stats(void);
void subscribe_blindpos_cmd(void);
void publish_blindpos_cmd(struct blind_s *blind);
void publish_blindangle_cmds(void);
void publish_blindangle_cmd(struct blind_s *blind);
void publish_lwt(bool online);
void publish_ha_discovery(void);
static void publish_ha_discovery_cover(struct blind_s *blind);
static void publish_ha_discovery_input(struct doubleswitch_s *ds);
static void publish_ha_discovery_current(void);

static err_t mqtt_publish_with_retry(const char *topic, const void *payload, u16_t payload_length,
                                     u8_t qos, u8_t retain, mqtt_request_cb_t cb, void *arg)
{
    err_t err = ERR_CONN;

    for(uint8_t attempt = 0; attempt < MQTT_PUBLISH_RETRY_MAX; ++attempt)
    {
        LOCK_TCPIP_CORE();
        if(!mqtt_client_is_connected(&client))
        {
            UNLOCK_TCPIP_CORE();
            return ERR_CONN;
        }

        err = mqtt_publish(&client, topic, payload, payload_length, qos, retain, cb, arg);
        UNLOCK_TCPIP_CORE();

        if(err == ERR_OK)
        {
            return ERR_OK;
        }

        if(err != ERR_MEM)
        {
            return err;
        }

        /* Give the TCP/MQTT stack time to flush queued bytes before retrying. */
        osDelay(MQTT_PUBLISH_RETRY_DELAY_MS);
    }

    return err;
}

static err_t mqtt_subscribe_with_retry(const char *topic, u8_t qos, mqtt_request_cb_t cb, void *arg)
{
    err_t err = ERR_CONN;

    for(uint8_t attempt = 0; attempt < MQTT_PUBLISH_RETRY_MAX; ++attempt)
    {
      LOCK_TCPIP_CORE();
      if(!mqtt_client_is_connected(&client))
      {
          UNLOCK_TCPIP_CORE();
          return ERR_CONN;
      }

      err = mqtt_subscribe(&client, topic, qos, cb, arg);
      UNLOCK_TCPIP_CORE();

        if(err == ERR_OK)
        {
            return ERR_OK;
        }

        if(err != ERR_MEM)
        {
            return err;
        }

        osDelay(MQTT_PUBLISH_RETRY_DELAY_MS);
    }

    return err;
}

void mqtt_connect(mqtt_client_t *client)
{
    struct mqtt_connect_client_info_t ci;
    err_t err;

    /* Setup an empty client info structure */
    memset(&ci, 0, sizeof(ci));

    /* Minimal amount of information required is client identifier, so set it here */
    char will_topic[strlen(mqttname) + 10];
    sprintf(will_topic, LWTTELE, mqttname);
    char will_msg[] = "Offline";

    ci.client_id = mqttname;
    ci.keep_alive = 60;
    ci.will_msg = will_msg;
    ci.will_topic = will_topic;
    ci.will_qos = 0;
    ci.will_retain = 1;

    /* Initiate client and connect to server, if this fails immediately an error code is returned
     otherwise mqtt_connection_cb will be called with connection result after attempting
     to establish a connection with the server.
     For now MQTT version 3.1.1 is always used */
    LOCK_TCPIP_CORE();
    err = mqtt_client_connect(client, &mqtt_server_ip_addr, MQTT_PORT, mqtt_connection_cb, NULL, &ci);
    UNLOCK_TCPIP_CORE();

    /* For now just print the result code if something goes wrong*/
    if(err != ERR_OK)
    {
        printf("ERROR: mqtt_client_connect %d\n", err);
    }
}
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
    if(status == MQTT_CONNECT_ACCEPTED)
    {
        printf("INFO: mqtt_connection_cb: Successfully connected\r\n");
        /* Setup callback for incoming publish requests */
        mqtt_set_inpub_callback(client, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, arg);
        /* Defer heavy bootstrap traffic to mqtt task (avoid blocking lwIP callback context). */
        mqtt_bootstrap_pending = true;
    }
    else
    {
        mqtt_bootstrap_in_progress = false;
        mqtt_bootstrap_pending = false;
        printf("ERROR: mqtt_connection_cb: Disconnected, reason: %d\r\n", status);
    }
}
static void mqtt_sub_request_cb(void *arg, err_t result)
{
    /* Just print the result code here for simplicity,
     normal behavior would be to take some action if subscribe fails like
     notifying user, retry subscribe or disconnect from server */
    printf("INFO: Subscribe result: %d\r\n", result);
}
static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{
    /* Decode topic string into a user defined reference */
    inpub_id = inpub_unknown;
    char comparetopic1[sizeof(mqttname) + 17];
    sprintf(comparetopic1, BLINDDIRCMND, mqttname); //build Topic

    if(strncmp(topic, comparetopic1, strlen(comparetopic1)) == 0)
    {
        inpub_id = inpub_blindcmnd;
        uint8_t n = strlen(comparetopic1); //start sscanf after topic
        sscanf(topic + n, "%"PRIu8"", &channel);
        //printf("blindCmd received %d\n\r",channel);
        channel--;
    }

    char comparetopic2[sizeof(mqttname) + 17];
    sprintf(comparetopic2, BLINDPOSCMND, mqttname); //build Topic
    if(strncmp(topic, comparetopic2, strlen(comparetopic2)) == 0)
    {
        inpub_id = inpud_blindposcmnd;
        uint8_t n = strlen(comparetopic2); //start sscanf after topic
        sscanf(topic + n, "%"PRIu8"", &channel);
        channel--;
    }
    char comparetopic3[sizeof(mqttname) + 17];
    sprintf(comparetopic3, BLINDANGCMND, mqttname); //build Topic
    if(strncmp(topic, comparetopic3, strlen(comparetopic3)) == 0)
    {
        inpub_id = inpud_blindangcmnd;
        uint8_t n = strlen(comparetopic3); //start sscanf after topic
        sscanf(topic + n, "%"PRIu8"", &channel);
        channel--;
    }


}
static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
    printf("Incoming publish payload with length %d, flags %u\n", len, (unsigned int) flags);

    if(flags & MQTT_DATA_FLAG_LAST)
    {
        /* Last fragment of payload received (or whole part if payload fits receive buffer
         See MQTT_VAR_HEADER_BUFFER_LEN)  */

        /* Call function or do action depending on reference, in this case inpub_id */
        if(inpub_id == inpub_blindcmnd)
        {
            if(strncmp((const char*) data, payload_off, len) == 0)
            {
                mqttBlinds_pst[channel].blinddirection = blinddirection_off;
                setBlindDirection(&mqttBlinds_pst[channel]);
                mqtt_blinddir_publish_pending = true;
            }
            else if(strncmp((const char*) data, payload_up, len) == 0)
            {
                mqttBlinds_pst[channel].blinddirection = blinddirection_up;
                mqttBlinds_pst[channel].position_target = 0 - 1000;
                mqttBlinds_pst[channel].angle_target = 0;
                if(mqttBlinds_pst[channel].position_function_active == false)
                {
                    mqttBlinds_pst[channel].position_actual = mqttBlinds_pst[channel].position_movingtimeup;
                }
                setBlindDirection(&mqttBlinds_pst[channel]);
                mqtt_blinddir_publish_pending = true;
            }
            else if(strncmp((const char*) data, payload_down, len) == 0)
            {
                mqttBlinds_pst[channel].blinddirection = blinddirection_down;
                mqttBlinds_pst[channel].position_target = mqttBlinds_pst[channel].position_movingtimeup + 1000;
                mqttBlinds_pst[channel].angle_target = mqttBlinds_pst[channel].angle_movingtime;
                if(mqttBlinds_pst[channel].position_function_active == false)
                {
                    mqttBlinds_pst[channel].position_actual = 0;
                }
                setBlindDirection(&mqttBlinds_pst[channel]);
                mqtt_blinddir_publish_pending = true;
            }
            else
            {
                uint8_t percent = 0;
                char fmt_str[16] = "";
                snprintf(fmt_str, 16, "%%%dPRIu8", len);
                if(sscanf((const char *)data, fmt_str, &percent)!=EOF)
                {

                	if(percent>=100)
                	{
                		mqttBlinds_pst[channel].position_target = mqttBlinds_pst[channel].position_movingtimeup + 1000;
                        if(mqttBlinds_pst[channel].position_function_active == false)
                        {
                            mqttBlinds_pst[channel].position_actual = mqttBlinds_pst[channel].position_movingtimeup;
                        }
                	}
                	else if(percent<=0)
                	{
                		mqttBlinds_pst[channel].position_target = 0 - 1000;
                        if(mqttBlinds_pst[channel].position_function_active == false)
                        {
                            mqttBlinds_pst[channel].position_actual = 0;
                        }
                	}
                	else
                	{
                	    calc_position(percent,&mqttBlinds_pst[channel]);
                	}


                    if(mqttBlinds_pst[channel].position_actual > mqttBlinds_pst[channel].position_target)
                    {
                        mqttBlinds_pst[channel].blinddirection = blinddirection_up;
                        if(mqttBlinds_pst[channel].position_function_active == false)
                        {
                            mqttBlinds_pst[channel].position_actual = mqttBlinds_pst[channel].position_movingtimeup;
                        }
                        mqttBlinds_pst[channel].angle_target = 0;
                    }
                    else
                    {
                        mqttBlinds_pst[channel].blinddirection = blinddirection_down;
                        if(mqttBlinds_pst[channel].position_function_active == false)
                        {
                            mqttBlinds_pst[channel].position_actual = 0;
                        }
                        mqttBlinds_pst[channel].angle_target = mqttBlinds_pst[channel].angle_movingtime;
                    }
                    setBlindDirection(&mqttBlinds_pst[channel]);
                    mqtt_blinddir_publish_pending = true;
                }
            }
        }
        else if(inpub_id == inpud_blindangcmnd)
        {

            uint8_t percent = 0;
            sscanf((const char *)data, "%"PRIu8"", &percent);
            if(percent>=100)
            {
                percent = 100;
            }
            else if(percent<=0)
            {
                percent = 0;
            }

            mqttBlinds_pst[channel].angle_target = (double)mqttBlinds_pst[channel].angle_movingtime/(double)100*percent;
            //start only moving if mqttBlinds_pst are stopped
            if(mqttBlinds_pst[channel].blinddirection == blinddirection_off)
            {
                if(mqttBlinds_pst[channel].angle_actual < mqttBlinds_pst[channel].angle_target)
                {
                    mqttBlinds_pst[channel].blinddirection = blinddirection_angle_down;
                }
                else if(mqttBlinds_pst[channel].angle_actual > mqttBlinds_pst[channel].angle_target)
                {
                    mqttBlinds_pst[channel].blinddirection = blinddirection_angle_up;
                }
                else
                {
                    mqttBlinds_pst[channel].blinddirection = blinddirection_off;
                }
                setBlindDirection(&mqttBlinds_pst[channel]);
            }
        }
        else
        {
            printf("ERROR: mqtt_incoming_data_cb: Ignoring payload\n");
        }
    }
    else
    {
        /* Handle fragmented payload, store in buffer, write to file or whatever */
        printf("ERROR: Fragmented\n\r");
    }
}
void publish_doubleswitch_stats(void)
{
    if(mqtt_client_is_connected(&client))
    {
        for(uint8_t var = 0; var < num_doubleswitches; ++var)
        {
            publish_doubleswitch_stat(&mqttDoubleswitches_pst[var]);
        }
    }
}
void publish_doubleswitch_stat(struct doubleswitch_s *doubleswitch)
{
    if(mqtt_client_is_connected(&client))
    {
        err_t err = ERR_OK;
        u8_t qos = 0; /* 0 1 or 2, see MQTT specification */
        u8_t retain = 0;

        char str[sizeof(mqttname) + 14];
        sprintf(str, INPUTSTAT"%02d", mqttname, doubleswitch->channel); //build Topic

        switch(doubleswitch->inputdirection)
        {
            case inputdirection_off:
                err = mqtt_publish_with_retry(str, payload_off, (u16_t)strlen(payload_off), qos, retain, mqtt_pub_request_cb,
                        NULL);
                break;
            case inputdirection_up:
                err = mqtt_publish_with_retry(str, payload_up, (u16_t)strlen(payload_up), qos, retain, mqtt_pub_request_cb,
                        NULL);
                break;
            case inputdirection_down:
                err = mqtt_publish_with_retry(str, payload_down, (u16_t)strlen(payload_down), qos, retain, mqtt_pub_request_cb,
                        NULL);
                break;
            default:
                break;
        }
        if(err != ERR_OK)
            printf("ERROR: publish_doubleswitch_stat %d\n", err);
    }
}
void publish_blinddir_stats(void)
{
    if(mqtt_client_is_connected(&client))
    {
        for(uint8_t var = 0; var < num_blinds; ++var)
        {
            publish_blinddir_stat(&mqttBlinds_pst[var]);
        }
    }
}
void publish_blinddir_stat(struct blind_s *blind)
{
    err_t err = ERR_OK;
    u8_t qos = 0; /* 0 1 or 2, see MQTT specification */
    u8_t retain = 0;
    if(mqtt_client_is_connected(&client))
    {

        char topic[sizeof(mqttname) + 17];
        sprintf(topic, BLINDDIRSTAT"%02d", mqttname, blind->channel); //build Topic
        switch(blind->blinddirection)
        {
            case blinddirection_up:
                err = mqtt_publish_with_retry(topic, payload_up, (u16_t)strlen(payload_up), qos, retain, mqtt_pub_request_cb,
                        NULL);
                break;
            case blinddirection_down:
                err = mqtt_publish_with_retry(topic, payload_down, (u16_t)strlen(payload_down), qos, retain, mqtt_pub_request_cb,
                        NULL);
                break;
            case blinddirection_off:
                err = mqtt_publish_with_retry(topic, payload_off, (u16_t)strlen(payload_off), qos, retain, mqtt_pub_request_cb,
                        NULL);
                break;
            default:
                break;
        }
        if(err != ERR_OK)
            printf("ERROR: publish_blinddir_stat: %d\r\n", err);
    }
}
void publish_blindpos_stats(void)
{
    if(mqtt_client_is_connected(&client))
    {
        for(uint8_t var = 0; var < num_blinds; ++var)
        {
            publish_blindpos_stat(&mqttBlinds_pst[var]);
        }
    }
}
void publish_blindpos_stat(struct blind_s *blind)
{
    err_t err = ERR_OK;
    u8_t qos = 0; /* 0 1 or 2, see MQTT specification */
    u8_t retain = 0;
    if(mqtt_client_is_connected(&client))
    {
        if(blind->position_changed)
        {
            char topic[sizeof(mqttname) + 19];
            sprintf(topic, BLINDPOSSTAT"%02d", mqttname, blind->channel); //build Topic
            char payload[6];
//            double percent;
//            percent = round((double) 100.0 / blind->position_movingtimeup * blind->position_actual);
//        	if(percent>=100)
//        	{
//        		percent = 100;
//        	}
//        	else if(percent<=0)
//        	{
//        		percent = 0;
//        	}
            sprintf(payload, "%d", (uint8_t) calc_real_position(blind));
            err = mqtt_publish_with_retry(topic, payload, (u16_t)strlen(payload), qos, retain, mqtt_pub_request_cb, NULL);

            if(err != ERR_OK)
                printf("ERROR: publish_blindpos_stat: %d\r\n", err);
        }
        blind->position_changed = false;
        //osDelay(10);
    }
}
void publish_blindangle_stats(void)
{
    if(mqtt_client_is_connected(&client))
    {
        for(uint8_t var = 0; var < num_blinds; ++var)
        {
            publish_blindangle_stat(&mqttBlinds_pst[var]);
        }
    }
}
void publish_blindangle_stat(struct blind_s *blind)
{
    err_t err = ERR_OK;
    u8_t qos = 0; /* 0 1 or 2, see MQTT specification */
    u8_t retain = 0;
    if(mqtt_client_is_connected(&client))
    {
        if(blind->angle_changed)
        {
            char topic[sizeof(mqttname) + 19];
            sprintf(topic, BLINDANGSTAT"%02d", mqttname, blind->channel); //build Topic
            char payload[6];
            double percent = round((double) 100.0 / blind->angle_movingtime * blind->angle_actual);
            sprintf(payload, "%d", (uint8_t) percent);
            err = mqtt_publish_with_retry(topic, payload, (u16_t)strlen(payload), qos, retain, mqtt_pub_request_cb, NULL);

            if(err != ERR_OK)
                printf("ERROR: publish_blindangle_stat: %d\r\n", err);
        }
        blind->angle_changed = false;
        //osDelay(10);
    }
}

void publish_current(void)
{
    err_t err = ERR_OK;
    u8_t qos = 0; /* 0 1 or 2, see MQTT specification */
    u8_t retain = 0;
    if(mqtt_client_is_connected(&client))
    {
        char topic[sizeof(mqttname) + 19];
        sprintf(topic, CURRENT, mqttname); //build Topic
        char payload[7];
        sprintf(payload, "%d", current);
        err = mqtt_publish_with_retry(topic, payload, (u16_t)strlen(payload), qos, retain, mqtt_pub_request_cb, NULL);

        if(err != ERR_OK)
            printf("ERROR: publish_current: %d\r\n", err);

    }
}

void publish_blinddir_cmds(void)
{
    if(mqtt_client_is_connected(&client))
    {
        for(uint8_t var = 0; var < num_blinds; ++var)
        {
            publish_blinddir_cmd(&mqttBlinds_pst[var]);
        }
    }
}
void publish_blinddir_cmd(struct blind_s *blind)
{
    err_t err = ERR_OK;
    u8_t qos = 0; /* 0 1 or 2, see MQTT specification */
    u8_t retain = 0;
    if(mqtt_client_is_connected(&client))
    {
        char topic[sizeof(mqttname) + 17];
        sprintf(topic, BLINDDIRCMND"%02d", mqttname, blind->channel); //build Topic
        switch(blind->blinddirection)
        {
            case blinddirection_up:
                err = mqtt_publish_with_retry(topic, payload_up, (u16_t)strlen(payload_up), qos, retain, mqtt_pub_request_cb,
                        NULL);
                break;
            case blinddirection_down:
                err = mqtt_publish_with_retry(topic, payload_down, (u16_t)strlen(payload_down), qos, retain, mqtt_pub_request_cb,
                        NULL);
                break;
            case blinddirection_off:
                err = mqtt_publish_with_retry(topic, payload_off, (u16_t)strlen(payload_off), qos, retain, mqtt_pub_request_cb,
                        NULL);
                break;
            default:
                break;
        }
        if(err != ERR_OK)
            printf("ERROR: publish_blinddir_cmd: %d\r\n", err);

    }
}

void publish_blindangle_cmds(void)
{
    if(mqtt_client_is_connected(&client))
    {
        for(uint8_t var = 0; var < num_blinds; ++var)
        {
            publish_blindangle_cmd(&mqttBlinds_pst[var]);
        }
    }
}

void publish_blindangle_cmd(struct blind_s *blind)
{
    err_t err = ERR_OK;
    u8_t qos = 0; /* 0 1 or 2, see MQTT specification */
    u8_t retain = 0;
    if(mqtt_client_is_connected(&client))
    {
        char topic[sizeof(mqttname) + 18];
        sprintf(topic, BLINDANGCMND"%02d", mqttname, blind->channel); //build Topic
        char payload[2];
        sprintf(payload, "%d", 0);
        err = mqtt_publish_with_retry(topic, payload, (u16_t)strlen(payload), qos, retain, mqtt_pub_request_cb, NULL);
        if(err != ERR_OK)
            printf("ERROR: publish_blindangle_cmd: %d\r\n", err);

    }
}
void publish_ip_mac(void)
{
    err_t err = ERR_OK;
    u8_t qos = 0; /* 0 1 or 2, see MQTT specification */
    u8_t retain = 0;
    if(mqtt_client_is_connected(&client))
    {
        char topic[strlen(mqttname) + 10];
        sprintf(topic, IPTELE, mqttname);
        char ip[16];
        sprintf(ip, "%s", ipaddr_ntoa(&netif_default->ip_addr));
        err = mqtt_publish_with_retry(topic, ip, (u16_t)strlen(ip), qos, retain, NULL, NULL);
        if(err != ERR_OK)
            printf("ERROR: publish_ip %d\r\n", err);

        sprintf(topic, MACTELE, mqttname);
        char mac[13];
        sprintf(mac, "%02x%02x%02x%02x%02x%02x", netif_default->hwaddr[0], netif_default->hwaddr[1], netif_default->hwaddr[2], netif_default->hwaddr[3],
                netif_default->hwaddr[4], netif_default->hwaddr[5]);

        err = mqtt_publish_with_retry(topic, mac, (u16_t)strlen(mac), qos, retain, NULL, NULL);

        if(err != ERR_OK)
            printf("ERROR: publish_mac %d\r\n", err);

    }
}
void publish_lwt(bool online)
{
    if(mqtt_client_is_connected(&client))
    {
        err_t err = ERR_OK;
        u8_t qos = 0; /* 0 1 or 2, see MQTT specification */

        char topic[sizeof(mqttname) + 10];
        sprintf(topic, LWTTELE, mqttname); //build Topic

        if(online)
        {
            char payload[] = "Online";
            err = mqtt_publish_with_retry(topic, payload, (u16_t)strlen(payload), qos, 1, mqtt_pub_request_cb, NULL);
        }
        else
        {
            char payload[] = "Offline";
            err = mqtt_publish_with_retry(topic, payload, (u16_t)strlen(payload), qos, 1, mqtt_pub_request_cb, NULL);
        }
        if(err != ERR_OK)
            printf("ERROR: publish_lwt %d\n", err);
    }
}
void subscribe_blinddir_cmd(void)
{
    err_t err;
    for(uint8_t var = 0; var < num_blinds; ++var)
    {
        char topic[sizeof(mqttname) + 17];
        sprintf(topic, BLINDDIRCMND"%02d", mqttname, mqttBlinds_pst[var].channel); //build Topic
        err = mqtt_subscribe_with_retry(topic, 1, mqtt_sub_request_cb, NULL);
        if(err != ERR_OK)
            printf("ERROR: subscribe_blinddir_cmd ch%d: %d\r\n", var+1, err);
        osDelay(10);
    }
}
void subscribe_blindpos_cmd(void)
{
    err_t err;
    for(uint8_t var = 0; var < num_blinds; ++var)
    {
        char topic[sizeof(mqttname) + 17];
        sprintf(topic, BLINDPOSCMND"%02d", mqttname, mqttBlinds_pst[var].channel); //build Topic
        err = mqtt_subscribe_with_retry(topic, 1, mqtt_sub_request_cb, NULL);
        if(err != ERR_OK)
            printf("ERROR: subscribe_blindpos_cmd ch%d: %d\r\n", var+1,  err);
        osDelay(10);
    }
}
void subscribe_blindangle_cmd(void)
{
    err_t err;
    for(uint8_t var = 0; var < num_blinds; ++var)
    {
        char topic[sizeof(mqttname) + 17];
        sprintf(topic, BLINDANGCMND"%02d", mqttname, mqttBlinds_pst[var].channel); //build Topic
        err = mqtt_subscribe_with_retry(topic, 1, mqtt_sub_request_cb, NULL);
        if(err != ERR_OK)
            printf("ERROR: subscribe_blindangle_cmd ch%d: %d\r\n", var+1,  err);
        osDelay(10);
    }
}
/* Called when publish is complete either with success or failure */
static void mqtt_pub_request_cb(void *arg, err_t result)
{
    if(result != ERR_OK)
    {
        printf("ERROR: Publish result: %d\r\n", result);
    }
}
void StartmqttTask(void *argument)
{
    printf("StartmqttTask\r\n");

    mqttBlinds_pst = Dio_GetBlinds();
    mqttDoubleswitches_pst = Dio_GetDoubleswitches();
    /* Infinite loop */
    for(;;)
    {
        if(netif_default->ip_addr.addr != 0) //we need a IP Address to connect
        {

            if(mqtt_client_is_connected(&client)) /* while connected, publish */
            {
                if(mqtt_bootstrap_pending)
                {
                    mqtt_bootstrap_pending = false;
                    mqtt_bootstrap_in_progress = true;

                    publish_ip_mac();
                    publish_lwt(true);
                    publish_ha_discovery();
                    publish_blinddir_cmds();
                    subscribe_blinddir_cmd();
                    publish_blinddir_stats();
                    subscribe_blindpos_cmd();
                    publish_blindangle_cmds();
                    subscribe_blindangle_cmd();
                    publish_doubleswitch_stats();

                    mqtt_bootstrap_in_progress = false;
                    printf("INFO: mqtt bootstrap complete\r\n");
                    osDelay(250);
                    continue;
                }
                if(mqtt_bootstrap_in_progress)
                {
                    osDelay(250);
                    continue;
                }
                if(mqtt_blinddir_publish_pending)
                {
                    mqtt_blinddir_publish_pending = false;
                    publish_blinddir_stats();
                }
                publish_blindpos_stats();
                publish_blindangle_stats();
                publish_current();
                osDelay(1000);
            }
            else
            {
                mqtt_connect(&client);
                osDelay(1000);
            }
        }
        else
        {
            osDelay(1000);
        }
    }
}
void getMQTTTopic(char *topic)
{
    strcpy(topic, mqttname);
}
void setMQTTTopic(char *topic)
{
    strcpy(mqttname, topic);
}
void getMQTTHost(ip_addr_t *mqtt_host_addr)
{
    *mqtt_host_addr = mqtt_server_ip_addr;
}
void setMQTTHost(ip_addr_t *mqtt_host_addr)
{
    printf("set MQTT Host Address: %s\r\n", ipaddr_ntoa(mqtt_host_addr));
    mqtt_server_ip_addr = *mqtt_host_addr;
    LOCK_TCPIP_CORE();
    mqtt_disconnect(&client); //disconnect to force new connect
    UNLOCK_TCPIP_CORE();
}
void setMQTTCurrent(int16_t _current)
{
    current = _current;
}

/* -----------------------------------------------------------------------
 * Home Assistant MQTT Discovery
 * Publishes retain=1 config messages to homeassistant/<type>/<id>/config
 * so that HA auto-discovers all entities.
 * ----------------------------------------------------------------------- */

static void publish_ha_discovery_cover(struct blind_s *blind)
{
    if(!mqtt_client_is_connected(&client))
        return;

    /* Static buffers to avoid stack overflow (task stack = 2048 bytes) */
    static char config_topic[60 + 21];
    static char payload[1400];

    int topic_len = snprintf(config_topic, sizeof(config_topic),
                             "%s/cover/%s_blind_%02d/config",
                             HA_DISCOVERY_PREFIX, mqttname, blind->channel);
    if(topic_len < 0 || topic_len >= (int)sizeof(config_topic))
    {
        printf("ERROR: HA discovery cover topic truncated ch%d\r\n", blind->channel);
        return;
    }

    int payload_len = snprintf(payload, sizeof(payload),
             "{"
             "\"name\":\"Blind %02d\","
             "\"unique_id\":\"%s_blind_%02d\","
             "\"command_topic\":\"cmnd/%s/blinddir%02d\","
             "\"state_topic\":\"stat/%s/blinddir%02d\","
             "\"position_topic\":\"stat/%s/blindpos%02d\","
             "\"set_position_topic\":\"cmnd/%s/blindpos%02d\","
             "\"tilt_command_topic\":\"cmnd/%s/blindang%02d\","
             "\"tilt_status_topic\":\"stat/%s/blindang%02d\","
             "\"availability_topic\":\"tele/%s/LWT\","
             "\"payload_available\":\"Online\","
             "\"payload_not_available\":\"Offline\","
             "\"payload_open\":\"UP\","
             "\"payload_close\":\"DOWN\","
             "\"payload_stop\":\"STOP\","
             "\"state_opening\":\"UP\","
             "\"state_closing\":\"DOWN\","
             "\"state_stopped\":\"STOP\","
             "\"position_open\":0,"
             "\"position_closed\":100,"
             "\"tilt_min\":0,"
             "\"tilt_max\":100,"
             "\"device\":{"
             "\"identifiers\":[\"%s\"],"
             "\"name\":\"%s\","
             "\"model\":\"EthCtrlBlinds\","
             "\"manufacturer\":\"Custom\""
             "}"
             "}",
             blind->channel,
             mqttname, blind->channel,
             mqttname, blind->channel,
             mqttname, blind->channel,
             mqttname, blind->channel,
             mqttname, blind->channel,
             mqttname, blind->channel,
             mqttname, blind->channel,
             mqttname,
             mqttname,
             mqttname);
    if(payload_len < 0 || payload_len >= (int)sizeof(payload))
    {
        printf("ERROR: HA discovery cover payload truncated ch%d\r\n", blind->channel);
        return;
    }

    printf("INFO: HA discovery publish %s (%d bytes)\r\n", config_topic, payload_len);
    err_t err = mqtt_publish_with_retry(config_topic, payload, (u16_t)payload_len,
                                        0 /* qos */, 1 /* retain */, mqtt_pub_request_cb, NULL);
    if(err != ERR_OK)
        printf("ERROR: publish_ha_discovery_cover ch%d: %d\r\n", blind->channel, err);
}

static void publish_ha_discovery_input(struct doubleswitch_s *ds)
{
    if(!mqtt_client_is_connected(&client))
        return;

    static char config_topic[65 + 21];
    static char payload[640];

    int topic_len = snprintf(config_topic, sizeof(config_topic),
                             "%s/binary_sensor/%s_input_%02d/config",
                             HA_DISCOVERY_PREFIX, mqttname, ds->channel);
    if(topic_len < 0 || topic_len >= (int)sizeof(config_topic))
    {
        printf("ERROR: HA discovery input topic truncated ch%d\r\n", ds->channel);
        return;
    }

    int payload_len = snprintf(payload, sizeof(payload),
             "{"
             "\"name\":\"Input %02d\","
             "\"unique_id\":\"%s_input_%02d\","
             "\"state_topic\":\"stat/%s/input%02d\","
             "\"payload_on\":\"UP\","
             "\"payload_off\":\"STOP\","
             "\"availability_topic\":\"tele/%s/LWT\","
             "\"payload_available\":\"Online\","
             "\"payload_not_available\":\"Offline\","
             "\"device\":{"
             "\"identifiers\":[\"%s\"],"
             "\"name\":\"%s\","
             "\"model\":\"EthCtrlBlinds\","
             "\"manufacturer\":\"Custom\""
             "}"
             "}",
             ds->channel,
             mqttname, ds->channel,
             mqttname, ds->channel,
             mqttname,
             mqttname,
             mqttname);
    if(payload_len < 0 || payload_len >= (int)sizeof(payload))
    {
        printf("ERROR: HA discovery input payload truncated ch%d\r\n", ds->channel);
        return;
    }

    printf("INFO: HA discovery publish %s (%d bytes)\r\n", config_topic, payload_len);
    err_t err = mqtt_publish_with_retry(config_topic, payload, (u16_t)payload_len,
                                        0 /* qos */, 1 /* retain */, mqtt_pub_request_cb, NULL);
    if(err != ERR_OK)
        printf("ERROR: publish_ha_discovery_input ch%d: %d\r\n", ds->channel, err);
}

static void publish_ha_discovery_current(void)
{
    if(!mqtt_client_is_connected(&client))
        return;

    static char config_topic[50 + 21];
    static char payload[560];

    int topic_len = snprintf(config_topic, sizeof(config_topic),
                             "%s/sensor/%s_current/config",
                             HA_DISCOVERY_PREFIX, mqttname);
    if(topic_len < 0 || topic_len >= (int)sizeof(config_topic))
    {
        printf("ERROR: HA discovery current topic truncated\r\n");
        return;
    }

    int payload_len = snprintf(payload, sizeof(payload),
             "{"
             "\"name\":\"Current\","
             "\"unique_id\":\"%s_current\","
             "\"state_topic\":\"stat/%s/current\","
             "\"unit_of_measurement\":\"mA\","
             "\"availability_topic\":\"tele/%s/LWT\","
             "\"payload_available\":\"Online\","
             "\"payload_not_available\":\"Offline\","
             "\"device\":{"
             "\"identifiers\":[\"%s\"],"
             "\"name\":\"%s\","
             "\"model\":\"EthCtrlBlinds\","
             "\"manufacturer\":\"Custom\""
             "}"
             "}",
             mqttname,
             mqttname,
             mqttname,
             mqttname,
             mqttname);
    if(payload_len < 0 || payload_len >= (int)sizeof(payload))
    {
        printf("ERROR: HA discovery current payload truncated\r\n");
        return;
    }

    printf("INFO: HA discovery publish %s (%d bytes)\r\n", config_topic, payload_len);
    err_t err = mqtt_publish_with_retry(config_topic, payload, (u16_t)payload_len,
                                        0 /* qos */, 1 /* retain */, mqtt_pub_request_cb, NULL);
    if(err != ERR_OK)
        printf("ERROR: publish_ha_discovery_current: %d\r\n", err);
}

void publish_ha_discovery(void)
{
    printf("INFO: Publishing Home Assistant MQTT discovery messages\r\n");

    for(uint8_t var = 0; var < num_blinds; ++var)
    {
        publish_ha_discovery_cover(&mqttBlinds_pst[var]);
        osDelay(500); /* wait for ring buffer to flush before next large payload */
    }

    // for(uint8_t var = 0; var < num_doubleswitches; ++var)
    // {
    //     publish_ha_discovery_input(&mqttDoubleswitches_pst[var]);
    //     osDelay(500);
    // }
    //
    // publish_ha_discovery_current();
    osDelay(500);

    printf("INFO: Home Assistant MQTT discovery complete\r\n");
}

