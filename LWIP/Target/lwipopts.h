
/**
  ******************************************************************************
  * File Name          : Target/lwipopts.h
  * Description        : This file overrides LwIP stack default configuration
  *                      done in opt.h file.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion --------------------------------------*/
#ifndef __LWIPOPTS__H__
#define __LWIPOPTS__H__

#include "main.h"

/*-----------------------------------------------------------------------------*/
/* Current version of LwIP supported by CubeMx: 2.1.2 -*/
/*-----------------------------------------------------------------------------*/

/* Within 'USER CODE' section, code will be kept by default at each generation */
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

#ifdef __cplusplus
 extern "C" {
#endif

/* STM32CubeMX Specific Parameters (not defined in opt.h) ---------------------*/
/* Parameters set in STM32CubeMX LwIP Configuration GUI -*/
/*----- WITH_RTOS enabled (Since FREERTOS is set) -----*/
#define WITH_RTOS 1
/*----- CHECKSUM_BY_HARDWARE enabled -----*/
#define CHECKSUM_BY_HARDWARE 1
/*-----------------------------------------------------------------------------*/

/* LwIP Stack Parameters (modified compared to initialization value in opt.h) -*/
/* Parameters set in STM32CubeMX LwIP Configuration GUI -*/
/*----- Value in opt.h for LWIP_DHCP: 0 -----*/
#define LWIP_DHCP 1
/*----- Default Value for LWIP_IGMP: 0 ---*/
#define LWIP_IGMP 1
/*----- Default Value for MEMP_NUM_UDP_PCB: 4 ---*/
#define MEMP_NUM_UDP_PCB 6
/*----- Default Value for MEMP_NUM_TCP_PCB: 5 ---*/
#define MEMP_NUM_TCP_PCB 6
/*----- Default Value for LWIP_TCPIP_CORE_LOCKING: 0 ---*/
#define LWIP_TCPIP_CORE_LOCKING 1
/*----- Default Value for MEMP_MEM_INIT: 0 ---*/
#define MEMP_MEM_INIT 1
/*----- Value in opt.h for MEM_ALIGNMENT: 1 -----*/
#define MEM_ALIGNMENT 4
/*----- Default Value for MEM_SIZE: 1600 ---*/
#define MEM_SIZE 61440
/*----- Default Value for MEMP_OVERFLOW_CHECK: 0 ---*/
#define MEMP_OVERFLOW_CHECK 1
/*----- Default Value for MEMP_SANITY_CHECK: 0 ---*/
#define MEMP_SANITY_CHECK 1
/*----- Default Value for MEM_OVERFLOW_CHECK: 0 ---*/
#define MEM_OVERFLOW_CHECK 1
/*----- Default Value for MEM_SANITY_CHECK: 0 ---*/
#define MEM_SANITY_CHECK 1
/*----- Default Value for LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT: 0 ---*/
#define LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT 1
/*----- Default Value for MEMP_NUM_PBUF: 16 ---*/
#define MEMP_NUM_PBUF 64
/*----- Default Value for MEMP_NUM_TCP_PCB_LISTEN: 8 ---*/
#define MEMP_NUM_TCP_PCB_LISTEN 10
/*----- Default Value for MEMP_NUM_SYS_TIMEOUT: 7 ---*/
#define MEMP_NUM_SYS_TIMEOUT 12
/*----- Default Value for PBUF_POOL_SIZE: 16 ---*/
#define PBUF_POOL_SIZE 64
/*----- Default Value for PBUF_POOL_BUFSIZE: 592 ---*/
#define PBUF_POOL_BUFSIZE 2048
/*----- Value in opt.h for LWIP_ETHERNET: LWIP_ARP || PPPOE_SUPPORT -*/
#define LWIP_ETHERNET 1
/*----- Default Value for LWIP_MULTICAST_PING: 0 ---*/
#define LWIP_MULTICAST_PING 1
/*----- Default Value for LWIP_AUTOIP: 0 ---*/
#define LWIP_AUTOIP 1
/*----- Default Value for LWIP_DHCP_AUTOIP_COOP: 0 ---*/
#define LWIP_DHCP_AUTOIP_COOP 1
/*----- Default Value for LWIP_DHCP_AUTOIP_COOP_TRIES: 9 ---*/
#define LWIP_DHCP_AUTOIP_COOP_TRIES 1
/*----- Value in opt.h for LWIP_DNS_SECURE: (LWIP_DNS_SECURE_RAND_XID | LWIP_DNS_SECURE_NO_MULTIPLE_OUTSTANDING | LWIP_DNS_SECURE_RAND_SRC_PORT) -*/
#define LWIP_DNS_SECURE 7
/*----- Default Value for TCP_SND_BUF: 1072 ---*/
#define TCP_SND_BUF 2048
/*----- Default Value for TCP_SND_QUEUELEN: 16 ---*/
#define TCP_SND_QUEUELEN 9
/*----- Value in opt.h for TCP_SNDQUEUELOWAT: LWIP_MAX(TCP_SND_QUEUELEN)/2, 5) -*/
#define TCP_SNDQUEUELOWAT 5
/*----- Value in opt.h for TCP_WND_UPDATE_THRESHOLD: LWIP_MIN(TCP_WND/4, TCP_MSS*4) -----*/
#define TCP_WND_UPDATE_THRESHOLD 536
/*----- Default Value for LWIP_SINGLE_NETIF: 0 ---*/
#define LWIP_SINGLE_NETIF 1
/*----- Default Value for LWIP_NETIF_HOSTNAME: 0 ---*/
#define LWIP_NETIF_HOSTNAME 1
/*----- Default Value for LWIP_NETIF_STATUS_CALLBACK: 0 ---*/
#define LWIP_NETIF_STATUS_CALLBACK 1
/*----- Value in opt.h for LWIP_NETIF_LINK_CALLBACK: 0 -----*/
#define LWIP_NETIF_LINK_CALLBACK 1
/*----- Default Value for LWIP_NUM_NETIF_CLIENT_DATA: 0 ---*/
#define LWIP_NUM_NETIF_CLIENT_DATA 1
/*----- Value in opt.h for TCPIP_THREAD_STACKSIZE: 0 -----*/
#define TCPIP_THREAD_STACKSIZE 4096
/*----- Value in opt.h for TCPIP_THREAD_PRIO: 1 -----*/
#define TCPIP_THREAD_PRIO 49
/*----- Value in opt.h for TCPIP_MBOX_SIZE: 0 -----*/
#define TCPIP_MBOX_SIZE 6
/*----- Value in opt.h for SLIPIF_THREAD_STACKSIZE: 0 -----*/
#define SLIPIF_THREAD_STACKSIZE 128
/*----- Value in opt.h for SLIPIF_THREAD_PRIO: 1 -----*/
#define SLIPIF_THREAD_PRIO 3
/*----- Value in opt.h for DEFAULT_THREAD_STACKSIZE: 0 -----*/
#define DEFAULT_THREAD_STACKSIZE 128
/*----- Value in opt.h for DEFAULT_THREAD_PRIO: 1 -----*/
#define DEFAULT_THREAD_PRIO 3
/*----- Value in opt.h for DEFAULT_UDP_RECVMBOX_SIZE: 0 -----*/
#define DEFAULT_UDP_RECVMBOX_SIZE 6
/*----- Value in opt.h for DEFAULT_TCP_RECVMBOX_SIZE: 0 -----*/
#define DEFAULT_TCP_RECVMBOX_SIZE 6
/*----- Value in opt.h for DEFAULT_ACCEPTMBOX_SIZE: 0 -----*/
#define DEFAULT_ACCEPTMBOX_SIZE 6
/*----- Default Value for LWIP_TCPIP_TIMEOUT: 0 ---*/
#define LWIP_TCPIP_TIMEOUT 1
/*----- Value in opt.h for RECV_BUFSIZE_DEFAULT: INT_MAX -----*/
#define RECV_BUFSIZE_DEFAULT 2000000000
/*----- Default Value for LWIP_HTTPD: 0 ---*/
#define LWIP_HTTPD 1
/*----- Default Value for LWIP_HTTPD_CGI: 0 ---*/
#define LWIP_HTTPD_CGI 1
/*----- Default Value for LWIP_HTTPD_SSI: 0 ---*/
#define LWIP_HTTPD_SSI 1
/*----- Default Value for LWIP_HTTPD_MAX_CGI_PARAMETERS: 16 ---*/
#define LWIP_HTTPD_MAX_CGI_PARAMETERS 48
/*----- Default Value for LWIP_HTTPD_MAX_TAG_NAME_LEN: 8 ---*/
#define LWIP_HTTPD_MAX_TAG_NAME_LEN 10
/*----- Default Value for LWIP_HTTPD_MAX_TAG_INSERT_LEN: 192 ---*/
#define LWIP_HTTPD_MAX_TAG_INSERT_LEN 800
/*----- Default Value for LWIP_HTTPD_DYNAMIC_HEADERS: 0 ---*/
#define LWIP_HTTPD_DYNAMIC_HEADERS 1
/*----- Default Value for LWIP_HTTPD_REQ_BUFSIZE: 1023 ---*/
#define LWIP_HTTPD_REQ_BUFSIZE 2048
/*----- Default Value for LWIP_HTTPD_ABORT_ON_CLOSE_MEM_ERROR: 0 ---*/
#define LWIP_HTTPD_ABORT_ON_CLOSE_MEM_ERROR 1
/*----- Default Value for LWIP_HTTPD_KILL_OLD_ON_CONNECTIONS_EXCEEDED: 0 ---*/
#define LWIP_HTTPD_KILL_OLD_ON_CONNECTIONS_EXCEEDED 1
/*----- Value in opt.h for HTTPD_USE_CUSTOM_FSDATA: 0 -----*/
#define HTTPD_USE_CUSTOM_FSDATA 1
/*----- Default Value for LWIP_MDNS: 0 ---*/
#define LWIP_MDNS 1
/*----- Default Value for LWIP_MDNS_RESPONDER: 0 ---*/
#define LWIP_MDNS_RESPONDER 1
/*----- Default Value for MDNS_MAX_SERVICES: 0 ---*/
#define MDNS_MAX_SERVICES 1
/*----- Default Value for MDNS_RESP_USENETIF_EXTCALLBACK: 0 ---*/
#define MDNS_RESP_USENETIF_EXTCALLBACK 1
/*----- Value in opt.h for LWIP_STATS: 1 -----*/
#define LWIP_STATS 0
/*----- Value in opt.h for CHECKSUM_GEN_IP: 1 -----*/
#define CHECKSUM_GEN_IP 0
/*----- Value in opt.h for CHECKSUM_GEN_UDP: 1 -----*/
#define CHECKSUM_GEN_UDP 0
/*----- Value in opt.h for CHECKSUM_GEN_TCP: 1 -----*/
#define CHECKSUM_GEN_TCP 0
/*----- Value in opt.h for CHECKSUM_GEN_ICMP: 1 -----*/
#define CHECKSUM_GEN_ICMP 0
/*----- Value in opt.h for CHECKSUM_GEN_ICMP6: 1 -----*/
#define CHECKSUM_GEN_ICMP6 0
/*----- Value in opt.h for CHECKSUM_CHECK_IP: 1 -----*/
#define CHECKSUM_CHECK_IP 0
/*----- Value in opt.h for CHECKSUM_CHECK_UDP: 1 -----*/
#define CHECKSUM_CHECK_UDP 0
/*----- Value in opt.h for CHECKSUM_CHECK_TCP: 1 -----*/
#define CHECKSUM_CHECK_TCP 0
/*----- Value in opt.h for CHECKSUM_CHECK_ICMP: 1 -----*/
#define CHECKSUM_CHECK_ICMP 0
/*----- Value in opt.h for CHECKSUM_CHECK_ICMP6: 1 -----*/
#define CHECKSUM_CHECK_ICMP6 0
/*-----------------------------------------------------------------------------*/
/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

#ifdef __cplusplus
}
#endif
#endif /*__LWIPOPTS__H__ */

/************************* (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
