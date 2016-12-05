/******************** (C) COPYRIGHT 2011 STMicroelectronics ********************
 * File Name          : hw_config.c
 * Author             : MCD Application Team
 * Version            : V3.3.0
 * Date               : 21-March-2011
 * Description        : Hardware Configuration & Setup
 ********************************************************************************
 * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
 * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
 * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
 * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
 * CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
 * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *******************************************************************************/

#include "stm32f10x_it.h"

#include "usb_lib.h"
#include "usb_prop.h"
#include "usb_desc.h"
#include "hw_config.h"
#include "platform_config.h"
#include "usb_pwr.h"
#include "stm32f10x_rcc.h"
#include "misc.h"

ErrorStatus HSEStartUpStatus;

static void IntToUnicode(uint32_t value, uint8_t *pbuf, uint8_t len);

/*******************************************************************************
 * Description    : Configures Main system clocks & power
 *******************************************************************************/
void Set_System(void)
{
}

/*******************************************************************************
 * Description    : Configures USB Clock input (48MHz)
 *******************************************************************************/
void Set_USBClock(void)
{
	/* Select USBCLK source */
	RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);

	/* Enable the USB clock */
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
}

/*******************************************************************************
 * Description    : Power-off system clocks and power while entering suspend mode
 *******************************************************************************/
void Enter_LowPowerMode(void)
{
	/* Set the device state to suspend */
	bDeviceState = SUSPENDED;
}

/*******************************************************************************
 * Description    : Restores system clocks and power while exiting suspend mode
 *******************************************************************************/
void Leave_LowPowerMode(void)
{
	DEVICE_INFO *pInfo = &Device_Info;

	/* Set the device state to the correct state */
	if (pInfo->Current_Configuration != 0)
	{
		/* Device configured */
		bDeviceState = CONFIGURED;
	}
	else
	{
		bDeviceState = ATTACHED;
	}
}

/*******************************************************************************
 * Description    : Configures the USB interrupts
 *******************************************************************************/
void USB_Interrupts_Config(void)
{
	NVIC_InitTypeDef NVIC_InitStructure;

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

	NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

/*******************************************************************************
 * Description    : Software Connection/Disconnection of USB Cable
 *******************************************************************************/
void USB_Cable_Config(FunctionalState NewState)
{
}

/*******************************************************************************
 * Description    : Create the serial number string descriptor.
 *******************************************************************************/
void Get_SerialNum(void)
{
	uint32_t Device_Serial0, Device_Serial1, Device_Serial2;

	Device_Serial0 = *(__IO uint32_t*) (0x1FFFF7E8);
	Device_Serial1 = *(__IO uint32_t*) (0x1FFFF7EC);
	Device_Serial2 = *(__IO uint32_t*) (0x1FFFF7F0);

	Device_Serial0 += Device_Serial2;

	if (Device_Serial0 != 0)
	{
		IntToUnicode(Device_Serial0, &Virtual_Com_Port_StringSerial[2], 8);
		IntToUnicode(Device_Serial1, &Virtual_Com_Port_StringSerial[18], 4);
	}
}

/*******************************************************************************
 * Description    : Convert Hex 32Bits value into char.
 *******************************************************************************/
static void IntToUnicode(uint32_t value, uint8_t *pbuf, uint8_t len)
{
	uint8_t idx = 0;

	for (idx = 0; idx < len; idx++)
	{
		if (((value >> 28)) < 0xA)
		{
			pbuf[2 * idx] = (value >> 28) + '0';
		}
		else
		{
			pbuf[2 * idx] = (value >> 28) + 'A' - 10;
		}

		value = value << 4;

		pbuf[2 * idx + 1] = 0;
	}
}
