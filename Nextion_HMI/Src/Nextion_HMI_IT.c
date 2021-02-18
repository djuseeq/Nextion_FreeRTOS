/*
 * Nextion_HMI_IT.c
 *
 *  Created on: Jan 21, 2021
 *      Author: György Kovács
 *
 *      Interrupt callback functions
 */

#include "Nextion_HMI.h"


/**
 * @brief UART RX Complete Callback
 * @note  If you use other UART in interrupt mode, please implement it here
 *
 * @param *huart = UART Handler
 * @retval void
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {

	if(huart == nextionHMI_h.pUart) {
		if(nextionHMI_h.rxCounter >= NEX_RX_BUFF_SIZE) {
			//Serial RX buffer overflow TODO :
			nextionHMI_h.rxCounter = nextionHMI_h.rxPosition = 0;
			nextionHMI_h.errorCnt++;
			HAL_UART_Receive_IT(nextionHMI_h.pUart, &nextionHMI_h.rxBuff[nextionHMI_h.rxCounter], 1);
		} else {
			HAL_UART_Receive_IT(nextionHMI_h.pUart, &nextionHMI_h.rxBuff[++nextionHMI_h.rxCounter], 1);
		}
		//PULSE();
		//Start/reset the timer at every received bytes
		xTimerResetFromISR(nextionHMI_h.rxTimerHandle, pdFALSE);
	}//end if NEX port

}

/**
 * @brief UART TX Complete Callback
 * @note  If you use other UART in interrupt mode, please implement it here
 *
 * @param *huart = UART Handler
 * @retval void
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	if(huart == nextionHMI_h.pUart) {
		static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		// Give back the semaphore when transmit is complete
		xSemaphoreGiveFromISR(nextionHMI_h.hmiUartTxSem, &xHigherPriorityTaskWoken);
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
	}//end if NEX port
}

/**
 * @brief RX Timer Callback
 * @note  Fires when RxTimer expires, RX line is IDLE
 *
 * @param *argument
 * @retval void
 */
void rxTimerCallback(void *argument) {
	//The RX timer has expired, no more incoming bytes on the RX line
	xTimerStop(nextionHMI_h.rxTimerHandle,0);
	//PULSE();
	//Wake up the RxTask to process the received stream
	xTaskNotify(hmiRxTaskHandle, 0, eSetValueWithOverwrite);
}
