/*
 * mqtt_client.c
 *
 *  Created on: Jan 11, 2021
 *      Author: LBekel
 */
#include "cmsis_os.h"
#include "string.h"
#include "lwip.h"
#include "mqtt.h"
#include "mqtt_client.h"
#include <lwip/dhcp.h>
#include "lwip/apps/mqtt.h"
#include "lwip/apps/mqtt_priv.h"
#include <stdio.h>




//uint16_t Relay_Pins[] = { Relay1_Pin, Relay2_Pin, Relay3_Pin, Relay4_Pin,
//		Relay5_Pin, Relay6_Pin, Relay7_Pin, Relay8_Pin };
//
//GPIO_TypeDef * Relay_Ports[] = { Relay1_GPIO_Port, Relay2_GPIO_Port, Relay3_GPIO_Port,
//		Relay4_GPIO_Port, Relay5_GPIO_Port, Relay6_GPIO_Port, Relay7_GPIO_Port,
//		Relay8_GPIO_Port };
//
//uint16_t Input_Pins[] = { Input_1_Pin, Input_2_Pin, Input_3_Pin, Input_4_Pin,
//		Input_5_Pin, Input_6_Pin, Input_7_Pin, Input_8_Pin , Input_9_Pin , Input_10_Pin };
//
//GPIO_TypeDef * Input_Ports[] = { Input_1_GPIO_Port, Input_2_GPIO_Port, Input_3_GPIO_Port,
//		Input_4_GPIO_Port, Input_5_GPIO_Port, Input_6_GPIO_Port, Input_7_GPIO_Port,
//		Input_8_GPIO_Port, Input_9_GPIO_Port, Input_10_GPIO_Port};

bool RelayStates[num_relay_ch] = {false};
bool InputStates[num_input_ch] = {false};
static mqtt_client_t* client;
static ip_addr_t mqtt_server_ip_addr;
static char *payload_ON = "ON";
static char *payload_OFF = "OFF";
static char mqttname[27];

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);
static void mqtt_sub_request_cb(void *arg, err_t result);
static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len);
static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags);
void example_publish(mqtt_client_t *client, void *arg);
void check_inputs(mqtt_client_t *client, bool force);
static void mqtt_pub_request_cb(void *arg, err_t result);

void mqtt_connect(mqtt_client_t *client)
{
    struct mqtt_connect_client_info_t ci;
    err_t err;

    IP4_ADDR(&mqtt_server_ip_addr, 192, 168, 1, 3);
    //printf("IP Address: %s\r\n", ipaddr_ntoa(&mqtt_server_ip_addr));
    /* Setup an empty client info structure */
    memset(&ci, 0, sizeof(ci));

    /* Minimal amount of information required is client identifier, so set it here */
    //printf("MAC Address: %02x:%02x:%02x:%02x:%02x:%02x\r\n",gnetif.hwaddr[0],gnetif.hwaddr[1],gnetif.hwaddr[2],gnetif.hwaddr[3],gnetif.hwaddr[4],gnetif.hwaddr[5]);
    sprintf(mqttname, "ETHCTRLBLINDS_%02x%02x%02x%02x%02x%02x", gnetif.hwaddr[0],gnetif.hwaddr[1],gnetif.hwaddr[2],gnetif.hwaddr[3],gnetif.hwaddr[4],gnetif.hwaddr[5]);

    ci.client_id = mqttname;
    ci.keep_alive = 60;

    //printf("ClientID: %s\r\n",ci.client_id);

    /* Initiate client and connect to server, if this fails immediately an error code is returned
     otherwise mqtt_connection_cb will be called with connection result after attempting
     to establish a connection with the server.
     For now MQTT version 3.1.1 is always used */
    err = mqtt_client_connect(client, &mqtt_server_ip_addr, MQTT_PORT, mqtt_connection_cb, NULL, &ci);

    /* For now just print the result code if something goes wrong*/
    if(err != ERR_OK)
    {
        printf("mqtt_connect return %d\r\n", err);
    }
}

static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
    err_t err;
    if(status == MQTT_CONNECT_ACCEPTED)
    {
        printf("mqtt_connection_cb: Successfully connected\r\n");

        /* Setup callback for incoming publish requests */
        mqtt_set_inpub_callback(client, mqtt_incoming_publish_cb, mqtt_incoming_data_cb, arg);

        /* Subscribe to a topic named "subtopic" with QoS level 1, call mqtt_sub_request_cb with result */
        //err = mqtt_subscribe(client, "switchCommandCh01", 0, mqtt_sub_request_cb, arg);
        for(uint8_t var = 1; var <= num_relay_ch; ++var)
        {
            char str[18];
            sprintf(str, "relayCommandCh%02d", var);
            //sync_printf("%s\n", str);

            err = mqtt_subscribe(client, str, 1, mqtt_sub_request_cb, arg);
            if(err != ERR_OK)
                printf("mqtt_subscribe return: %d\r\n", err);
        }
    }
    else
    {
        printf("mqtt_connection_cb: Disconnected, reason: %d\r\n", status);

        /* Its more nice to be connected, so try to reconnect */
        //mqtt_connect(client);
    }
}

static void mqtt_sub_request_cb(void *arg, err_t result)
{
    /* Just print the result code here for simplicity,
     normal behavior would be to take some action if subscribe fails like
     notifying user, retry subscribe or disconnect from server */
    printf("Subscribe result: %d\r\n", result);
}

static uint8_t inpub_id;
static uint8_t channel;
static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{
    //sync_printf("Incoming publish at topic %s with total length %u\n", topic, (unsigned int) tot_len);
    /* Decode topic string into a user defined reference */
    if(strncmp(topic, "relayCommandCh", 14) == 0)
    {
        inpub_id = 0;
        sscanf(topic,"relayCommandCh%"PRIu8"",&channel);
        printf("Channel %d ",channel);
    }
    else if(topic[0] == 'A')
    {
        /* All topics starting with 'A' might be handled at the same way */
        inpub_id = 1;
    }
    else
    {
        /* For all other topics */
        inpub_id = 2;
    }
}

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
    //sync_printf("Incoming publish payload with length %d, flags %u\n", len, (unsigned int) flags);

    if(flags & MQTT_DATA_FLAG_LAST)
    {
        /* Last fragment of payload received (or whole part if payload fits receive buffer
         See MQTT_VAR_HEADER_BUFFER_LEN)  */

        /* Call function or do action depending on reference, in this case inpub_id */
        if(inpub_id == 0)
        {
            if(strncmp((const char*)data, "ON", len) == 0)
            {
                printf("ON\n");
                HAL_GPIO_WritePin(Relay_Ports[channel-1], Relay_Pins[channel-1], GPIO_PIN_SET);
                RelayStates[channel-1] = true;

            }
            else if(strncmp((const char*)data, "OFF", len) == 0)
            {
                printf("OFF\n");
                HAL_GPIO_WritePin(Relay_Ports[channel-1], Relay_Pins[channel-1], GPIO_PIN_RESET);
                RelayStates[channel-1] = false;
            }
            else
            {
                //printf("mqtt_incoming_data_cb: %s\n", (const char *) data);
            }
            publish_relay_states();
        }
        else if(inpub_id == 1)
        {
            /* Call an 'A' function... */
        }
        else
        {
            //printf("mqtt_incoming_data_cb: Ignoring payload...\n");
        }
    }
    else
    {
        /* Handle fragmented payload, store in buffer, write to file or whatever */
    }
}

void check_inputs(mqtt_client_t *client, bool force)
{
    err_t err = ERR_OK;
    u8_t qos = 0; /* 0 1 or 2, see MQTT specification */
    u8_t retain = 0;

    for(uint8_t var = 1; var < num_input_ch + 1; ++var)
    {
        char str[16];
        sprintf(str, "inputStateCh%02d", var); //build Topic
        //printf("%s\n", str);

        if(HAL_GPIO_ReadPin(Input_Ports[var-1], Input_Pins[var-1])==GPIO_PIN_RESET)
        {
        	if((InputStates[var-1] == false)||force)//only publish on value change
        	{
        		err = mqtt_publish(client, str, payload_ON, strlen(payload_ON), qos, retain, mqtt_pub_request_cb, NULL);
        		osDelay(100); //TODO: entprellen
        	}
        	InputStates[var-1] = true;
        }
        else
        {
        	if((InputStates[var-1] == true)||force)//only publish on value change
        	{
        		err = mqtt_publish(client, str, payload_OFF, strlen(payload_OFF), qos, retain, mqtt_pub_request_cb, NULL);
        		osDelay(100); //TODO: entprellen
        	}
        	InputStates[var-1] = false;
        }
		if (err != ERR_OK)
			printf("Publish err: %d\r\n", err);

    }
}

void publish_relay_states(void) {
	err_t err = ERR_OK;
	u8_t qos = 0; /* 0 1 or 2, see MQTT specification */
	u8_t retain = 0;
	if (mqtt_client_is_connected(client)) {
		for (uint8_t var = 1; var < num_relay_ch + 1; ++var) {
			char str[44];
			sprintf(str, "%s/relayStateCh%02d", mqttname, var); //build Topic
			//printf("%s\n", str);
			if (RelayStates[var - 1] == true) {
				err = mqtt_publish(client, str, payload_ON, strlen(payload_ON),
						qos, retain, mqtt_pub_request_cb, NULL);
			} else {
				err = mqtt_publish(client, str, payload_OFF,
						strlen(payload_OFF), qos, retain, mqtt_pub_request_cb,
						NULL);
			}
			if (err != ERR_OK)
				printf("Publish err: %d\r\n", err);

		}
	}

}

/* Called when publish is complete either with success or failure */
static void mqtt_pub_request_cb(void *arg, err_t result)
{
    if(result != ERR_OK)
    {
        printf("Publish result: %d\r\n", result);
    }
}


/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
void StartmqttTask(void *argument)
{
	printf("StartmqttTask\r\n");
	client = mqtt_client_new();
	if(client == NULL){
		printf("ERROR: New MQTT client space\r\n");
	}
	printf("Client %d.\r\n", client->conn_state);
	/* Infinite loop */
	for (;;)
	{
		if (gnetif.ip_addr.addr != 0) //we need a IP Address to connect
		{

			if (mqtt_client_is_connected(client)) /* while connected, publish */
			{
				publish_relay_states();
				//check_inputs(client,false);
				osDelay(1000);
			}
			else
			{
				printf("MQTT Disconnect %d.\r\n", client->conn_state);
				mqtt_connect(client);
				//force publish actual inputstats
				//check_inputs(client, true);

				osDelay(1000);
			}
		}
	}
}
