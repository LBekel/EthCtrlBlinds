/*
 *  Created on: Jan 11, 2021
 *      Author: LBekel
 */
#include "cmsis_os.h"
#include "string.h"
#include "mqtt.h"
#include "MqttClient.h"
#include <lwip/dhcp.h>
#include <lwip/tcpip.h>
#include <lwip/dns.h>

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

#define MODEL "EthCtrlBlinds"
#define MANUFACTURER "LarsBekel"
#define HA_DISCOVERY_PREFIX "homeassistant"

#define MQTT_PUBLISH_RETRY_MAX 8
#define MQTT_PUBLISH_RETRY_DELAY_MS 150

#define MQTT_PUBLISH_QUEUE_LENGTH 24
#define MQTT_PUBLISH_MAX_TOPIC_LEN 96
#define MQTT_PUBLISH_MAX_PAYLOAD_LEN 256
#define MQTT_PUBLISH_QUEUE_TIMEOUT_MS 20
#define MQTT_BLINDS_REFRESH_INTERVAL_MS 60000U

#define MQTT_CLIENT_ADDITIONAL_MOVE_TIME_MS 5000U
#define MQTT_HOST_MAX_LEN 32U

typedef struct
{
    char topic[MQTT_PUBLISH_MAX_TOPIC_LEN + 1];
    uint8_t payload[MQTT_PUBLISH_MAX_PAYLOAD_LEN];
    u16_t payload_length;
    u8_t qos;
    u8_t retain;
    mqtt_request_cb_t cb;
    void *arg;
} mqtt_publish_queue_item_t;

static osMessageQueueId_t mqtt_publish_queue_id = NULL;
static osThreadId_t mqtt_task_thread_id = NULL;
static volatile bool mqtt_publish_queue_overflow_logged = false;

static mqtt_client_t client;
static ip_addr_t mqtt_server_ip_addr;
static char mqtt_server_host[MQTT_HOST_MAX_LEN] = "192.168.1.3";
static bool mqtt_server_host_is_dns = false;
static volatile bool mqtt_dns_query_pending = false;
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

static err_t mqttClientPublishWithRetry(const char *topic, const void *payload, u16_t payload_length,
                                     u8_t qos, u8_t retain, mqtt_request_cb_t cb, void *arg);
static err_t mqttClientSubscribeWithRetry(const char *topic, u8_t qos, mqtt_request_cb_t cb, void *arg);
static void mqttClientConnectionCallback(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);
static void mqttClientSubRequestCallback(void *arg, err_t result);
static void mqttClientIncomingPublishCallback(void *arg, const char *topic, u32_t tot_len);
static void mqttClientIncomingDataCallback(void *arg, const u8_t *data, u16_t len, u8_t flags);
static void mqttClientPublishRequestCallback(void *arg, err_t result);
static err_t mqttClientPublishDispatch(const char *topic, const void *payload, u16_t payload_length,
                                   u8_t qos, u8_t retain, mqtt_request_cb_t cb, void *arg);
static bool mqttClientIsMqttTaskContext(void);
static void mqttClientPublishQueueInit(void);
static void mqttClientDrainPublishQueue(void);
static void mqttClientConnect(mqtt_client_t *client);
static void mqttClientDnsFoundCallback(const char *name, const ip_addr_t *ipaddr, void *callback_arg);
static void mqttClientPublishBlinddirCmds(void);
static void mqttClientSubscribeBlinddirCmd(void);
static void mqttClientSubscribeBlindAngleCmd(void);
static void mqttClientPublishBlindDirStats(void);
static void mqttClientPublishBlindPosStats(void);
static void mqttClientPublishDoubleswitchStats(void);
static void mqttClientSubscribeBlindPosCmd(void);
static void mqttClientPublishBlindAngleCmds(void);
static void mqttClientPublishLwt(bool online);
static void mqttClientPublishHaDiscoveryCover(struct blind_s *blind);
static void mqttClientPublishHaDiscoveryInput(struct doubleswitch_s *ds);
uint8_t mqttClientCalcRealBlindPosition(struct blind_s *blind);
void mqttClientCalcBlindPosition(uint8_t percent, struct blind_s *blind);
void mqttClientPublishBlindAngleStats(void);
void mqttClientPublishBlindAngleStat(struct blind_s *blind);
void mqttClientPublishBlindAngleCmd(struct blind_s *blind);
void mqttClientPublishBlindPosStat(struct blind_s *blind);
void mqttClientPublishIpMac(void);
void mqttClientPublishHaDiscovery(void);


/**
 * @brief mqtt publish with retry.
 * @param topic TODO.
 * @param payload TODO.
 * @param payload_length TODO.
 * @param qos TODO.
 * @param retain TODO.
 * @param cb TODO.
 * @param arg TODO.
 * @return TODO.
 */
static err_t mqttClientPublishWithRetry(const char *topic, const void *payload, u16_t payload_length,
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

/**
 * @brief mqtt subscribe with retry.
 * @param topic TODO.
 * @param qos TODO.
 * @param cb TODO.
 * @param arg TODO.
 * @return TODO.
 */
static err_t mqttClientSubscribeWithRetry(const char *topic, u8_t qos, mqtt_request_cb_t cb, void *arg)
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

/**
 * @brief mqtt is mqtt task context.
 * @return TODO.
 */
static bool mqttClientIsMqttTaskContext(void)
{
    return (mqtt_task_thread_id != NULL) && (osThreadGetId() == mqtt_task_thread_id);
}

/**
 * @brief mqtt publish queue init.
 * @return TODO.
 */
static void mqttClientPublishQueueInit(void)
{
    if(mqtt_publish_queue_id != NULL)
    {
        return;
    }

    mqtt_publish_queue_id = osMessageQueueNew(MQTT_PUBLISH_QUEUE_LENGTH,
                                              sizeof(mqtt_publish_queue_item_t),
                                              NULL);
    if(mqtt_publish_queue_id == NULL)
    {
        printf("ERROR: mqtt_publish_queue_init failed\r\n");
    }
}


static err_t mqttClientPublishDispatch(const char *topic, const void *payload, u16_t payload_length,
                                   u8_t qos, u8_t retain, mqtt_request_cb_t cb, void *arg)
{
    if(mqttClientIsMqttTaskContext() || (mqtt_publish_queue_id == NULL))
    {
        return mqttClientPublishWithRetry(topic, payload, payload_length, qos, retain, cb, arg);
    }

    size_t topic_len = strlen(topic);
    if(topic_len > MQTT_PUBLISH_MAX_TOPIC_LEN)
    {
        printf("ERROR: mqtt_publish_dispatch topic too long\r\n");
        return ERR_ARG;
    }

    if(payload_length > MQTT_PUBLISH_MAX_PAYLOAD_LEN)
    {
        printf("ERROR: mqtt_publish_dispatch payload too large (%u)\r\n", payload_length);
        return ERR_ARG;
    }

    mqtt_publish_queue_item_t item = {0};
    memcpy(item.topic, topic, topic_len);
    item.topic[topic_len] = '\0';

    if((payload != NULL) && (payload_length > 0))
    {
        memcpy(item.payload, payload, payload_length);
    }

    item.payload_length = payload_length;
    item.qos = qos;
    item.retain = retain;
    item.cb = cb;
    item.arg = arg;

    osStatus_t status = osMessageQueuePut(mqtt_publish_queue_id, &item, 0U, MQTT_PUBLISH_QUEUE_TIMEOUT_MS);
    if(status == osOK)
    {
        mqtt_publish_queue_overflow_logged = false;
        return ERR_OK;
    }

    if((status == osErrorTimeout) || (status == osErrorResource))
    {
        if(!mqtt_publish_queue_overflow_logged)
        {
            mqtt_publish_queue_overflow_logged = true;
            printf("ERROR: MQTT publish queue full\r\n");
        }
        return ERR_MEM;
    }

    printf("ERROR: mqtt_publish_dispatch queue put failed (%d)\r\n", (int)status);
    return ERR_IF;
}

/**
 * @brief
 * @return TODO.
 */
static void mqttClientDrainPublishQueue(void)
{
    if((mqtt_publish_queue_id == NULL) || !mqtt_client_is_connected(&client))
    {
        return;
    }

    mqtt_publish_queue_item_t item;
    while(osMessageQueueGet(mqtt_publish_queue_id, &item, NULL, 0U) == osOK)
    {
        err_t err = mqttClientPublishWithRetry(item.topic,
                                            item.payload,
                                            item.payload_length,
                                            item.qos,
                                            item.retain,
                                            item.cb,
                                            item.arg);
        if(err != ERR_OK)
        {
            printf("ERROR: mqtt_drain_publish_queue: %d\r\n", err);
            if(err == ERR_CONN)
            {
                break;
            }
        }
    }
}
/**
 * @brief mqtt connect.
 * @param client Parameter client.
 */
static void mqttClientConnect(mqtt_client_t *client)
{
    struct mqtt_connect_client_info_t ci;
    err_t err;

    if(mqtt_server_host_is_dns)
    {
        if(mqtt_dns_query_pending)
        {
            return;
        }

        int dns_err = dns_gethostbyname(mqtt_server_host, &mqtt_server_ip_addr, mqttClientDnsFoundCallback, NULL);
        if(dns_err == ERR_INPROGRESS)
        {
            mqtt_dns_query_pending = true;
            printf("INFO: mqtt dns resolve started: %s\r\n", mqtt_server_host);
            return;
        }
        if(dns_err != ERR_OK)
        {
            printf("ERROR: mqtt dns resolve failed for %s (%d)\r\n", mqtt_server_host, dns_err);
            return;
        }

        printf("INFO: mqtt dns resolved immediately: %s -> %s\r\n", mqtt_server_host, ipaddr_ntoa(&mqtt_server_ip_addr));
    }

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
    err = mqtt_client_connect(client, &mqtt_server_ip_addr, MQTT_PORT, mqttClientConnectionCallback, NULL, &ci);
    UNLOCK_TCPIP_CORE();

    /* For now just print the result code if something goes wrong*/
    if(err != ERR_OK)
    {
        printf("ERROR: mqtt_client_connect %d\n", err);
    }
}

static void mqttClientDnsFoundCallback(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
    LWIP_UNUSED_ARG(callback_arg);
    mqtt_dns_query_pending = false;

    if(ipaddr == NULL)
    {
        printf("ERROR: mqtt dns callback failed: %s\r\n", name);
        return;
    }

    mqtt_server_ip_addr = *ipaddr;
    printf("INFO: mqtt dns callback resolved: %s -> %s\r\n", name, ipaddr_ntoa(&mqtt_server_ip_addr));
}
/**
 * @brief mqtt connection cb.
 * @param client Parameter client.
 * @param arg Parameter arg.
 * @param status Parameter status.
 */
static void mqttClientConnectionCallback(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
    if(status == MQTT_CONNECT_ACCEPTED)
    {
        printf("INFO: mqtt_connection_cb: Successfully connected\r\n");
        /* Setup callback for incoming publish requests */
        mqtt_set_inpub_callback(client, mqttClientIncomingPublishCallback, mqttClientIncomingDataCallback, arg);
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
/**
 * @brief mqtt sub request cb.
 * @param arg Parameter arg.
 * @param result Parameter result.
 */
static void mqttClientSubRequestCallback(void *arg, err_t result)
{
    /* Just print the result code here for simplicity,
     normal behavior would be to take some action if subscribe fails like
     notifying user, retry subscribe or disconnect from server */
    printf("INFO: Subscribe result: %d\r\n", result);
}
/**
 * @brief mqtt incoming publish cb.
 * @param arg Parameter arg.
 * @param topic Parameter topic.
 * @param tot_len Parameter tot_len.
 */
static void mqttClientIncomingPublishCallback(void *arg, const char *topic, u32_t tot_len)
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
/**
 * @brief mqtt incoming data cb.
 * @param arg Parameter arg.
 * @param data Parameter data.
 * @param len Parameter len.
 * @param flags Parameter flags.
 */
static void mqttClientIncomingDataCallback(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
    // printf("Incoming publish payload with length %d, flags %u\n", len, (unsigned int) flags);

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
                Dio_SetBlindDirection(&mqttBlinds_pst[channel]);
                mqtt_blinddir_publish_pending = true;
            }
            else if(strncmp((const char*) data, payload_up, len) == 0)
            {
                mqttBlinds_pst[channel].blinddirection = blinddirection_up;
                mqttBlinds_pst[channel].position_target = 0 - MQTT_CLIENT_ADDITIONAL_MOVE_TIME_MS;
                mqttBlinds_pst[channel].angle_target = 0;
                if(mqttBlinds_pst[channel].position_function_active == false)
                {
                    mqttBlinds_pst[channel].position_actual = mqttBlinds_pst[channel].position_movingtimeup;
                }
                Dio_SetBlindDirection(&mqttBlinds_pst[channel]);
                mqtt_blinddir_publish_pending = true;
            }
            else if(strncmp((const char*) data, payload_down, len) == 0)
            {
                mqttBlinds_pst[channel].blinddirection = blinddirection_down;
                mqttBlinds_pst[channel].position_target = mqttBlinds_pst[channel].position_movingtimeup + MQTT_CLIENT_ADDITIONAL_MOVE_TIME_MS;
                mqttBlinds_pst[channel].angle_target = mqttBlinds_pst[channel].angle_movingtime;
                if(mqttBlinds_pst[channel].position_function_active == false)
                {
                    mqttBlinds_pst[channel].position_actual = 0;
                }
                Dio_SetBlindDirection(&mqttBlinds_pst[channel]);
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
                		mqttBlinds_pst[channel].position_target = mqttBlinds_pst[channel].position_movingtimeup + MQTT_CLIENT_ADDITIONAL_MOVE_TIME_MS;
                        if(mqttBlinds_pst[channel].position_function_active == false)
                        {
                            mqttBlinds_pst[channel].position_actual = mqttBlinds_pst[channel].position_movingtimeup;
                        }
                	}
                	else if(percent<=0)
                	{
                		mqttBlinds_pst[channel].position_target = 0 - MQTT_CLIENT_ADDITIONAL_MOVE_TIME_MS;
                        if(mqttBlinds_pst[channel].position_function_active == false)
                        {
                            mqttBlinds_pst[channel].position_actual = 0;
                        }
                	}
                	else
                	{
                	    mqttClientCalcBlindPosition(percent,&mqttBlinds_pst[channel]);
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
                    Dio_SetBlindDirection(&mqttBlinds_pst[channel]);
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
                Dio_SetBlindDirection(&mqttBlinds_pst[channel]);
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
/**
 * @brief publish doubleswitch stats.
 */
static void mqttClientPublishDoubleswitchStats(void)
{
    if(mqtt_client_is_connected(&client))
    {
        for(uint8_t var = 0; var < num_doubleswitches; ++var)
        {
            MqttClient_PublishDoubleswitchStat(&mqttDoubleswitches_pst[var]);
        }
    }
}

/**
 * @brief publish blinddir stats.
 */
void mqttClientPublishBlindDirStats(void)
{
    if(mqtt_client_is_connected(&client))
    {
        for(uint8_t var = 0; var < num_blinds; ++var)
        {
            MqttClient_PublishBlindDirStat(&mqttBlinds_pst[var]);
        }
    }
}

/**
 * @brief publish blindpos stats.
 */
void mqttClientPublishBlindPosStats(void)
{
    if(mqtt_client_is_connected(&client))
    {
        for(uint8_t var = 0; var < num_blinds; ++var)
        {
            mqttClientPublishBlindPosStat(&mqttBlinds_pst[var]);
        }
    }
}

/**
 * @brief publish blindpos stat.
 * @param blind Parameter blind.
 */
void mqttClientPublishBlindPosStat(struct blind_s *blind)
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

            sprintf(payload, "%d", (uint8_t) mqttClientCalcRealBlindPosition(blind));
            err = mqttClientPublishDispatch(topic, payload, (u16_t)strlen(payload), qos, retain, mqttClientPublishRequestCallback, NULL);

            if(err != ERR_OK)
                printf("ERROR: publish_blindpos_stat: %d\r\n", err);
        }
        blind->position_changed = false;
    }
}

/**
 * @brief publish blindangle stats.
 */
void mqttClientPublishBlindAngleStats(void)
{
    if(mqtt_client_is_connected(&client))
    {
        for(uint8_t var = 0; var < num_blinds; ++var)
        {
            mqttClientPublishBlindAngleStat(&mqttBlinds_pst[var]);
        }
    }
}

/**
 * @brief publish blindangle stat.
 * @param blind Parameter blind.
 */
void mqttClientPublishBlindAngleStat(struct blind_s *blind)
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
            err = mqttClientPublishDispatch(topic, payload, (u16_t)strlen(payload), qos, retain, mqttClientPublishRequestCallback, NULL);

            if(err != ERR_OK)
                printf("ERROR: publish_blindangle_stat: %d\r\n", err);
        }
        blind->angle_changed = false;
        //osDelay(10);
    }
}

/**
 * @brief publish blinddir cmds.
 */
void mqttClientPublishBlinddirCmds(void)
{
    if(mqtt_client_is_connected(&client))
    {
        for(uint8_t var = 0; var < num_blinds; ++var)
        {
            MqttClient_PublishBlindDirCmd(&mqttBlinds_pst[var]);
        }
    }
}


/**
 * @brief publish blindangle cmds.
 */
void mqttClientPublishBlindAngleCmds(void)
{
    if(mqtt_client_is_connected(&client))
    {
        for(uint8_t var = 0; var < num_blinds; ++var)
        {
            mqttClientPublishBlindAngleCmd(&mqttBlinds_pst[var]);
        }
    }
}

/**
 * @brief publish blindangle cmd.
 * @param blind Parameter blind.
 */
void mqttClientPublishBlindAngleCmd(struct blind_s *blind)
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
        err = mqttClientPublishDispatch(topic, payload, (u16_t)strlen(payload), qos, retain, mqttClientPublishRequestCallback, NULL);
        if(err != ERR_OK)
            printf("ERROR: publish_blindangle_cmd: %d\r\n", err);

    }
}
/**
 * @brief publish ip mac.
 */
void mqttClientPublishIpMac(void)
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
        err = mqttClientPublishDispatch(topic, ip, (u16_t)strlen(ip), qos, retain, NULL, NULL);
        if(err != ERR_OK)
            printf("ERROR: publish_ip %d\r\n", err);

        sprintf(topic, MACTELE, mqttname);
        char mac[13];
        sprintf(mac, "%02x%02x%02x%02x%02x%02x", netif_default->hwaddr[0], netif_default->hwaddr[1], netif_default->hwaddr[2], netif_default->hwaddr[3],
                netif_default->hwaddr[4], netif_default->hwaddr[5]);

        err = mqttClientPublishDispatch(topic, mac, (u16_t)strlen(mac), qos, retain, NULL, NULL);

        if(err != ERR_OK)
            printf("ERROR: publish_mac %d\r\n", err);

    }
}
/**
 * @brief publish lwt.
 * @param online Parameter online.
 */
void mqttClientPublishLwt(bool online)
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
            err = mqttClientPublishDispatch(topic, payload, (u16_t)strlen(payload), qos, 1, mqttClientPublishRequestCallback, NULL);
        }
        else
        {
            char payload[] = "Offline";
            err = mqttClientPublishDispatch(topic, payload, (u16_t)strlen(payload), qos, 1, mqttClientPublishRequestCallback, NULL);
        }
        if(err != ERR_OK)
            printf("ERROR: publish_lwt %d\n", err);
    }
}
/**
 * @brief subscribe blinddir cmd.
 */
void mqttClientSubscribeBlinddirCmd(void)
{
    err_t err;
    for(uint8_t var = 0; var < num_blinds; ++var)
    {
        char topic[sizeof(mqttname) + 17];
        sprintf(topic, BLINDDIRCMND"%02d", mqttname, mqttBlinds_pst[var].channel); //build Topic
        err = mqttClientSubscribeWithRetry(topic, 1, mqttClientSubRequestCallback, NULL);
        if(err != ERR_OK)
            printf("ERROR: subscribe_blinddir_cmd ch%d: %d\r\n", var+1, err);
        osDelay(10);
    }
}
/**
 * @brief subscribe blindpos cmd.
 */
void mqttClientSubscribeBlindPosCmd(void)
{
    err_t err;
    for(uint8_t var = 0; var < num_blinds; ++var)
    {
        char topic[sizeof(mqttname) + 17];
        sprintf(topic, BLINDPOSCMND"%02d", mqttname, mqttBlinds_pst[var].channel); //build Topic
        err = mqttClientSubscribeWithRetry(topic, 1, mqttClientSubRequestCallback, NULL);
        if(err != ERR_OK)
            printf("ERROR: subscribe_blindpos_cmd ch%d: %d\r\n", var+1,  err);
        osDelay(10);
    }
}
/**
 * @brief subscribe blindangle cmd.
 */
void mqttClientSubscribeBlindAngleCmd(void)
{
    err_t err;
    for(uint8_t var = 0; var < num_blinds; ++var)
    {
        char topic[sizeof(mqttname) + 17];
        sprintf(topic, BLINDANGCMND"%02d", mqttname, mqttBlinds_pst[var].channel); //build Topic
        err = mqttClientSubscribeWithRetry(topic, 1, mqttClientSubRequestCallback, NULL);
        if(err != ERR_OK)
            printf("ERROR: subscribe_blindangle_cmd ch%d: %d\r\n", var+1,  err);
        osDelay(10);
    }
}
/* Called when publish is complete either with success or failure */
/**
 * @brief mqtt pub request cb.
 * @param arg Parameter arg.
 * @param result Parameter result.
 */
static void mqttClientPublishRequestCallback(void *arg, err_t result)
{
    if(result != ERR_OK)
    {
        printf("ERROR: Publish result: %d\r\n", result);
    }
}


uint8_t mqttClientCalcRealBlindPosition(struct blind_s *blind)
{
    uint8_t percent;
    percent = round((double) 100.0 / blind->position_movingtimeup * blind->position_actual);
    if(percent >= 100)
    {
        percent = 100;
    }
    else if(percent <= 0)
    {
        percent = 0;
    }

    uint8_t xs[] = {0,0,100};
    uint8_t ys[] = {0,50,100};

    xs[1] = blind->position_50;


    /* number of elements in the array */
    static const int count = sizeof(xs)/sizeof(xs[0]);

    int i;
    double dx, dy;

    if (percent < xs[0]) {
        /* x is less than the minimum element
         * handle error here if you want */
        return ys[0]; /* return minimum element */
    }

    if (percent > xs[count-1]) {
        return ys[count-1]; /* return maximum */
    }

    /* find i, such that xs[i] <= x < xs[i+1] */
    for (i = 0; i < count-1; i++) {
        if (xs[i+1] > percent) {
            break;
        }
    }

    /* interpolate */
    dx = xs[i+1] - xs[i];
    dy = ys[i+1] - ys[i];
    return ys[i] + (percent - xs[i]) * dy / dx;
}

void mqttClientCalcBlindPosition(uint8_t percent, struct blind_s *blind)
{
    if(percent >= 100)
    {
        percent = 100;
    }
    else if(percent <= 0)
    {
        percent = 0;
    }

    uint8_t xs[] = {0,0,100};
    uint8_t ys[] = {0,50,100};

    xs[1] = blind->position_50;

    /* number of elements in the array */
    static const int count = sizeof(xs)/sizeof(xs[0]);

    int i;
    double dx, dy;

    /* find i, such that xs[i] <= x < xs[i+1] */
    for (i = 0; i < count-1; i++) {
        if (ys[i+1] > percent) {
            break;
        }
    }

    /* interpolate */
    dx = xs[i+1] - xs[i];
    dy = ys[i+1] - ys[i];

    percent = xs[i] + (percent - ys[i]) * dx / dy;

    blind->position_target = (double)blind->position_movingtimeup/(double)100*percent;
}


/**
 * @brief publish ha discovery cover.
 * @param blind Parameter blind.
 */
static void mqttClientPublishHaDiscoveryCover(struct blind_s *blind)
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
             "\"set_position_topic\":\"cmnd/%s/blinddir%02d\","
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
             "\"model\":\""MODEL"\","
             "\"manufacturer\":\""MANUFACTURER"\""
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
    err_t err = mqttClientPublishDispatch(config_topic, payload, (u16_t)payload_len,
                                        0 /* qos */, 1 /* retain */, mqttClientPublishRequestCallback, NULL);
    if(err != ERR_OK)
        printf("ERROR: publish_ha_discovery_cover ch%d: %d\r\n", blind->channel, err);
}

/**
 * @brief publish ha discovery input.
 * @param ds Parameter ds.
 * @return TODO.
 */
static void mqttClientPublishHaDiscoveryInput(struct doubleswitch_s *ds)
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
             "\"model\":\""MODEL"\","
             "\"manufacturer\":\""MANUFACTURER"\""
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
    err_t err = mqttClientPublishDispatch(config_topic, payload, (u16_t)payload_len,
                                        0 /* qos */, 1 /* retain */, mqttClientPublishRequestCallback, NULL);
    if(err != ERR_OK)
        printf("ERROR: publish_ha_discovery_input ch%d: %d\r\n", ds->channel, err);
}

/**
 * @brief publish ha discovery.
 */
void mqttClientPublishHaDiscovery(void)
{
    printf("INFO: Publishing Home Assistant MQTT discovery messages\r\n");

    for(uint8_t var = 0; var < num_blinds; ++var)
    {
        mqttClientPublishHaDiscoveryCover(&mqttBlinds_pst[var]);
        osDelay(500); /* wait for ring buffer to flush before next large payload */
    }

    mqttClientPublishHaDiscoveryInput(&mqttDoubleswitches_pst[8]);

    printf("INFO: Home Assistant MQTT discovery complete\r\n");
}



/**
 * @brief StartmqttTask.
 * @param argument Parameter argument.
 */
void MqttClient_StartTask(void *argument)
{
    printf("StartmqttTask\r\n");

    mqtt_task_thread_id = osThreadGetId();
    mqttClientPublishQueueInit();

    mqttBlinds_pst = Dio_GetBlinds();
    mqttDoubleswitches_pst = Dio_GetDoubleswitches();
    uint32_t last_blinds_refresh_tick = osKernelGetTickCount();

    /* Infinite loop */
    for(;;)
    {
        if(netif_default->ip_addr.addr != 0) //we need a IP Address to connect
        {
            if(mqtt_client_is_connected(&client)) /* while connected, publish */
            {
                mqttClientDrainPublishQueue();

                if(mqtt_bootstrap_pending)
                {
                    mqtt_bootstrap_pending = false;
                    mqtt_bootstrap_in_progress = true;

                    mqttClientPublishIpMac();
                    mqttClientPublishLwt(true);
                    mqttClientPublishHaDiscovery();
                    mqttClientPublishBlinddirCmds();
                    mqttClientSubscribeBlinddirCmd();
                    mqttClientPublishBlindDirStats();
                    mqttClientSubscribeBlindPosCmd();
                    mqttClientPublishBlindAngleCmds();
                    mqttClientSubscribeBlindAngleCmd();
                    mqttClientPublishDoubleswitchStats();

                    mqtt_bootstrap_in_progress = false;
                    printf("INFO: mqtt bootstrap complete\r\n");
                    mqttClientDrainPublishQueue();
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
                    mqttClientPublishBlindDirStats();
                }

                uint32_t now_tick = osKernelGetTickCount();
                if((now_tick - last_blinds_refresh_tick) >= MQTT_BLINDS_REFRESH_INTERVAL_MS)
                {
                    for(uint8_t var = 0; var < num_blinds; ++var)
                    {
                        mqttBlinds_pst[var].position_changed = true;
                        mqttBlinds_pst[var].angle_changed = true;
                    }
                    last_blinds_refresh_tick = now_tick;
                }

                mqttClientPublishBlindPosStats();
                mqttClientPublishBlindAngleStats();
                mqttClientDrainPublishQueue();
                osDelay(1000);
            }
            else
            {
                mqttClientConnect(&client);
                osDelay(1000);
            }
        }
        else
        {
            osDelay(1000);
        }
    }
}
/**
 * @brief getMQTTTopic.
 * @param topic Parameter topic.
 */
void MqttClient_GetMQTTTopic(char *topic)
{
    strcpy(topic, mqttname);
}
/**
 * @brief setMQTTTopic.
 * @param topic Parameter topic.
 */
void MqttClient_SetMQTTTopic(char *topic)
{
    strcpy(mqttname, topic);
}
/**
 * @brief getMQTTHost.
 * @param mqtt_host_addr Parameter mqtt_host_addr.
 */
void MqttClient_GetMQTTHost(ip_addr_t *mqtt_host_addr)
{
    *mqtt_host_addr = mqtt_server_ip_addr;
}

void MqttClient_GetMQTTHostString(char *host)
{
    if(host == NULL)
    {
        return;
    }

    strcpy(host, mqtt_server_host);
}
/**
 * @brief setMQTTHost.
 * @param mqtt_host_addr Parameter mqtt_host_addr.
 */
void MqttClient_SetMQTTHost(ip_addr_t *mqtt_host_addr)
{
    if(mqtt_host_addr == NULL)
    {
        return;
    }

    printf("set MQTT Host Address: %s\r\n", ipaddr_ntoa(mqtt_host_addr));
    mqtt_server_ip_addr = *mqtt_host_addr;
    snprintf(mqtt_server_host, sizeof(mqtt_server_host), "%s", ipaddr_ntoa(mqtt_host_addr));
    mqtt_server_host_is_dns = false;
    mqtt_dns_query_pending = false;
    mqtt_disconnect(&client); //disconnect to force new connect
}

void MqttClient_SetMQTTHostString(const char *host)
{
    if((host == NULL) || (host[0] == '\0'))
    {
        return;
    }

    snprintf(mqtt_server_host, sizeof(mqtt_server_host), "%s", host);

    if(ipaddr_aton(mqtt_server_host, &mqtt_server_ip_addr) == 1)
    {
        mqtt_server_host_is_dns = false;
        mqtt_dns_query_pending = false;
        printf("set MQTT Host Address (IPv4): %s\r\n", mqtt_server_host);
    }
    else
    {
        mqtt_server_host_is_dns = true;
        mqtt_dns_query_pending = false;
        printf("set MQTT Host Address (DNS): %s\r\n", mqtt_server_host);
    }

    mqtt_disconnect(&client); //disconnect to force new connect
}
/**
 * @brief setMQTTCurrent.
 * @param _current Parameter _current.
 */
void MqttClient_SetMQTTCurrent(int16_t _current)
{
    current = _current;
}

/**
 * @brief publish blinddir cmd.
 * @param blind Parameter blind.
 */
void MqttClient_PublishBlindDirCmd(struct blind_s *blind)
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
            err = mqttClientPublishDispatch(topic, payload_up, (u16_t)strlen(payload_up), qos, retain, mqttClientPublishRequestCallback,
                    NULL);
            break;
        case blinddirection_down:
            err = mqttClientPublishDispatch(topic, payload_down, (u16_t)strlen(payload_down), qos, retain, mqttClientPublishRequestCallback,
                    NULL);
            break;
        case blinddirection_off:
            err = mqttClientPublishDispatch(topic, payload_off, (u16_t)strlen(payload_off), qos, retain, mqttClientPublishRequestCallback,
                    NULL);
            break;
        default:
            break;
        }
        if(err != ERR_OK)
            printf("ERROR: publish_blinddir_cmd: %d\r\n", err);

    }
}


/**
 * @brief publish doubleswitch stat.
 * @param doubleswitch Parameter doubleswitch.
 */
void MqttClient_PublishDoubleswitchStat(struct doubleswitch_s *doubleswitch)
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
                err = mqttClientPublishDispatch(str, payload_off, (u16_t)strlen(payload_off), qos, retain, mqttClientPublishRequestCallback,
                        NULL);
                break;
            case inputdirection_up:
                err = mqttClientPublishDispatch(str, payload_up, (u16_t)strlen(payload_up), qos, retain, mqttClientPublishRequestCallback,
                        NULL);
                break;
            case inputdirection_down:
                err = mqttClientPublishDispatch(str, payload_down, (u16_t)strlen(payload_down), qos, retain, mqttClientPublishRequestCallback,
                        NULL);
                break;
            default:
                break;
        }
        if(err != ERR_OK)
            printf("ERROR: publish_doubleswitch_stat %d\n", err);
    }
}

/**
 * @brief publish blinddir stat.
 * @param blind Parameter blind.
 */
void MqttClient_PublishBlindDirStat(struct blind_s *blind)
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
                err = mqttClientPublishDispatch(topic, payload_up, (u16_t)strlen(payload_up), qos, retain, mqttClientPublishRequestCallback,
                        NULL);
                break;
            case blinddirection_down:
                err = mqttClientPublishDispatch(topic, payload_down, (u16_t)strlen(payload_down), qos, retain, mqttClientPublishRequestCallback,
                        NULL);
                break;
            case blinddirection_off:
                err = mqttClientPublishDispatch(topic, payload_off, (u16_t)strlen(payload_off), qos, retain, mqttClientPublishRequestCallback,
                        NULL);
                break;
            default:
                break;
        }
        if(err != ERR_OK)
            printf("ERROR: publish_blinddir_stat: %d\r\n", err);
    }
}


/**
 * @brief publish current.
 */
void MqttClient_PublishCurrent(void)
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
        err = mqttClientPublishDispatch(topic, payload, (u16_t)strlen(payload), qos, retain, mqttClientPublishRequestCallback, NULL);

        if(err != ERR_OK)
            printf("ERROR: publish_current: %d\r\n", err);

    }
}
