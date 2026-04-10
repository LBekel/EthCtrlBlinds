/*
 * Copyright (c) 2013-2018 Arm Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * -----------------------------------------------------------------------
 *
 * $Date:        25. May 2018
 * $Revision:    V6.3
 *
 * Project:      Ethernet Physical Layer Transceiver (PHY)
 *               Definitions for KSZ8081RNA (KSZ8081RNB compatible)
 * -------------------------------------------------------------------- */

#ifndef __PHY_KSZ8081RNA_H
#define __PHY_KSZ8081RNA_H

#include <stdint.h>

/* Register Map */
#define KSZ8081_BMCR              0x00U     /* Basic Control                      */
#define KSZ8081_BMSR              0x01U     /* Basic Status                       */
#define KSZ8081_PHYIDR1           0x02U     /* PHY Identifier 1                   */
#define KSZ8081_PHYIDR2           0x03U     /* PHY Identifier 2                   */
#define KSZ8081_ANAR              0x04U     /* Auto-Negotiation Advertisement     */
#define KSZ8081_ANLPAR            0x05U     /* Auto-Neg. Link Partner Ability     */
#define KSZ8081_ANER              0x06U     /* Auto-Neg. Expansion                */
#define KSZ8081_ANNP              0x07U     /* Auto-Neg. Next Page                */
#define KSZ8081_LPNP              0x08U     /* Link Partner Next Page Ability     */

#define KSZ8081_DRCTRL            0x10U     /* Digital Reserved Control           */
#define KSZ8081_AFECTRL1          0x11U     /* AFE Control 1                      */
#define KSZ8081_RXERCNT           0x15U     /* RXER Counter                       */
#define KSZ8081_OMSO              0x16U     /* Operation Mode Strap Override      */
#define KSZ8081_OMSS              0x17U     /* Operation Mode Strap Status        */
#define KSZ8081_EXCTRL            0x18U     /* Expanded Control                   */
#define KSZ8081_IRQCS             0x1BU     /* Interrupt Control/Status           */
#define KSZ8081_LMDCS             0x1DU     /* LinkMD Control/Status              */
#define KSZ8081_PHYCR1            0x1EU     /* PHY Control 1                      */
#define KSZ8081_PHYCR2            0x1FU     /* PHY Control 2                      */

/* Basic Control Register Bitmasks */
#define KSZ8081_BMCR_RESET            0x8000U   /* Software Reset                     */
#define KSZ8081_BMCR_LOOPBACK         0x4000U   /* Loopback mode                      */
#define KSZ8081_BMCR_SPEED_SELECT     0x2000U   /* Speed Select (1=100Mb/s)           */
#define KSZ8081_BMCR_ANEG_EN          0x1000U   /* Auto Negotiation Enable            */
#define KSZ8081_BMCR_POWER_DOWN       0x0800U   /* Power Down                         */
#define KSZ8081_BMCR_ISOLATE          0x0400U   /* Isolate Media interface            */
#define KSZ8081_BMCR_RESTART_ANEG     0x0200U   /* Restart Auto Negotiation           */
#define KSZ8081_BMCR_DUPLEX_MODE      0x0100U   /* Duplex Mode (1=Full duplex)        */
#define KSZ8081_BMCR_COLLISION_TEST   0x0080U   /* Collision Test                     */

/* Basic Status Register Bitmasks */
#define KSZ8081_BMSR_100B_T4          0x8000U   /* 100BASE-T4 Capable                 */
#define KSZ8081_BMSR_100B_TX_FD       0x4000U   /* 100BASE-TX Full Duplex Capable     */
#define KSZ8081_BMSR_100B_TX_HD       0x2000U   /* 100BASE-TX Half Duplex Capable     */
#define KSZ8081_BMSR_10B_T_FD         0x1000U   /* 10BASE-T Full Duplex Capable       */
#define KSZ8081_BMSR_10B_T_HD         0x0800U   /* 10BASE-T Half Duplex Capable       */
#define KSZ8081_BMSR_NO_PREAMBLE      0x0040U   /* Preamble suppression               */
#define KSZ8081_BMSR_ANEG_COMPLETE    0x0020U   /* Auto Negotiation Complete          */
#define KSZ8081_BMSR_REMOTE_FAULT     0x0010U   /* Remote Fault                       */
#define KSZ8081_BMSR_ANEG_ABILITY     0x0008U   /* Auto Negotiation Ability           */
#define KSZ8081_BMSR_LINK_STAT        0x0004U   /* Link Status (1=link is up)         */
#define KSZ8081_BMSR_JABBER_DETECT    0x0002U   /* Jabber Detect                      */
#define KSZ8081_BMSR_EXT_CAPAB        0x0001U   /* Extended Capability                */

/* PHY Identifier Registers Bitmasks */
#define KSZ8081_PHY_ID1               0x0022U   /* PHY ID Number 1                    */
#define KSZ8081_PHY_ID2               0x1560U   /* PHY ID Number 2                    */

/* Auto-Negotiation Advertisement Register Bitmasks */
#define KSZ8081_ANAR_NEXT_PAGE        0x8000U   /* Next page capable                  */
#define KSZ8081_ANAR_REMOTE_FAULT     0x2000U   /* Remote fault supported             */
#define KSZ8081_ANAR_PAUSE            0x0C00U   /* Pause                              */
#define KSZ8081_ANAR_100B_T4          0x0200U   /* 100Base-T4 capable                 */
#define KSZ8081_ANAR_100B_TX_FD       0x0100U   /* 100MBps full-duplex capable        */
#define KSZ8081_ANAR_100B_TX_HD       0x0080U   /* 100MBps half-duplex capable        */
#define KSZ8081_ANAR_10B_TX_FD        0x0040U   /* 10MBps full-duplex capable         */
#define KSZ8081_ANAR_10B_TX_HD        0x0020U   /* 10MBps half-duplex capable         */
#define KSZ8081_ANAR_SELECTOR_FIELD   0x001FU   /* Selector field (0x01==IEEE 802.3)  */

/* Auto-Negotiation Link Partner Ability Register Bitmasks */
#define KSZ8081_ANLPAR_NEXT_PAGE      0x8000U   /* Next page capable                  */
#define KSZ8081_ANLPAR_ACKNOWLEDGE    0x4000U   /* Acknowledge from partner           */
#define KSZ8081_ANLPAR_REMOTE_FAULT   0x2000U   /* Remote fault detected              */
#define KSZ8081_ANLPAR_PAUSE          0x0C00U   /* Pause                              */
#define KSZ8081_ANLPAR_100B_TX_FD     0x0100U   /* 100MBps full-duplex capable        */
#define KSZ8081_ANLPAR_100B_TX_HD     0x0080U   /* 100MBps half-duplex capable        */
#define KSZ8081_ANLPAR_10B_TX_FD      0x0040U   /* 10MBps full-duplex capable         */
#define KSZ8081_ANLPAR_10B_TX_HD      0x0020U   /* 10MBps half-duplex capable         */
#define KSZ8081_ANLPAR_SELECTOR_FIELD 0x001FU   /* Selector field (0x01==IEEE 802.3)  */

/* Auto-Negotiation Expansion Register Bitmasks */
#define KSZ8081_ANER_PDF              0x0010U   /* Parallel Detection Fault           */
#define KSZ8081_ANER_LPAR_NEXT_PAGE   0x0008U   /* Link Partner Next Page Able        */
#define KSZ8081_ANER_NEXT_PAGE        0x0004U   /* Next Page Able                     */
#define KSZ8081_ANER_PAGE_RECEIVED    0x0002U   /* Page Received                      */
#define KSZ8081_ANER_LPAR_ANEG        0x0001U   /* Link Partner Auto-Negotiation Able */

/* Auto-Negotiation Next Page Register Bitmasks */
#define KSZ8081_ANNP_NEXT_PAGE        0x8000U   /* Next Page                          */
#define KSZ8081_ANNP_MESSAGE_PAGE     0x2000U   /* Message Page                       */
#define KSZ8081_ANNP_ACKNOWLEDGE2     0x1000U   /* Acknowledge2                       */
#define KSZ8081_ANNP_TOGGLE           0x0800U   /* Toggle                             */
#define KSZ8081_ANNP_MESSAGE_FIELD    0x07FFU   /* Message Field                      */

/* Link Partner Next Page Ability Register Bitmasks */
#define KSZ8081_LPNP_NEXT_PAGE        0x8000U   /* Next page                          */
#define KSZ8081_LPNP_ACKNOWLEDGE      0x4000U   /* Acknowledge                        */
#define KSZ8081_LPNP_MESSAGE_PAGE     0x2000U   /* Message Page                       */
#define KSZ8081_LPNP_ACKNOWLEDGE2     0x1000U   /* Acknowledge2                       */
#define KSZ8081_LPNP_TOGGLE           0x0800U   /* Toggle                             */
#define KSZ8081_LPNP_MESSAGE_FIELD    0x07FFU   /* Message Field                      */

/* Digital Reserved Control Register Bitmasks */
#define KSZ8081_DRCTRL_PLL_OFF        0x0010U   /* PLL Off                            */

/* AFE Control 1 Register Bitmasks */
#define KSZ8081_AFECTRL1_SOSC_EN      0x0020U   /* Slow-Oscillator Mode Enable        */

/* Operation Mode Strap Override Register Bitmasks */
#define KSZ8081_OMSO_RSVD_FACTORY     0x8000U   /* Reserved Factory Mode              */
#define KSZ8081_OMSO_B_CAST_OFF       0x0200U   /* B-CAST_OFF Override                */
#define KSZ8081_OMSO_RMII_B_TO_B      0x0040U   /* RMII B-to-B Override               */
#define KSZ8081_OMSO_NAND_TREE        0x0020U   /* NAND Tree Override                 */
#define KSZ8081_OMSO_RMII             0x0002U   /* RMII Override                      */

/* Operation Mode Strap Status Register Bitmasks */
#define KSZ8081_OMSS_PHYAD            0xE000U   /* PHYAD[2:0] Strap-In Status         */
#define KSZ8081_OMSS_RMII             0x0001U   /* RMII Strap-In Status               */

/* Expanded Control Register Bitmasks */
#define KSZ8081_EXCTRL_EDPD           0x0800U   /* EDPD Disabled                      */

/* clear Control/Status Register Bitmasks */
#define KSZ8081_IRQCS_JABBER_IE       0x8000U   /* Jabber Interrupt Enable            */
#define KSZ8081_IRQCS_RXERR_IE        0x4000U   /* Receive Error Interrupt Enable     */
#define KSZ8081_IRQCS_PGRCVD_IE       0x2000U   /* Page Received Interrupt Enable     */
#define KSZ8081_IRQCS_PDF_IE          0x1000U   /* Parallel Detect Fault Int. Enable  */
#define KSZ8081_IRQCS_LP_ACK_IE       0x0800U   /* Link Partner Ack. Interrupt Enable */
#define KSZ8081_IRQCS_LINK_DOWN_IE    0x0400U   /* Link-Down Interrupt Enable         */
#define KSZ8081_IRQCS_RFAULT_IE       0x0200U   /* Remote Fault Interrupt Enable      */
#define KSZ8081_IRQCS_LINK_UP_IE      0x0100U   /* Link-Up Interrupt Enable           */
#define KSZ8081_IRQCS_JABBER_IRQ      0x0080U   /* Jabber Interrupt                   */
#define KSZ8081_IRQCS_RXERR_IRQ       0x0040U   /* Receive Error Interrupt            */
#define KSZ8081_IRQCS_PGRCVD_IRQ      0x0020U   /* Page Receive Interrupt             */
#define KSZ8081_IRQCS_PDF_IRQ         0x0010U   /* Parallel Detect Fault Interrupt    */
#define KSZ8081_IRQCS_LP_ACK_IRQ      0x0008U   /* Link Partner Acknowledge Interrupt */
#define KSZ8081_IRQCS_LINK_DOWN_IRQ   0x0004U   /* Link-Down Interrupt                */
#define KSZ8081_IRQCS_RFAULT_IRQ      0x0002U   /* Remote Fault Interrupt             */
#define KSZ8081_IRQCS_LINK_UP_IRQ     0x0001U   /* Link-Up Interrupt                  */

/* LinkMD Control/Status Register Bitmasks */
#define KSZ8081_LMDCS_CABLE_TEST_EN   0x8000U   /* Cable Diagnostic Test Enable       */
#define KSZ8081_LMDCS_CABLE_TEST_RES  0x6000U   /* Cable Diagnostic Test Result       */
#define KSZ8081_LMDCS_SHORT_CABLE     0x1000U   /* Short Cable Indicator              */
#define KSZ8081_LMDCS_CABLE_FAULT_CNT 0x00FFU   /* Cable Fault Counter                */

/* PHY Control 1 Register Bitmasks */
#define KSZ8081_PHYCR1_EN_PAUSE       0x0200U   /* Enable Pause (Flow Control)        */
#define KSZ8081_PHYCR1_LINK_STAT      0x0100U   /* Link Status                        */
#define KSZ8081_PHYCR1_POLARITY_STAT  0x0080U   /* Polarity Status                    */
#define KSZ8081_PHYCR1_MDI_STATE      0x0020U   /* MDI/MDI-X State                    */
#define KSZ8081_PHYCR1_ENERGY_DETECT  0x0010U   /* Energy Detect                      */
#define KSZ8081_PHYCR1_PHY_ISOLATE    0x0008U   /* PHY Isolate                        */
#define KSZ8081_PHYCR1_OPERATION_MODE 0x0007U   /* Operation Mode Indication          */

/* PHY Control 1 Operation Mode Bitmasks */
#define KSZ8081_PHYCR1_OM_100B        0x0002U   /* 100Base-TX bitmask                 */
#define KSZ8081_PHYCR1_OM_FD          0x0004U   /* Full-duplex bitmask                */

/* PHY Control 2 Register Bitmasks */
#define KSZ8081_PHYCR2_HP_MDIX        0x8000U   /* HP_MDIX                            */
#define KSZ8081_PHYCR2_MDI_SELECT     0x4000U   /* MDI/MDI-X Select                   */
#define KSZ8081_PHYCR2_PAIR_SWAP_DIS  0x2000U   /* Pair Swap Disable                  */
#define KSZ8081_PHYCR2_FORCE_LINK     0x0800U   /* Force Link                         */
#define KSZ8081_PHYCR2_POWER_SAVING   0x0400U   /* Power Saving                       */
#define KSZ8081_PHYCR2_IRQ_LEVEL      0x0200U   /* Interrupt Level                    */
#define KSZ8081_PHYCR2_EN_JABBER      0x0100U   /* Enable Jabber                      */
#define KSZ8081_PHYCR2_REF_CLK_SELECT 0x0080U   /* RMII Reference Clock Select        */
#define KSZ8081_PHYCR2_LED_MODE       0x0030U   /* LED Mode                           */
#define KSZ8081_PHYCR2_DIS_TX         0x0008U   /* Disable Transmitter                */
#define KSZ8081_PHYCR2_REM_LOOPBACK   0x0004U   /* Remote Loopback                    */
#define KSZ8081_PHYCR2_DIS_DATA_SCR   0x0001U   /* Disable Data Scrambling            */

/* PHY Driver State Flags */
#define KSZ8081_PHY_INIT              0x01U     /* Driver initialized                */
#define KSZ8081_PHY_POWER             0x02U     /* Driver power is on                */

/** @defgroup KSZ8081_Status KSZ8081 Status
  * @{
  */

#define  KSZ8081_STATUS_READ_ERROR            ((int32_t)-5)
#define  KSZ8081_STATUS_WRITE_ERROR           ((int32_t)-4)
#define  KSZ8081_STATUS_ADDRESS_ERROR         ((int32_t)-3)
#define  KSZ8081_STATUS_RESET_TIMEOUT         ((int32_t)-2)
#define  KSZ8081_STATUS_ERROR                 ((int32_t)-1)
#define  KSZ8081_STATUS_OK                    ((int32_t) 0)
#define  KSZ8081_STATUS_LINK_DOWN             ((int32_t) 1)
#define  KSZ8081_STATUS_100MBITS_FULLDUPLEX   ((int32_t) 2)
#define  KSZ8081_STATUS_100MBITS_HALFDUPLEX   ((int32_t) 3)
#define  KSZ8081_STATUS_10MBITS_FULLDUPLEX    ((int32_t) 4)
#define  KSZ8081_STATUS_10MBITS_HALFDUPLEX    ((int32_t) 5)
#define  KSZ8081_STATUS_AUTONEGO_NOTDONE      ((int32_t) 6)
/**
  * @}
  */

/* Exported types ------------------------------------------------------------*/
/** @defgroup ksz8081_Exported_Types ksz8081 Exported Types
  * @{
  */
typedef int32_t  (*ksz8081_Init_Func) (void);
typedef int32_t  (*ksz8081_DeInit_Func) (void);
typedef int32_t  (*ksz8081_ReadReg_Func)   (uint32_t, uint32_t, uint32_t *);
typedef int32_t  (*ksz8081_WriteReg_Func)  (uint32_t, uint32_t, uint32_t);
typedef int32_t  (*ksz8081_GetTick_Func)  (void);

typedef struct
{
  ksz8081_Init_Func      Init;
  ksz8081_DeInit_Func    DeInit;
  ksz8081_WriteReg_Func  WriteReg;
  ksz8081_ReadReg_Func   ReadReg;
  ksz8081_GetTick_Func   GetTick;
} ksz8081_IOCtx_t;

typedef struct
{
  uint32_t            DevAddr;
  uint32_t            Is_Initialized;
  ksz8081_IOCtx_t     IO;
  void               *pData;
}ksz8081_Object_t;

/* Exported macro ------------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
/** @defgroup KSZ8081_Exported_Functions KSZ8081 Exported Functions
  * @{
  */
int32_t KSZ8081_RegisterBusIO(ksz8081_Object_t *pObj, ksz8081_IOCtx_t *ioctx);
int32_t KSZ8081_Init(ksz8081_Object_t *pObj);
int32_t KSZ8081_DeInit(ksz8081_Object_t *pObj);
int32_t KSZ8081_DisablePowerDownMode(ksz8081_Object_t *pObj);
int32_t KSZ8081_EnablePowerDownMode(ksz8081_Object_t *pObj);
int32_t KSZ8081_StartAutoNego(ksz8081_Object_t *pObj);
int32_t KSZ8081_GetLinkState(ksz8081_Object_t *pObj);
int32_t KSZ8081_SetLinkState(ksz8081_Object_t *pObj, uint32_t LinkState);
int32_t KSZ8081_EnableLoopbackMode(ksz8081_Object_t *pObj);
int32_t KSZ8081_DisableLoopbackMode(ksz8081_Object_t *pObj);
int32_t KSZ8081_EnableIT(ksz8081_Object_t *pObj, uint32_t Interrupt);
int32_t KSZ8081_DisableIT(ksz8081_Object_t *pObj, uint32_t Interrupt);
int32_t KSZ8081_ClearIT(ksz8081_Object_t *pObj, uint32_t Interrupt);
int32_t KSZ8081_GetITStatus(ksz8081_Object_t *pObj, uint32_t Interrupt);
/**
  * @}
  */

#endif /* __PHY_KSZ8081RNA_H */
