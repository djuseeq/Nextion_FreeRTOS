/*
 * Nextion_HMI.c
 *
 *  Created on: Jan 19, 2021
 *      Author: György Kovács
 */

#include "Nextion_HMI.h"


static Nextion_Object_t *Nextion_Object_List[NEX_MAX_OBJECTS];
static uint16_t Nextion_Object_Count = 0;


const osThreadAttr_t hmiObjectTask_attributes = {
  .name = "hmiObjectTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 256 * 4
};

const osThreadAttr_t hmiRxTask_attributes = {
  .name = "hmiRxTask",
  .priority = (osPriority_t) osPriorityNormal,
  .stack_size = 128 * 4
};

const osMessageQueueAttr_t rxComQ_attributes = {
  .name = "rxComQ"
};

const osMessageQueueAttr_t rxNumber_attributes = {
  .name = "rxNumQ"
};

const osMessageQueueAttr_t txObjQ_attributes = {
  .name = "txObjQ"
};

//char term[4] = {0xFF, 0xFF, 0xFF};


void ObjectHandlerTask(void *argument);
void StartHmiRxTask(void *argument);

static void HmiSendCommand(const char *cmd);

static int8_t HmiCmdFromStream(uint8_t *buff, uint8_t buffSize);

static void validateCommand(uint8_t *cmdBuff);

static void findObject(uint8_t pid, uint8_t cid, uint8_t event);


/* FreeRTOS Task ObjectHandler*/
void ObjectHandlerTask(void *argument) {

	Ret_Command_t objCommand;
	NxHmi_ResetDevice();

  for(;;) {
	  	  // Block until an command arrives
	  if(xQueueReceive(nextionHMI_h.objectQueueHandle, &objCommand, portMAX_DELAY) == pdPASS){
		  //Successfully received a command
		  //Lookup in object array for the received command and call the corresponding function
		  findObject(objCommand.pageId, objCommand.cmpntId, objCommand.event);
	  }
  }//end for loop
}


/* FreeRTOS Task HmiRx*/
void StartHmiRxTask(void *argument) {

	uint8_t commandBuffer[20];

	HAL_UART_Receive_IT(nextionHMI_h.pUart, &nextionHMI_h.rxBuff[nextionHMI_h.rxCounter++], 1);

  /* Infinite loop */
  for(;;) {

	  /* Block indefinitely until a Serial data arrives or query timeouts*/
	  	  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	  	  //loop thru RX buffer until no more data left or error occurs
	  	while(HmiCmdFromStream(commandBuffer, sizeof(commandBuffer) ) > 0 ){
	  		validateCommand(commandBuffer);
	  	}

  }//end for loop
}


/**
 * @brief Initialize FreeRTOS components and variables .
 * @note  Call this function BEFORE osKernelStart().
 *
 * @param , communication line with Nextion display
 * @retval void
 */
void NxHmi_Init(UART_HandleTypeDef *huart){
	nextionHMI_h.pUart = huart;
	nextionHMI_h.rxCounter = 0;
	nextionHMI_h.rxPosition = 0;
	nextionHMI_h.errorCnt = 0;
	nextionHMI_h.cmdCnt = 0;


	  /* creation of hmiTasks */
	  hmiObjectTaskHandle = osThreadNew(ObjectHandlerTask, NULL, &hmiObjectTask_attributes);
	  hmiRxTaskHandle = osThreadNew(StartHmiRxTask, NULL, &hmiRxTask_attributes);


	  /* creation of queues */

	  nextionHMI_h.rxCommandQHandle = osMessageQueueNew (4, sizeof(Ret_Command_t), &rxComQ_attributes);
	  nextionHMI_h.rxNumber = osMessageQueueNew(4, sizeof(uint32_t), &rxNumber_attributes);
	  nextionHMI_h.objectQueueHandle = osMessageQueueNew (4, sizeof(Ret_Command_t), &txObjQ_attributes);

	  /* creation of RX timer */
	  nextionHMI_h.rxTimerHandle = xTimerCreate("RxTimer",         // Just a text name, not used by the kernel.
		  	  	  	  	  	  	  	TOUT_PERIOD_CALC(nextionHMI_h.pUart->Init.BaudRate) , // The timer period in ticks.
                                    pdFALSE,         // The timers will auto-reload themselves when they expire.
									( void * )0,     // Assign each timer a unique id equal to its array index.
                                    (TimerCallbackFunction_t) rxTimerCallback     // Each timer calls the same callback when it expires.
                                    );

	  /* creation of semaphore */
	  nextionHMI_h.hmiSmphrHandle = xSemaphoreCreateBinary();
	  //nextionHMI_h.hmiSmphrHandle = xSemaphoreCreateMutex();
/*
	  if(nextionHMI_h.hmiSmphrHandle == NULL){
		  for(;;);
	  }
*/
	  xSemaphoreGive(nextionHMI_h.hmiSmphrHandle);

}

/**
 * @brief Add Nextion objects to object array .
 * @note
 *
 * @param *pOb_handle  Nextion object
 * @retval 0-failed, 1-success
 */
uint8_t NxHmi_AddObject(Nextion_Object_t *pOb_handle) {
	if (Nextion_Object_Count < NEX_MAX_OBJECTS) {
		Nextion_Object_List[Nextion_Object_Count] = pOb_handle;
		Nextion_Object_Count++;
		return 1;
	}
	return 0;
}

/**
 * @brief Search for the object by IDs
 * @note  Static function, intended for internal task
 *
 * @param pid = Page ID
 * @param cid = Component ID
 * @param event = 0-Release, 1-Touch ( https://nextion.tech/instruction-set/#s7 )
 * @retval void
 */
static void findObject(uint8_t pid, uint8_t cid, uint8_t event) {
	Nextion_Object_t *handle = NULL;

	uint16_t i = 0;

	for (i = 0; i < Nextion_Object_Count; i++) {
		handle = Nextion_Object_List[i];

		if (handle->Page_ID == pid && handle->Component_ID == cid) {
			if (NEX_EVENT_TOUCH == event) {
				if (handle->PressCallback != NULL) {
					handle->PressCallback();
				}
			}
			else if (NEX_EVENT_RELEASE == event) {
				if (handle->ReleaseCallback != NULL) {
					handle->ReleaseCallback();
				}
			}

			break; //object found break the loop
		}
	}
}

/**
 * @brief Search for return value or event from Nextion display
 * @note  Static function, intended for internal task
 *
 * @param *buff = Pointer for the (RX) incoming data buffer
 * @param buffSize = Amount of incoming data
 * @retval int8_t = 0-No more data, 1-More data in the stream, <0 -Error
 */
static int8_t HmiCmdFromStream(uint8_t *buff, uint8_t buffSize){

	memset(buff, 0xFF, buffSize);

	uint8_t termPatternCnt = 0;

	if(nextionHMI_h.rxBuff[nextionHMI_h.rxPosition] == 0xFF){
		//Skip the last command termination characters
		while(nextionHMI_h.rxBuff[nextionHMI_h.rxPosition++] == 0xFF);
		//Step back
		nextionHMI_h.rxPosition--;
	}

	uint8_t helper = nextionHMI_h.rxPosition;

	//loop till end of stream
	for(; nextionHMI_h.rxPosition < nextionHMI_h.rxCounter; nextionHMI_h.rxPosition++){
		//if found termination character, break the loop
		if(nextionHMI_h.rxBuff[nextionHMI_h.rxPosition] == 0xFF){
			break;
		} else {
			if((nextionHMI_h.rxPosition - helper) < buffSize){
				//copy serial stream to command buffer
				buff[nextionHMI_h.rxPosition - helper] = nextionHMI_h.rxBuff[nextionHMI_h.rxPosition];
			} else {
				//buffer overflow, TODO: drop everything?
				memset(buff, 0xFF, buffSize);
				nextionHMI_h.errorCnt++;
				return -3;
			}

		}
	}//end for loop

	//count the terminator characters (Nextion always send 3 pcs)
	for(; nextionHMI_h.rxBuff[termPatternCnt + nextionHMI_h.rxPosition] == 0xFF; termPatternCnt++){
		//if we have reached the end of the stream
		if( (termPatternCnt + nextionHMI_h.rxPosition) >= nextionHMI_h.rxCounter){
			break;
		}
	}//end for loop

	if(nextionHMI_h.rxCounter == (nextionHMI_h.rxPosition) ){
		//Reach the end
		nextionHMI_h.rxPosition = nextionHMI_h.rxCounter = 0;
		//to store data at beginning of the buffer
		HAL_UART_AbortReceive_IT(nextionHMI_h.pUart);
		memset(nextionHMI_h.rxBuff,0,NEX_RX_BUFF_SIZE);
		HAL_UART_Receive_IT(nextionHMI_h.pUart, &nextionHMI_h.rxBuff[nextionHMI_h.rxCounter++], 1);

		if(termPatternCnt == 3){
			nextionHMI_h.cmdCnt++;
			return 0; //no more data
		} else {
			nextionHMI_h.errorCnt++;
			return -2; // error
		}

	} else {
		//More data in the buffer

		if(termPatternCnt == 3){
			nextionHMI_h.cmdCnt++;
			return 1; //there is more data in the RX buffer
		} else {
			nextionHMI_h.errorCnt++;
			return -1; // more data and error
		}
	}

}

/**
 * @brief Parse incoming event from Nextion display
 * @note  Static function, intended for internal task
 *
 * @param *cmdBuff = Pointer for the (RX) incoming data buffer
 * @retval void
 */
static void validateCommand(uint8_t *cmdBuff){
	Ret_Command_t command;
	memset(&command, 0, sizeof(Ret_Command_t));
	uint8_t sendQueue = 1;

	if(cmdBuff[0] == 0x00 && cmdBuff[1] == 0xFF ){
		//Invalid instruction
		command.cmdCode = 0x00;
		nextionHMI_h.errorCnt++;
	} else if(cmdBuff[0] > 0x01 && cmdBuff[0] <= 0x23){
		//Other errors
		command.cmdCode = 0x00;
		nextionHMI_h.errorCnt++;

	} else {
		//Normal return answer
		switch (cmdBuff[0]) {
			case NEX_EVENT_SUCCESS:
				command.cmdCode = cmdBuff[0];
				break;

			case NEX_EVENT_INIT_OK: // after reset
				command.cmdCode = cmdBuff[0];
				sendQueue = 0;
				xQueueSend(nextionHMI_h.rxNumber, &command.cmdCode, NEX_QUEUE_TIMEOUT);
				break;

			case NEX_EVENT_UPGRADE:
				command.cmdCode = cmdBuff[0];
				break;

			case NEX_EVENT_TOUCH_HEAD:
				command.cmdCode = cmdBuff[0];
				command.pageId = cmdBuff[1];
				command.cmpntId = cmdBuff[2];
				command.event = cmdBuff[3];
				xQueueSend(nextionHMI_h.objectQueueHandle, &command, NEX_QUEUE_TIMEOUT);
				sendQueue = 0;
				break;

			case NEX_EVENT_POSITION_HEAD:
				command.cmdCode = cmdBuff[0];
				command.xCoordinate = (cmdBuff[1] << 0) | (cmdBuff[2] << 8);
				command.yCoordinate = (cmdBuff[3] << 0) | (cmdBuff[4] << 8);
				command.event = cmdBuff[5];
				break;

			case NEX_RET_CURRENT_PAGEID_HEAD:
				command.cmdCode = cmdBuff[0];
				command.pageId = cmdBuff[1];
				break;

			case NEX_RET_STRING_HEAD:
				command.cmdCode = cmdBuff[0];
				break;

			case NEX_RET_NUMBER_HEAD:
				command.cmdCode = cmdBuff[0];
				command.numData = (cmdBuff[1] << 0) | (cmdBuff[2] << 8) |
						(cmdBuff[3] << 16) | (cmdBuff[4] << 24);
				sendQueue = 0;
				xQueueSend(nextionHMI_h.rxNumber, &command.numData, NEX_QUEUE_TIMEOUT);
				break;

			default:
				command.cmdCode = 0x00;
				break;
		}//end switch
	}//end if

	if(sendQueue){		//The command I don't use for now
						//TODO: just drop it here
		xQueueSend(nextionHMI_h.rxCommandQHandle, &command, NEX_QUEUE_TIMEOUT);
		if(uxQueueSpacesAvailable(nextionHMI_h.rxCommandQHandle) == 0){
			//If no task will "consume" the messages, then delete, otherwise will block
		//    the hmiRxTask permanently
			xQueueReset(nextionHMI_h.rxCommandQHandle);
		}//end if no free space in queue
	}//end if send queue

	if(uxQueueSpacesAvailable(nextionHMI_h.rxNumber) == 0){
		xQueueReset(nextionHMI_h.rxNumber);
	}//end if no free space in queue

	if(uxQueueSpacesAvailable(nextionHMI_h.objectQueueHandle) == 0){
		xQueueReset(nextionHMI_h.objectQueueHandle);
	}//end if no free space in queue

}

/**
 * @brief Set text for txt type of Nextion object
 * @note  Send text
 *
 * @param *pOb_handle = Nextion object handler
 * @param *buffer = string pointer
 * @retval void
 */
void NxHmi_SetText(Nextion_Object_t *pOb_handle, const char *buffer) {

	xSemaphoreTake(nextionHMI_h.hmiSmphrHandle, portMAX_DELAY);


	sprintf(txBuf, "%s.txt=\"%s\"", pOb_handle->Name, buffer);
	//sprintf(txBuf, "%s.txt=\"%s\"%s", pOb_handle->Name, buffer, term);
	//HAL_UART_Transmit_IT(nextionHMI_h.pUart,(uint8_t*)txBuf, strlen(txBuf) );
	HmiSendCommand(txBuf);

}

/**
 * @brief Set integer number for int type of Nextion object
 * @note  Send number
 *
 * @param *pOb_handle = Nextion object handler
 * @param number = integer
 * @retval void
 */
void NxHmi_SetIntValue(Nextion_Object_t *pOb_handle, int16_t number) {

	xSemaphoreTake(nextionHMI_h.hmiSmphrHandle, portMAX_DELAY);
	sprintf(txBuf, "%s.val=%i", pOb_handle->Name, number);
	//sprintf(txBuf, "%s.val=%i%s", pOb_handle->Name, number, term);
	//HAL_UART_Transmit_IT(nextionHMI_h.pUart,(uint8_t*)txBuf, strlen(txBuf) );
	HmiSendCommand( txBuf);
}

/**
 * @brief Set float number for int type of Nextion object
 * @note  Send float number
 *
 * @param *pOb_handle = Nextion object handler
 * @param number = float number
 * @retval void
 */
void NxHmi_SetFloatValue( Nextion_Object_t *pOb_handle, float number) {
	xSemaphoreTake(nextionHMI_h.hmiSmphrHandle, portMAX_DELAY);
	sprintf(txBuf, "%s.txt=\"%.2f\"", pOb_handle->Name, number);
	//sprintf(txBuf, "%s.txt=\"%.2f\"%s", pOb_handle->Name, number, term);
	//HAL_UART_Transmit_IT(nextionHMI_h.pUart,(uint8_t*)txBuf, strlen(txBuf) );
	HmiSendCommand( txBuf);
}

/**
 * @brief Set Nextion object background color
 * @note  16bit color code ( https://nextion.tech/instruction-set/#s5 )
 *
 * @param *pOb_handle = Nextion object handler
 * @param colour = 16bit RGB color code, R-5bit G-6bit, B-5bit
 * @retval void
 */
void NxHmi_SetBcoColour(Nextion_Object_t *pOb_handle, int16_t color) {

	xSemaphoreTake(nextionHMI_h.hmiSmphrHandle, portMAX_DELAY);
	sprintf(txBuf, "%s.bco=%i", pOb_handle->Name, color);
	//sprintf(txBuf, "%s.bco=%i%s", pOb_handle->Name, color, term);
	//HAL_UART_Transmit_IT(nextionHMI_h.pUart,(uint8_t*)txBuf, strlen(txBuf) );
	HmiSendCommand(txBuf);
/*
	// Refresh component, not mandatory, "auto-refresh when attribute changes since v0.38"
	xSemaphoreTake(nextionHMI_h.hmiSmphrHandle, portMAX_DELAY);
	sprintf(txBuf, "ref %s", pOb_handle->Name);
	//sprintf(txBuf, "ref %s%s", pOb_handle->Name, term);
	//HAL_UART_Transmit_IT(nextionHMI_h.pUart,(uint8_t*)txBuf, strlen(txBuf) );
	HmiSendCommand( txBuf);
*/
}

/**
 * @brief Set Nextion object visibility
 * @note  Show/hide object
 *
 * @param *pOb_handle = Nextion object handler
 * @param visible = OBJ_HIDE, OBJ_SHOW
 * @retval void
 */
void NxHmi_SetObjectVisibility(Nextion_Object_t *pOb_handle, Ob_visibility_t visible) {
	xSemaphoreTake(nextionHMI_h.hmiSmphrHandle, portMAX_DELAY);
	sprintf(txBuf, "vis %s,%d", pOb_handle->Name, visible);
	//sprintf(txBuf, "vis %s,%d%s", pOb_handle->Name, visible,term);
	//HAL_UART_Transmit_IT(nextionHMI_h.pUart,(uint8_t*)txBuf, strlen(txBuf) );

	HmiSendCommand( txBuf);
}

/**
 * @brief Set Nextion display backlight brightness
 * @note  Set/save lcd brightness
 *
 * @param value = Brightness value in %, 0-100%
 * @param cnfSave = SET_TEMPORARY = after reset goes back to default brightness
 *                  SET_PERMANENT = save the value as default
 * @retval void
 */
void NxHmi_SetBacklight( uint8_t value, Cnf_permanence_t cnfSave) {
	xSemaphoreTake(nextionHMI_h.hmiSmphrHandle, portMAX_DELAY);
	if (cnfSave) {
		sprintf(txBuf, "dims=%i", value);
		//sprintf(txBuf, "dims=%i%s", value,term);
	} else {
		sprintf(txBuf, "dim=%i", value);
		//sprintf(txBuf, "dim=%i%s", value, term);
	}
	//HAL_UART_Transmit_IT(nextionHMI_h.pUart,(uint8_t*)txBuf, strlen(txBuf) );
	HmiSendCommand( txBuf);
}

/**
 * @brief Change to the specified page
 * @note  Default page is 0
 *
 * @param pageId = Page number, 0-default
 * @retval void
 */
void NxHmi_GotoPage(uint8_t pageId){
	xSemaphoreTake(nextionHMI_h.hmiSmphrHandle, portMAX_DELAY);
	sprintf(txBuf, "page %i", pageId);
	//sprintf(txBuf, "page %i%s", pageId, term);
	//HAL_UART_Transmit_IT(nextionHMI_h.pUart,(uint8_t*)txBuf, strlen(txBuf) );
	HmiSendCommand( txBuf);
}

/**
 * @brief Get a Nextion objects value
 * @note  TODO: implement string ret value
 *		  TODO: return value if timeout occurs
 * @param *pOb_handle = Nextion object handler
 * @retval int32
 */
int32_t NxHmi_GetObjValue(Nextion_Object_t *pOb_handle){
	xSemaphoreTake(nextionHMI_h.hmiSmphrHandle, portMAX_DELAY);
	uint32_t retNumber;
	switch (pOb_handle->dataType) {
		case OBJ_TYPE_INT:
			sprintf(txBuf, "get %s.val", pOb_handle->Name);
			//sprintf(txBuf, "get %s.val%s", pOb_handle->Name, term);
			break;

		case OBJ_TYPE_TXT: //TODO: not implemented yet
			sprintf(txBuf, "ref %s.txt", pOb_handle->Name);
			//sprintf(txBuf, "ref %s.txt%s", pOb_handle->Name, term);
			break;
		default:
			break;
	}

	HmiSendCommand( txBuf);
	//HAL_UART_Transmit_IT(nextionHMI_h.pUart,(uint8_t*)txBuf, strlen(txBuf) );

	if(xQueueReceive(nextionHMI_h.rxNumber, &retNumber, NEX_ANSW_TIMEOUT) == pdFALSE){
		//Timeout TODO: -1  probably is not the best return value for error
		return -1;
	}

	return retNumber;
}

/**
 * @brief Perform a soft reset
 * @note  Restart the display, when it is ready, returns 00 00 00 FF FF FF, 88 FF FF FF
 * 			takes approximately 250ms
 *
 * @param void
 * @retval int32
 */
int32_t NxHmi_ResetDevice(void){
	nextionHMI_h.hmiStatus = COMP_BUSY;
	xSemaphoreTake(nextionHMI_h.hmiSmphrHandle, portMAX_DELAY);
	HmiSendCommand("");
	//sprintf(txBuf, "%s", term);
	//HAL_UART_Transmit_IT(nextionHMI_h.pUart,(uint8_t*)txBuf, strlen(txBuf) );
	xSemaphoreTake(nextionHMI_h.hmiSmphrHandle, portMAX_DELAY);
	uint32_t retNumber;
	sprintf(txBuf, "rest");
	//sprintf(txBuf, "rest%s", term);
	//HAL_UART_Transmit_IT(nextionHMI_h.pUart,(uint8_t*)txBuf, strlen(txBuf) );
	HmiSendCommand(txBuf);

	if(xQueueReceive(nextionHMI_h.rxNumber, &retNumber, NEX_ANSW_TIMEOUT) == pdFALSE){
		return -1;
	}
	if(retNumber == NEX_EVENT_INIT_OK){
		nextionHMI_h.hmiStatus = COMP_IDLE;
	}
	return retNumber;
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
	xSemaphoreTake(nextionHMI_h.hmiSmphrHandle, portMAX_DELAY);
	if (cnfSave) {
		sprintf(txBuf, "bauds=%i", (int)baud);
	} else {
		sprintf(txBuf, "baud=%i", (int)baud);
	}

	NxHmi_Send_Command( txBuf);

	HAL_UART_AbortReceive_IT(nextionHMI_h.pUart);


    //osDelay(100);
    while (HAL_UART_GetState(nextionHMI_h.pUart) == HAL_UART_STATE_BUSY){
    	osDelay(1);
    }

    taskENTER_CRITICAL();
	//nextionHMI_h.pUart->Instance->CR1 &= ~(USART_CR1_UE);     //Disable USART
	//nextionHMI_h.pUart->Instance->BRR = UART_BRR_SAMPLING8(HAL_RCC_GetPCLK2Freq(), baud);
	//nextionHMI_h.pUart->Instance->CR1 |= (USART_CR1_UE);     //Enable USART
    HAL_UART_DeInit(nextionHMI_h.pUart);
    nextionHMI_h.pUart->Init.BaudRate = baud;
    if (HAL_UART_Init(nextionHMI_h.pUart) != HAL_OK)
    {
      Error_Handler();
    }


    while (HAL_UART_GetState(nextionHMI_h.pUart) != HAL_UART_STATE_READY){

    }
    nextionHMI_h.rxCounter = 0;
    HAL_UART_Receive_IT(nextionHMI_h.pUart, &nextionHMI_h.rxBuff[0], 1);
    taskEXIT_CRITICAL();
    if( xTimerIsTimerActive( nextionHMI_h.rxTimerHandle ) != pdFALSE ){
    	xTimerDelete(nextionHMI_h.rxTimerHandle,0);
    }
	xTimerChangePeriod(nextionHMI_h.rxTimerHandle, TOUT_PERIOD_CALC(baud), 10);



}*/

/**
 * @brief Attach the terminating characters at the end of the command string
 * @note  Static function, intended for internal task
 * 		  I observed that under RTOS it consume more stack space from the task
 * 		  if i use these methods (function calling from function)
 *
 * @param *cmd = command string value
 * @retval void
 */


static void HmiSendCommand(const char *cmd) {

    char term[4] = {0xFF, 0xFF, 0xFF};

	sprintf(txBuf, "%s%s", cmd, term);

    while (HAL_UART_GetState(nextionHMI_h.pUart) == HAL_UART_STATE_BUSY){
    	osDelay(1);
    }

    HAL_UART_Transmit_IT(nextionHMI_h.pUart,(uint8_t*)txBuf, strlen(txBuf) );

}





