/*
 * Nextion_HMI_SysCom.c
 *
 *  Created on: Feb 15, 2021
 *      Author: György Kovács
 *
 *      System variable command
 */

#include "Nextion_HMI.h"




/**
 * @brief Set the display verbose level
 * @levels	0 - Off, no feedback
 * 			1 - OnSuccess, return data only if the last serial command was executed successfully
 * 			2 - OnFailure, return data only if the execution of the last serial command failed
 * 			3 - Always,
 *
 * @param vLevel = Verbose level
 * @retval void
 */
void NxHmi_Verbosity(uint8_t vLevel) {

	prepareToSend();
	sprintf(txBuf, "bkcmd=%i", vLevel);
	HmiSendCommand(txBuf);
	waitForAnswer(NULL);
}

/**
 * @brief Set Nextion display backlight brightness
 * @note  Set/save lcd brightness
 *
 * @param value = Brightness value in %, 0-100%
 * @param cnfSave = SET_TEMPORARY = after reset goes back to default brightness
 *                  SET_PERMANENT = save the value as default
 * @retval see @ref waitForAnswer() function for return value
 */
FNC_Ret_Status_t NxHmi_SetBacklight( uint8_t value, Cnf_permanence_t cnfSave) {

	prepareToSend();
	if(value > 100) value = 100;

	if (cnfSave == SET_PERMANENT) {
		sprintf(txBuf, "dims=%i", value);
	} else {
		sprintf(txBuf, "dim=%i", value);
	}
	HmiSendCommand(txBuf);

	return waitForAnswer(NULL);
}

/**
 * @brief Start sending real time touch coordinates
 * @note  Receiving procedure must be performed in the user code
 *
 * @param status = 0 - stop sending, 1 - start sending
 * @retval see @ref waitForAnswer() function for return value
 */
FNC_Ret_Status_t NxHmi_SendXYcoordinates(uint8_t status) {
	prepareToSend();
	if (status) {
		status = 1;
	}

	sprintf(txBuf, "sendxy=%i", status);
	HmiSendCommand(txBuf);

	return waitForAnswer(NULL);
}

/**
 * @brief Start sending real time touch coordinates
 * @note  Receiving procedure must be performed in the user code
 *
 * @param status = 0 - stop sending, 1 - start sending
 * @retval see @ref waitForAnswer() function for return value
 */
FNC_Ret_Status_t NxHmi_Sleep(uint8_t status) {
	prepareToSend();
	if (status) {
		status = 1;
	}
	sprintf(txBuf, "sleep=%i", status);
	HmiSendCommand(txBuf);

	return waitForAnswer(NULL);
}

/**
 * @brief Configure the display communications speed
 * @note  TODO: proper way to change the UART speed on the fly under RTOS
 *
 * @param baud = The new desired speed value
 * @param cnfSave = SET_TEMPORARY = after reset goes back to default speed
 *                  SET_PERMANENT = save the value as default
 * @retval void
 */

/*
void NxHmi_ComSpeed(uint32_t baud, Cnf_permanence_t cnfSave) {

	prepareToSend();
	if (cnfSave) {
		sprintf(txBuf, "bauds=%i", (int)baud);
	} else {
		sprintf(txBuf, "baud=%i", (int)baud);
	}

	NxHmi_Send_Command( txBuf);

	HAL_UART_AbortReceive_IT(nextionHMI_h.pUart);


    //osDelay(100);
    while (HAL_UART_GetState(nextionHMI_h.pUart) == HAL_UART_STATE_BUSY) {
    	osDelay(1);
    }

    taskENTER_CRITICAL();
	//nextionHMI_h.pUart->Instance->CR1 &= ~(USART_CR1_UE);     //Disable USART
	//nextionHMI_h.pUart->Instance->BRR = UART_BRR_SAMPLING8(HAL_RCC_GetPCLK2Freq(), baud);
	//nextionHMI_h.pUart->Instance->CR1 |= (USART_CR1_UE);     //Enable USART
    HAL_UART_DeInit(nextionHMI_h.pUart);
    nextionHMI_h.pUart->Init.BaudRate = baud;
    if (HAL_UART_Init(nextionHMI_h.pUart) != HAL_OK) {
      Error_Handler();
    }


    while (HAL_UART_GetState(nextionHMI_h.pUart) != HAL_UART_STATE_READY) {

    }
    nextionHMI_h.rxCounter = 0;
    HAL_UART_Receive_IT(nextionHMI_h.pUart, &nextionHMI_h.rxBuff[0], 1);
    taskEXIT_CRITICAL();
    if( xTimerIsTimerActive( nextionHMI_h.rxTimerHandle ) != pdFALSE ) {
    	xTimerDelete(nextionHMI_h.rxTimerHandle,0);
    }
	xTimerChangePeriod(nextionHMI_h.rxTimerHandle, TOUT_PERIOD_CALC(baud), 10);


}*/

