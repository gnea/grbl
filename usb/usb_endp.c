/******************** (C) COPYRIGHT 2011 STMicroelectronics ********************
 * File Name          : usb_endp.c
 * Author             : MCD Application Team
 * Version            : V3.3.0
 * Date               : 21-March-2011
 * Description        : Endpoint routines
 ********************************************************************************
 * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
 * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
 * AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
 * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
 * CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
 * INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *******************************************************************************/

#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_mem.h"
#include "hw_config.h"
#include "usb_istr.h"
#include "usb_pwr.h"
#include "serial.h"
uint8_t USB_Rx_Buffer[VIRTUAL_COM_PORT_DATA_SIZE];

extern uint8_t serial_tx_buffer[];
extern uint8_t serial_tx_buffer_head;
extern volatile uint8_t serial_tx_buffer_tail;

void EP3_OUT_Callback(void)
{
	uint16_t USB_Rx_Cnt;

	/* Get the received data buffer and update the counter */
	USB_Rx_Cnt = USB_SIL_Read(EP3_OUT, USB_Rx_Buffer);

	/* USB data will be immediately processed, this allow next USB traffic being
	NAKed till the end of the USART Xfer */

	OnUsbDataRx(USB_Rx_Buffer, USB_Rx_Cnt);

	/* Enable the receive of data on EP3 */
	SetEPRxValid(ENDP3);
}
void EP1_IN_Callback (void)
{
	if (serial_tx_buffer_head != serial_tx_buffer_tail && (_GetEPTxStatus(ENDP1) == EP_TX_NAK))
    {
	    uint16_t USB_Tx_length;

        if (serial_tx_buffer_head > serial_tx_buffer_tail)
		    USB_Tx_length = serial_tx_buffer_head - serial_tx_buffer_tail;
        else
        {
		    USB_Tx_length = TX_BUFFER_SIZE - serial_tx_buffer_tail + serial_tx_buffer_head;
        }

        if (USB_Tx_length != 0)
        {
            if (USB_Tx_length > 64)
    	        USB_Tx_length = 64;

    //        UserToPMABufferCopy(&serial_tx_buffer[serial_tx_buffer_tail], ENDP1_TXADDR, USB_Tx_length);
            {
                uint8_t *pbUsrBuf = serial_tx_buffer + serial_tx_buffer_tail;
                uint32_t n = (USB_Tx_length + 1) >> 1;   /* n = (wNBytes + 1) / 2 */
                uint32_t i, temp1;
                uint16_t *pdwVal;
                pdwVal = (uint16_t *)(ENDP1_TXADDR * 2 + PMAAddr);
                for (i = n; i != 0; i--)
                {
                    temp1 = (uint16_t) * pbUsrBuf;
                    pbUsrBuf++;
                    if (pbUsrBuf - serial_tx_buffer == TX_BUFFER_SIZE)
                        pbUsrBuf = serial_tx_buffer;

                    *pdwVal++ = temp1 | (uint16_t) * pbUsrBuf << 8;
                    pdwVal++;
                    pbUsrBuf++;
                    if (pbUsrBuf - serial_tx_buffer == TX_BUFFER_SIZE)
                        pbUsrBuf = serial_tx_buffer;
                }
            }

	        SetEPTxCount(ENDP1, USB_Tx_length);
	        SetEPTxValid(ENDP1);

            serial_tx_buffer_tail += USB_Tx_length;
            if (serial_tx_buffer_tail >= TX_BUFFER_SIZE)
                serial_tx_buffer_tail -= TX_BUFFER_SIZE;
        }
	}
}


/*	\brief Start Of Frame (SOF) callback
 */
void SOF_Callback(void)
{
	if(bDeviceState == CONFIGURED)
	{
		/* Check the data to be sent through IN pipe */
		EP1_IN_Callback();
	}
}
/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/

