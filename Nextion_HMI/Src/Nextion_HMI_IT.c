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
		static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		if(nextionHMI_h.rxCounter >= NEX_RX_BUFF_SIZE) {
			//Serial RX buffer overflow TODO :
			nextionHMI_h.rxCounter = nextionHMI_h.rxPosition = 0;
			nextionHMI_h.errorCnt++;
			HAL_UART_Receive_IT(nextionHMI_h.pUart, &nextionHMI_h.rxBuff[nextionHMI_h.rxCounter], 1);
		} else {
			HAL_UART_Receive_IT(nextionHMI_h.pUart, &nextionHMI_h.rxBuff[++nextionHMI_h.rxCounter], 1);
		}
		//Start/reset the timer at every received bytes
		xTimerResetFromISR(nextionHMI_h.rxTimerHandle, &xHigherPriorityTaskWoken);
		//If a TX timer is started, HAL_UART_TxCpltCallback has been called
		if(nextionHMI_h.hmiStatus == COMP_BUSY_TX){
			//timer is active
			xTimerResetFromISR(nextionHMI_h.blockTx, pdFALSE);
		}
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
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
		//nextionHMI_h.hmiStatus = COMP_BUSY_RX;
		static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		// Notify the sending task
		vTaskNotifyGiveFromISR(nextionHMI_h.xTaskToNotify, &xHigherPriorityTaskWoken);
		// The sending task is no longer waiting
		nextionHMI_h.xTaskToNotify = NULL;
		//PULSE();//dbg
		//Every received byte will reset the timer
		nextionHMI_h.hmiStatus = COMP_BUSY_TX;
		xTimerResetFromISR(nextionHMI_h.blockTx, pdFALSE);

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

/**
 * @brief TX Timer Callback
 * @note  Fires when TxTimer expires, data is processed or simply timeout occurred
 * 		  Continue executing the next command
 *
 * @param *argument
 * @retval void
 */
void txTimerCallback(void *argument) {
	//The TX timer has expired, send the next command
	xTimerStop(nextionHMI_h.blockTx,0);
	//PULSE();//dbg
	//Give back the semaphore for the next command
	nextionHMI_h.hmiStatus = COMP_IDLE;
	xSemaphoreGive(nextionHMI_h.hmiUartTxSem);
}
