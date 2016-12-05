/******************** (C) COPYRIGHT 2011 STMicroelectronics ********************
* File Name          : usb_istr.c
* Author             : MCD Application Team
* Version            : V3.3.0
* Date               : 21-March-2011
* Description        : ISTR events interrupt service routines
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "usb_lib.h"
#include "usb_prop.h"
#include "usb_pwr.h"
#include "usb_istr.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
__IO uint16_t wIstr;  /* ISTR register last read value */
__IO uint8_t bIntPackSOF = 0;  /* SOFs received between 2 consecutive packets */

/* Extern variables ----------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/* function pointers to non-control endpoints service routines */
void (*pEpInt_IN[7])(void) =
  {
    EP1_IN_Callback,
    EP2_IN_Callback,
    EP3_IN_Callback,
    EP4_IN_Callback,
    EP5_IN_Callback,
    EP6_IN_Callback,
    EP7_IN_Callback,
  };

void (*pEpInt_OUT[7])(void) =
  {
    EP1_OUT_Callback,
    EP2_OUT_Callback,
    EP3_OUT_Callback,
    EP4_OUT_Callback,
    EP5_OUT_Callback,
    EP6_OUT_Callback,
    EP7_OUT_Callback,
  };

#ifndef STM32F10X_CL

/*******************************************************************************
* Function Name  : USB_Istr
* Description    : STR events interrupt service routine
* Input          :
* Output         :
* Return         :
*******************************************************************************/
void USB_Istr(void)
{

  wIstr = _GetISTR();

#if (IMR_MSK & ISTR_SOF)
  if (wIstr & ISTR_SOF & wInterrupt_Mask)
  {
    _SetISTR((uint16_t)CLR_SOF);
    bIntPackSOF++;

#ifdef SOF_CALLBACK
    SOF_Callback();
#endif
  }
#endif
  /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/  
  
#if (IMR_MSK & ISTR_CTR)
  if (wIstr & ISTR_CTR & wInterrupt_Mask)
  {
    /* servicing of the endpoint correct transfer interrupt */
    /* clear of the CTR flag into the sub */
    CTR_LP();
#ifdef CTR_CALLBACK
    CTR_Callback();
#endif
  }
#endif
  /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/  
#if (IMR_MSK & ISTR_RESET)
  if (wIstr & ISTR_RESET & wInterrupt_Mask)
  {
    _SetISTR((uint16_t)CLR_RESET);
    Device_Property.Reset();
#ifdef RESET_CALLBACK
    RESET_Callback();
#endif
  }
#endif
  /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if (IMR_MSK & ISTR_DOVR)
  if (wIstr & ISTR_DOVR & wInterrupt_Mask)
  {
    _SetISTR((uint16_t)CLR_DOVR);
#ifdef DOVR_CALLBACK
    DOVR_Callback();
#endif
  }
#endif
  /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if (IMR_MSK & ISTR_ERR)
  if (wIstr & ISTR_ERR & wInterrupt_Mask)
  {
    _SetISTR((uint16_t)CLR_ERR);
#ifdef ERR_CALLBACK
    ERR_Callback();
#endif
  }
#endif
  /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if (IMR_MSK & ISTR_WKUP)
  if (wIstr & ISTR_WKUP & wInterrupt_Mask)
  {
    _SetISTR((uint16_t)CLR_WKUP);
    Resume(RESUME_EXTERNAL);
#ifdef WKUP_CALLBACK
    WKUP_Callback();
#endif
  }
#endif
  /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#if (IMR_MSK & ISTR_SUSP)
  if (wIstr & ISTR_SUSP & wInterrupt_Mask)
  {

    /* check if SUSPEND is possible */
    if (fSuspendEnabled)
    {
      Suspend();
    }
    else
    {
      /* if not possible then resume after xx ms */
      Resume(RESUME_LATER);
    }
    /* clear of the ISTR bit must be done after setting of CNTR_FSUSP */
    _SetISTR((uint16_t)CLR_SUSP);
#ifdef SUSP_CALLBACK
    SUSP_Callback();
#endif
  }
#endif
  /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/

#if (IMR_MSK & ISTR_ESOF)
  if (wIstr & ISTR_ESOF & wInterrupt_Mask)
  {
    _SetISTR((uint16_t)CLR_ESOF);
    /* resume handling timing is made with ESOFs */
    Resume(RESUME_ESOF); /* request without change of the machine state */

#ifdef ESOF_CALLBACK
    ESOF_Callback();
#endif
  }
#endif
} /* USB_Istr */

/*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
#else /* STM32F10X_CL */


/*******************************************************************************
* Function Name  : STM32_PCD_OTG_ISR_Handler
* Description    : Handles all USB Device Interrupts
* Input          : None
* Output         : None
* Return         : status
*******************************************************************************/
u32 STM32_PCD_OTG_ISR_Handler (void)
{
  USB_OTG_GINTSTS_TypeDef gintr_status;
  u32 retval = 0;

  if (USBD_FS_IsDeviceMode()) /* ensure that we are in device mode */
  {
    gintr_status.d32 = OTGD_FS_ReadCoreItr();

   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
    
    /* If there is no interrupt pending exit the interrupt routine */
    if (!gintr_status.d32)
    {
      return 0;
    }

   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/    
    /* Early Suspend interrupt */ 
#ifdef INTR_ERLYSUSPEND
    if (gintr_status.b.erlysuspend)
    {
      retval |= OTGD_FS_Handle_EarlySuspend_ISR();
    }
#endif /* INTR_ERLYSUSPEND */
    
   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
    /* End of Periodic Frame interrupt */
#ifdef INTR_EOPFRAME    
    if (gintr_status.b.eopframe)
    {
      retval |= OTGD_FS_Handle_EOPF_ISR();
    }
#endif /* INTR_EOPFRAME */
    
   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
    /* Non Periodic Tx FIFO Empty interrupt */
#ifdef INTR_NPTXFEMPTY    
    if (gintr_status.b.nptxfempty)
    {
      retval |= OTGD_FS_Handle_NPTxFE_ISR();
    }
#endif /* INTR_NPTXFEMPTY */
    
   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/    
    /* Wakeup or RemoteWakeup interrupt */
#ifdef INTR_WKUPINTR    
    if (gintr_status.b.wkupintr)
    {   
      retval |= OTGD_FS_Handle_Wakeup_ISR();
    }
#endif /* INTR_WKUPINTR */   
    
   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
    /* Suspend interrupt */
#ifdef INTR_USBSUSPEND
    if (gintr_status.b.usbsuspend)
    { 
      /* check if SUSPEND is possible */
      if (fSuspendEnabled)
      {
        Suspend();
      }
      else
      {
        /* if not possible then resume after xx ms */
        Resume(RESUME_LATER); /* This case shouldn't happen in OTG Device mode because 
        there's no ESOF interrupt to increment the ResumeS.bESOFcnt in the Resume state machine */
      }
            
      retval |= OTGD_FS_Handle_USBSuspend_ISR();
    }
#endif /* INTR_USBSUSPEND */

   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
    /* Start of Frame interrupt */
#ifdef INTR_SOFINTR    
    if (gintr_status.b.sofintr)
    {
      /* Update the frame number variable */
      bIntPackSOF++;
      
      retval |= OTGD_FS_Handle_Sof_ISR();
    }
#endif /* INTR_SOFINTR */
    
   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
    /* Receive FIFO Queue Status Level interrupt */
#ifdef INTR_RXSTSQLVL
    if (gintr_status.b.rxstsqlvl)
    {
      retval |= OTGD_FS_Handle_RxStatusQueueLevel_ISR();
    }
#endif /* INTR_RXSTSQLVL */
    
   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
    /* Enumeration Done interrupt */
#ifdef INTR_ENUMDONE
    if (gintr_status.b.enumdone)
    {
      retval |= OTGD_FS_Handle_EnumDone_ISR();
    }
#endif /* INTR_ENUMDONE */
    
   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
    /* Reset interrupt */
#ifdef INTR_USBRESET
    if (gintr_status.b.usbreset)
    {
      retval |= OTGD_FS_Handle_UsbReset_ISR();
    }    
#endif /* INTR_USBRESET */
    
   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/
    /* IN Endpoint interrupt */
#ifdef INTR_INEPINTR
    if (gintr_status.b.inepint)
    {
      retval |= OTGD_FS_Handle_InEP_ISR();
    }
#endif /* INTR_INEPINTR */
    
   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/    
    /* OUT Endpoint interrupt */
#ifdef INTR_OUTEPINTR
    if (gintr_status.b.outepintr)
    {
      retval |= OTGD_FS_Handle_OutEP_ISR();
    }
#endif /* INTR_OUTEPINTR */    
 
   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/    
    /* Mode Mismatch interrupt */
#ifdef INTR_MODEMISMATCH
    if (gintr_status.b.modemismatch)
    {
      retval |= OTGD_FS_Handle_ModeMismatch_ISR();
    }
#endif /* INTR_MODEMISMATCH */  

   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/    
    /* Global IN Endpoints NAK Effective interrupt */
#ifdef INTR_GINNAKEFF
    if (gintr_status.b.ginnakeff)
    {
      retval |= OTGD_FS_Handle_GInNakEff_ISR();
    }
#endif /* INTR_GINNAKEFF */  

   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/    
    /* Global OUT Endpoints NAK effective interrupt */
#ifdef INTR_GOUTNAKEFF
    if (gintr_status.b.goutnakeff)
    {
      retval |= OTGD_FS_Handle_GOutNakEff_ISR();
    }
#endif /* INTR_GOUTNAKEFF */  

   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/    
    /* Isochronous Out packet Dropped interrupt */
#ifdef INTR_ISOOUTDROP
    if (gintr_status.b.isooutdrop)
    {
      retval |= OTGD_FS_Handle_IsoOutDrop_ISR();
    }
#endif /* INTR_ISOOUTDROP */  

   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/    
    /* Endpoint Mismatch error interrupt */
#ifdef INTR_EPMISMATCH
    if (gintr_status.b.epmismatch)
    {
      retval |= OTGD_FS_Handle_EPMismatch_ISR();
    }
#endif /* INTR_EPMISMATCH */  

   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/    
    /* Incomplete Isochronous IN transfer error interrupt */
#ifdef INTR_INCOMPLISOIN
    if (gintr_status.b.incomplisoin)
    {
      retval |= OTGD_FS_Handle_IncomplIsoIn_ISR();
    }
#endif /* INTR_INCOMPLISOIN */  

   /*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*/    
    /* Incomplete Isochronous OUT transfer error interrupt */
#ifdef INTR_INCOMPLISOOUT
    if (gintr_status.b.outepintr)
    {
      retval |= OTGD_FS_Handle_IncomplIsoOut_ISR();
    }
#endif /* INTR_INCOMPLISOOUT */  
  
  }
  return retval;
}

#endif /* STM32F10X_CL */

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
