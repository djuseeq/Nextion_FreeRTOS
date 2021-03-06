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
 * @note  Without calling this function, 2 is the default level.
 * @levels	0 - Off, no feedback
 * 			1 - OnSuccess, return data only if the last serial command was executed successfully
 * 			2 - OnFailure, return data only if the execution of the last serial command failed
 * 			3 - Always,
 *
 * @param vLevel = Verbose level
 * @retval void
 */
void NxHmi_Verbosity(uint8_t vLevel) {

	if(vLevel > 3) vLevel = 3;

	prepareToSend(0);
	sprintf(txBuf, "bkcmd=%i", vLevel);
	HmiSendCommand(txBuf);
	waitForAnswer(NULL);
	nextionHMI_h.ifaceVerbose = vLevel;
}

/**
 * @brief Set Nextion display backlight brightness
 * @note  Set/save lcd brightness
 *
 * @param value   = Brightness value in %, 0-100%
 * @param cnfSave = SET_TEMPORARY = after reset goes back to default brightness
 *                  SET_PERMANENT = save the value as default
 * @retval see @ref waitForAnswer() function for return value
 */
Ret_Status_t NxHmi_SetBacklight( uint8_t value, Cnf_permanence_t cnfSave) {

	prepareToSend(0);
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
 * @note  Receiving procedure must to be implemented in the user code
 *
 * @param status = 0 - stop sending, 1 - start sending
 * @retval see @ref waitForAnswer() function for return value
 */
Ret_Status_t NxHmi_SendXYcoordinates(uint8_t status) {

	prepareToSend(0);
	if (status) {
		status = 1;
	}

	sprintf(txBuf, "sendxy=%i", status);
	HmiSendCommand(txBuf);

	return waitForAnswer(NULL);
}

/**
 * @brief Send display to sleep mode
 * @note
 *
 * @param status = 0 - exit from sleep mode, 1 - enter to sleep mode
 * @retval see @ref waitForAnswer() function for return value
 */
Ret_Status_t NxHmi_Sleep(uint8_t status) {

	prepareToSend(0);
	if (status) {
		status = 1;
	}
	sprintf(txBuf, "sleep=%i", status);
	HmiSendCommand(txBuf);

	return waitForAnswer(NULL);
}

/**
 * @brief Configure display auto sleep/wake up events
 * @note
 *
 * @param slNoSer 	- No serial then sleep timer in seconds (3 - 65535), default: 0 - turned off
 * @param slNoTouch - No touch then sleep timer in seconds (3 - 65535), default: 0 - turned off
 * @param wkpSer	- Wake up if serial data arrives, 0 - off (don't wake up), 1 - on
 * @param wkpTouch	- Wake up if touch event occurs, 0 - off (don't wake up), 1 - on
 * @retval see @ref waitForAnswer() function for return value
 */
Ret_Status_t NxHmi_SetAutoSleep(uint16_t slNoSer, uint16_t slNoTouch, uint8_t wkpSer, uint8_t wkpTouch) {
	Ret_Status_t tmpRet;

	//limiting values
	if( (slNoSer > 0) && (slNoSer < 3) ) slNoSer = 3; //min. 3 second or 0 - disabled
	if( (slNoTouch > 0) && (slNoTouch < 3) ) slNoTouch = 3; //min. 3 second or 0 - disabled
	if( wkpSer ) wkpSer = 1;
	if( wkpTouch ) wkpTouch = 1;

	//Enable/disable wake up on serial event
	prepareToSend(0);
	sprintf(txBuf, "usup=%i", wkpSer);
	HmiSendCommand(txBuf);
	tmpRet = waitForAnswer(NULL);

	//Enable/disable wake up on touch event
	if(tmpRet == STAT_OK) {
		prepareToSend(0);
		sprintf(txBuf, "thup=%i", wkpTouch);
		HmiSendCommand(txBuf);
		tmpRet = waitForAnswer(NULL);
	} else {
		return tmpRet;
	}

	//Set no serial timer
	if(tmpRet == STAT_OK) {
		prepareToSend(0);
		sprintf(txBuf, "ussp=%u", slNoSer);
		HmiSendCommand(txBuf);
		tmpRet = waitForAnswer(NULL);
	} else {
		return tmpRet;
	}

	//Set no touch timer
	if(tmpRet == STAT_OK) {
		prepareToSend(0);
		sprintf(txBuf, "thsp=%u", slNoTouch);
		HmiSendCommand(txBuf);
		tmpRet = waitForAnswer(NULL);
	}
	return tmpRet;
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

