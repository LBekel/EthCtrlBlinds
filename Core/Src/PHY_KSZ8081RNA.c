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
 * Driver:       Driver_ETH_PHYn (default: Driver_ETH_PHY0)
 * Project:      Ethernet Physical Layer Transceiver (PHY)
 *               Driver for KSZ8081RNA (KSZ8081RNB compatible)
 * -----------------------------------------------------------------------
 * Use the following configuration settings in the middleware component
 * to connect to this driver.
 *
 *   Configuration Setting                     Value
 *   ---------------------                     -----
 *   Connect to hardware via Driver_ETH_PHY# = n (default: 0)
 * -------------------------------------------------------------------- */

/* History:
 *  Version 6.3
 *    Added reference clock select configuration option (ETH_PHY_REF_CLK_50M)
 *  Version 6.2
 *    Updated for ARM compiler 6
 *  Version 6.1
 *    Added driver flow control flags
 *  Version 6.0
 *    Initial release
 */

#include "../Inc/PHY_KSZ8081RNA.h"
#include <stddef.h>

//#ifndef ETH_PHY_NUM
//#define ETH_PHY_NUM          0        /* Default driver number */
//#endif

#ifndef ETH_PHY_ADDR
#define ETH_PHY_ADDR         0x00     /* Default device address */
#endif

#ifndef ETH_PHY_REF_CLK_50M
#define ETH_PHY_REF_CLK_50M  0        /* RMII Reference Clock (0:25MHz,1:50MHz) */
#endif

#define KSZ8081_MAX_DEV_ADDR   ((uint32_t)31U)



/**
  * @brief  Register IO functions to component object
  * @param  pObj: device object  of KSZ8081_Object_t.
  * @param  ioctx: holds device IO functions.
  * @retval KSZ8081_STATUS_OK  if OK
  *         KSZ8081_STATUS_ERROR if missing mandatory function
  */
int32_t  KSZ8081_RegisterBusIO(ksz8081_Object_t *pObj, ksz8081_IOCtx_t *ioctx)
{
  if(!pObj || !ioctx->ReadReg || !ioctx->WriteReg || !ioctx->GetTick)
  {
    return KSZ8081_STATUS_ERROR;
  }

  pObj->IO.Init = ioctx->Init;
  pObj->IO.DeInit = ioctx->DeInit;
  pObj->IO.ReadReg = ioctx->ReadReg;
  pObj->IO.WriteReg = ioctx->WriteReg;
  pObj->IO.GetTick = ioctx->GetTick;

  return KSZ8081_STATUS_OK;
}

/**
  * @brief  Initialize the ksz8081 and configure the needed hardware resources
  * @param  pObj: device object KSZ8081_Object_t.
  * @retval KSZ8081_STATUS_OK  if OK
  *         KSZ8081_STATUS_ADDRESS_ERROR if cannot find device address
  *         KSZ8081_STATUS_READ_ERROR if cannot read register
  */
 int32_t KSZ8081_Init(ksz8081_Object_t *pObj)
 {
   uint32_t regvalue = 0, regvalue2 = 0, addr = 0;
   uint32_t addrs[3] = {ETH_PHY_ADDR, 0x00U, 0x03U};
   int32_t status = KSZ8081_STATUS_OK;

   if(pObj->Is_Initialized == 0)
   {
     if(pObj->IO.Init != 0)
     {
       /* GPIO and Clocks initialization */
       pObj->IO.Init();
     }


    status = KSZ8081_STATUS_ADDRESS_ERROR;
    for(size_t i = 0; i < (sizeof(addrs) / sizeof(addrs[0])); i++)
    {
      addr = addrs[i];
      if(pObj->IO.ReadReg(addr, KSZ8081_PHYIDR1, &regvalue) < 0)
      {
        status = KSZ8081_STATUS_READ_ERROR;
        continue;
      }
      if(pObj->IO.ReadReg(addr, KSZ8081_PHYIDR2, &regvalue2) < 0)
      {
        status = KSZ8081_STATUS_READ_ERROR;
        continue;
      }

      if((regvalue == KSZ8081_PHY_ID1) && ((regvalue2 & 0xFFF0U) == (KSZ8081_PHY_ID2 & 0xFFF0U)))
      {
        pObj->DevAddr = addr;
        status = KSZ8081_STATUS_OK;
        pObj->Is_Initialized = 1;
        break;
      }
    }

    if(status == KSZ8081_STATUS_OK)
    {
      if(pObj->IO.ReadReg(pObj->DevAddr, KSZ8081_PHYCR2, &regvalue) >= 0)
      {
        if(ETH_PHY_REF_CLK_50M != 0)
        {
          regvalue |= KSZ8081_PHYCR2_REF_CLK_SELECT;
        }
        else
        {
          regvalue &= ~KSZ8081_PHYCR2_REF_CLK_SELECT;
        }
        (void)pObj->IO.WriteReg(pObj->DevAddr, KSZ8081_PHYCR2, regvalue);
      }
    }

   }

   return status;
 }

 /**
   * @brief  De-Initialize the ksz8081 and it's hardware resources
   * @param  pObj: device object KSZ8081_Object_t.
   * @retval None
   */
 int32_t KSZ8081_DeInit(ksz8081_Object_t *pObj)
 {
   if(pObj->Is_Initialized)
   {
     if(pObj->IO.DeInit != 0)
     {
       if(pObj->IO.DeInit() < 0)
       {
         return KSZ8081_STATUS_ERROR;
       }
     }

     pObj->Is_Initialized = 0;
   }

   return KSZ8081_STATUS_OK;
 }

 /**
   * @brief  Disable the KSZ8081 power down mode.
   * @param  pObj: device object KSZ8081_Object_t.
   * @retval KSZ8081_STATUS_OK  if OK
   *         KSZ8081_STATUS_READ_ERROR if cannot read register
   *         KSZ8081_STATUS_WRITE_ERROR if cannot write to register
   */
 int32_t KSZ8081_DisablePowerDownMode(ksz8081_Object_t *pObj)
 {
   uint32_t readval = 0;
   int32_t status = KSZ8081_STATUS_OK;

   if(pObj->IO.ReadReg(pObj->DevAddr, KSZ8081_BMCR, &readval) >= 0)
   {
     readval &= ~KSZ8081_BMCR_POWER_DOWN;

     /* Apply configuration */
     if(pObj->IO.WriteReg(pObj->DevAddr, KSZ8081_BMCR, readval) < 0)
     {
       status =  KSZ8081_STATUS_WRITE_ERROR;
     }
   }
   else
   {
     status = KSZ8081_STATUS_READ_ERROR;
   }

   return status;
 }

 /**
   * @brief  Enable the KSZ8081 power down mode.
   * @param  pObj: device object KSZ8081_Object_t.
   * @retval KSZ8081_STATUS_OK  if OK
   *         KSZ8081_STATUS_READ_ERROR if cannot read register
   *         KSZ8081_STATUS_WRITE_ERROR if cannot write to register
   */
 int32_t KSZ8081_EnablePowerDownMode(ksz8081_Object_t *pObj)
 {
   uint32_t readval = 0;
   int32_t status = KSZ8081_STATUS_OK;

   if(pObj->IO.ReadReg(pObj->DevAddr, KSZ8081_BMCR, &readval) >= 0)
   {
     readval |= KSZ8081_BMCR_POWER_DOWN;

     /* Apply configuration */
     if(pObj->IO.WriteReg(pObj->DevAddr, KSZ8081_BMCR, readval) < 0)
     {
       status =  KSZ8081_STATUS_WRITE_ERROR;
     }
   }
   else
   {
     status = KSZ8081_STATUS_READ_ERROR;
   }

   return status;
 }

 /**
   * @brief  Start the auto negotiation process.
   * @param  pObj: device object KSZ8081_Object_t.
   * @retval KSZ8081_STATUS_OK  if OK
   *         KSZ8081_STATUS_READ_ERROR if cannot read register
   *         KSZ8081_STATUS_WRITE_ERROR if cannot write to register
   */
 int32_t KSZ8081_StartAutoNego(ksz8081_Object_t *pObj)
 {
   uint32_t readval = 0;
   int32_t status = KSZ8081_STATUS_OK;

   if(pObj->IO.ReadReg(pObj->DevAddr, KSZ8081_BMCR, &readval) >= 0)
   {
     readval |= (KSZ8081_BMCR_ANEG_EN | KSZ8081_BMCR_RESTART_ANEG);

     /* Apply configuration */
     if(pObj->IO.WriteReg(pObj->DevAddr, KSZ8081_BMCR, readval) < 0)
     {
       status =  KSZ8081_STATUS_WRITE_ERROR;
     }
   }
   else
   {
     status = KSZ8081_STATUS_READ_ERROR;
   }

   return status;
 }

 /**
   * @brief  Get the link state of KSZ8081 device.
   * @param  pObj: Pointer to device object.
   * @param  pLinkState: Pointer to link state
   * @retval KSZ8081_STATUS_LINK_DOWN  if link is down
   *         KSZ8081_STATUS_AUTONEGO_NOTDONE if Auto nego not completed
   *         KSZ8081_STATUS_100MBITS_FULLDUPLEX if 100Mb/s FD
   *         KSZ8081_STATUS_100MBITS_HALFDUPLEX if 100Mb/s HD
   *         KSZ8081_STATUS_10MBITS_FULLDUPLEX  if 10Mb/s FD
   *         KSZ8081_STATUS_10MBITS_HALFDUPLEX  if 10Mb/s HD
   *         KSZ8081_STATUS_READ_ERROR if cannot read register
   *         KSZ8081_STATUS_WRITE_ERROR if cannot write to register
   */
 int32_t KSZ8081_GetLinkState(ksz8081_Object_t *pObj)
 {
   uint32_t readval = 0;

   /* Read Status register  */
   if(pObj->IO.ReadReg(pObj->DevAddr, KSZ8081_BMSR, &readval) < 0)
   {
     return KSZ8081_STATUS_READ_ERROR;
   }

   /* Read Status register again */
   if(pObj->IO.ReadReg(pObj->DevAddr, KSZ8081_BMSR, &readval) < 0)
   {
     return KSZ8081_STATUS_READ_ERROR;
   }

   if((readval & KSZ8081_BMSR_LINK_STAT) == 0)
   {
     /* Return Link Down status */
     return KSZ8081_STATUS_LINK_DOWN;
   }

   /* Check Auto negotiation */
   if(pObj->IO.ReadReg(pObj->DevAddr, KSZ8081_BMCR, &readval) < 0)
   {
     return KSZ8081_STATUS_READ_ERROR;
   }

   if((readval & KSZ8081_BMCR_ANEG_EN) != KSZ8081_BMCR_ANEG_EN)
   {
     if(((readval & KSZ8081_BMCR_SPEED_SELECT) == KSZ8081_BMCR_SPEED_SELECT) && ((readval & KSZ8081_BMCR_DUPLEX_MODE) == KSZ8081_BMCR_DUPLEX_MODE))
     {
       return KSZ8081_STATUS_100MBITS_FULLDUPLEX;
     }
     else if ((readval & KSZ8081_BMCR_SPEED_SELECT) == KSZ8081_BMCR_SPEED_SELECT)
     {
       return KSZ8081_STATUS_100MBITS_HALFDUPLEX;
     }
     else if ((readval & KSZ8081_BMCR_DUPLEX_MODE) == KSZ8081_BMCR_DUPLEX_MODE)
     {
       return KSZ8081_STATUS_10MBITS_FULLDUPLEX;
     }
     else
     {
       return KSZ8081_STATUS_10MBITS_HALFDUPLEX;
     }
   }
   else /* Auto Nego enabled */
   {
     if(pObj->IO.ReadReg(pObj->DevAddr, KSZ8081_BMSR, &readval) < 0)
     {
       return KSZ8081_STATUS_READ_ERROR;
     }

     /* Check if auto nego not done */
     if((readval & KSZ8081_BMSR_ANEG_COMPLETE) == 0)
     {
       return KSZ8081_STATUS_AUTONEGO_NOTDONE;
     }

     if(pObj->IO.ReadReg(pObj->DevAddr, KSZ8081_PHYCR1, &readval) < 0)
     {
       return KSZ8081_STATUS_READ_ERROR;
     }

     readval &= KSZ8081_PHYCR1_OPERATION_MODE;
     if(readval == 0x0U)
     {
       return KSZ8081_STATUS_AUTONEGO_NOTDONE;
     }
     if((readval & (KSZ8081_PHYCR1_OM_100B | KSZ8081_PHYCR1_OM_FD)) == (KSZ8081_PHYCR1_OM_100B | KSZ8081_PHYCR1_OM_FD))
     {
       return KSZ8081_STATUS_100MBITS_FULLDUPLEX;
     }
     if((readval & KSZ8081_PHYCR1_OM_100B) == KSZ8081_PHYCR1_OM_100B)
     {
       return KSZ8081_STATUS_100MBITS_HALFDUPLEX;
     }
     if((readval & KSZ8081_PHYCR1_OM_FD) == KSZ8081_PHYCR1_OM_FD)
     {
       return KSZ8081_STATUS_10MBITS_FULLDUPLEX;
     }
     if(readval == 0x01U)
     {
       return KSZ8081_STATUS_10MBITS_HALFDUPLEX;
     }

     return KSZ8081_STATUS_AUTONEGO_NOTDONE;
   }
 }

 /**
   * @brief  Set the link state of KSZ8081 device.
   * @param  pObj: Pointer to device object.
   * @param  pLinkState: link state can be one of the following
   *         KSZ8081_STATUS_100MBITS_FULLDUPLEX if 100Mb/s FD
   *         KSZ8081_STATUS_100MBITS_HALFDUPLEX if 100Mb/s HD
   *         KSZ8081_STATUS_10MBITS_FULLDUPLEX  if 10Mb/s FD
   *         KSZ8081_STATUS_10MBITS_HALFDUPLEX  if 10Mb/s HD
   * @retval KSZ8081_STATUS_OK  if OK
   *         KSZ8081_STATUS_ERROR  if parameter error
   *         KSZ8081_STATUS_READ_ERROR if cannot read register
   *         KSZ8081_STATUS_WRITE_ERROR if cannot write to register
   */
 int32_t KSZ8081_SetLinkState(ksz8081_Object_t *pObj, uint32_t LinkState)
 {
   uint32_t bcrvalue = 0;
   int32_t status = KSZ8081_STATUS_OK;

   if(pObj->IO.ReadReg(pObj->DevAddr, KSZ8081_BMCR, &bcrvalue) >= 0)
   {
     /* Disable link config (Auto nego, speed and duplex) */
     bcrvalue &= ~(KSZ8081_BMCR_ANEG_EN | KSZ8081_BMCR_SPEED_SELECT | KSZ8081_BMCR_DUPLEX_MODE);

     if(LinkState == KSZ8081_STATUS_100MBITS_FULLDUPLEX)
     {
       bcrvalue |= (KSZ8081_BMCR_SPEED_SELECT | KSZ8081_BMCR_DUPLEX_MODE);
     }
     else if (LinkState == KSZ8081_STATUS_100MBITS_HALFDUPLEX)
     {
       bcrvalue |= KSZ8081_BMCR_SPEED_SELECT;
     }
     else if (LinkState == KSZ8081_STATUS_10MBITS_FULLDUPLEX)
     {
       bcrvalue |= KSZ8081_BMCR_DUPLEX_MODE;
     }
     else
     {
       /* Wrong link status parameter */
       status = KSZ8081_STATUS_ERROR;
     }
   }
   else
   {
     status = KSZ8081_STATUS_READ_ERROR;
   }

   if(status == KSZ8081_STATUS_OK)
   {
     /* Apply configuration */
     if(pObj->IO.WriteReg(pObj->DevAddr, KSZ8081_BMCR, bcrvalue) < 0)
     {
       status = KSZ8081_STATUS_WRITE_ERROR;
     }
   }

   return status;
 }

 /**
   * @brief  Enable loopback mode.
   * @param  pObj: Pointer to device object.
   * @retval KSZ8081_STATUS_OK  if OK
   *         KSZ8081_STATUS_READ_ERROR if cannot read register
   *         KSZ8081_STATUS_WRITE_ERROR if cannot write to register
   */
 int32_t KSZ8081_EnableLoopbackMode(ksz8081_Object_t *pObj)
 {
   uint32_t readval = 0;
   int32_t status = KSZ8081_STATUS_OK;

   if(pObj->IO.ReadReg(pObj->DevAddr, KSZ8081_BMCR, &readval) >= 0)
   {
     readval |= KSZ8081_BMCR_LOOPBACK;

     /* Apply configuration */
     if(pObj->IO.WriteReg(pObj->DevAddr, KSZ8081_BMCR, readval) < 0)
     {
       status = KSZ8081_STATUS_WRITE_ERROR;
     }
   }
   else
   {
     status = KSZ8081_STATUS_READ_ERROR;
   }

   return status;
 }

 /**
   * @brief  Disable loopback mode.
   * @param  pObj: Pointer to device object.
   * @retval KSZ8081_STATUS_OK  if OK
   *         KSZ8081_STATUS_READ_ERROR if cannot read register
   *         KSZ8081_STATUS_WRITE_ERROR if cannot write to register
   */
 int32_t KSZ8081_DisableLoopbackMode(ksz8081_Object_t *pObj)
 {
   uint32_t readval = 0;
   int32_t status = KSZ8081_STATUS_OK;

   if(pObj->IO.ReadReg(pObj->DevAddr, KSZ8081_BMCR, &readval) >= 0)
   {
     readval &= ~KSZ8081_BMCR_LOOPBACK;

     /* Apply configuration */
     if(pObj->IO.WriteReg(pObj->DevAddr, KSZ8081_BMCR, readval) < 0)
     {
       status =  KSZ8081_STATUS_WRITE_ERROR;
     }
   }
   else
   {
     status = KSZ8081_STATUS_READ_ERROR;
   }

   return status;
 }

 /**
   * @brief  Enable IT source.
   * @param  pObj: Pointer to device object.
   * @param  Interrupt: IT source to be enabled
   *         should be a value or a combination of the following:
   *         KSZ8081_WOL_IT
   *         KSZ8081_ENERGYON_IT
   *         KSZ8081_AUTONEGO_COMPLETE_IT
   *         KSZ8081_REMOTE_FAULT_IT
   *         KSZ8081_LINK_DOWN_IT
   *         KSZ8081_AUTONEGO_LP_ACK_IT
   *         KSZ8081_PARALLEL_DETECTION_FAULT_IT
   *         KSZ8081_AUTONEGO_PAGE_RECEIVED_IT
   * @retval KSZ8081_STATUS_OK  if OK
   *         KSZ8081_STATUS_READ_ERROR if cannot read register
   *         KSZ8081_STATUS_WRITE_ERROR if cannot write to register
   */
 int32_t KSZ8081_EnableIT(ksz8081_Object_t *pObj, uint32_t Interrupt)
 {
   uint32_t readval = 0;
   int32_t status = KSZ8081_STATUS_OK;

   if(pObj->IO.ReadReg(pObj->DevAddr, KSZ8081_IRQCS, &readval) >= 0)
   {
     readval |= Interrupt;

     /* Apply configuration */
     if(pObj->IO.WriteReg(pObj->DevAddr, KSZ8081_IRQCS, readval) < 0)
     {
       status =  KSZ8081_STATUS_WRITE_ERROR;
     }
   }
   else
   {
     status = KSZ8081_STATUS_READ_ERROR;
   }

   return status;
 }

 /**
   * @brief  Disable IT source.
   * @param  pObj: Pointer to device object.
   * @param  Interrupt: IT source to be disabled
   *         should be a value or a combination of the following:
   *         KSZ8081_WOL_IT
   *         KSZ8081_ENERGYON_IT
   *         KSZ8081_AUTONEGO_COMPLETE_IT
   *         KSZ8081_REMOTE_FAULT_IT
   *         KSZ8081_LINK_DOWN_IT
   *         KSZ8081_AUTONEGO_LP_ACK_IT
   *         KSZ8081_PARALLEL_DETECTION_FAULT_IT
   *         KSZ8081_AUTONEGO_PAGE_RECEIVED_IT
   * @retval KSZ8081_STATUS_OK  if OK
   *         KSZ8081_STATUS_READ_ERROR if cannot read register
   *         KSZ8081_STATUS_WRITE_ERROR if cannot write to register
   */
 int32_t KSZ8081_DisableIT(ksz8081_Object_t *pObj, uint32_t Interrupt)
 {
   uint32_t readval = 0;
   int32_t status = KSZ8081_STATUS_OK;

   if(pObj->IO.ReadReg(pObj->DevAddr, KSZ8081_IRQCS, &readval) >= 0)
   {
     readval &= ~Interrupt;

     /* Apply configuration */
     if(pObj->IO.WriteReg(pObj->DevAddr, KSZ8081_IRQCS, readval) < 0)
     {
       status = KSZ8081_STATUS_WRITE_ERROR;
     }
   }
   else
   {
     status = KSZ8081_STATUS_READ_ERROR;
   }

   return status;
 }

 /**
   * @brief  Clear IT flag.
   * @param  pObj: Pointer to device object.
   * @param  Interrupt: IT flag to be cleared
   *         should be a value or a combination of the following:
   *         KSZ8081_WOL_IT
   *         KSZ8081_ENERGYON_IT
   *         KSZ8081_AUTONEGO_COMPLETE_IT
   *         KSZ8081_REMOTE_FAULT_IT
   *         KSZ8081_LINK_DOWN_IT
   *         KSZ8081_AUTONEGO_LP_ACK_IT
   *         KSZ8081_PARALLEL_DETECTION_FAULT_IT
   *         KSZ8081_AUTONEGO_PAGE_RECEIVED_IT
   * @retval KSZ8081_STATUS_OK  if OK
   *         KSZ8081_STATUS_READ_ERROR if cannot read register
   */
 int32_t  KSZ8081_ClearIT(ksz8081_Object_t *pObj, uint32_t Interrupt)
 {

   int32_t status = KSZ8081_STATUS_OK;

// not needed interrupts cleared after read

   return status;
 }

 /**
   * @brief  Get IT Flag status.
   * @param  pObj: Pointer to device object.
   * @param  Interrupt: IT Flag to be checked,
   *         should be a value or a combination of the following:
   *         KSZ8081_WOL_IT
   *         KSZ8081_ENERGYON_IT
   *         KSZ8081_AUTONEGO_COMPLETE_IT
   *         KSZ8081_REMOTE_FAULT_IT
   *         KSZ8081_LINK_DOWN_IT
   *         KSZ8081_AUTONEGO_LP_ACK_IT
   *         KSZ8081_PARALLEL_DETECTION_FAULT_IT
   *         KSZ8081_AUTONEGO_PAGE_RECEIVED_IT
   * @retval 1 IT flag is SET
   *         0 IT flag is RESET
   *         KSZ8081_STATUS_READ_ERROR if cannot read register
   */
 int32_t KSZ8081_GetITStatus(ksz8081_Object_t *pObj, uint32_t Interrupt)
 {
   uint32_t readval = 0;
   int32_t status = 0;

   if(pObj->IO.ReadReg(pObj->DevAddr, KSZ8081_IRQCS, &readval) >= 0)
   {
     status = ((readval & Interrupt) == Interrupt);
   }
   else
   {
     status = KSZ8081_STATUS_READ_ERROR;
   }

   return status;
 }



