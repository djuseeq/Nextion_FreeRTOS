/*
 * Nextion_HMI.c
 *
 *  Created on: Jan 19, 2021
 *      Author: György Kovács
 */

#include "Nextion_HMI.h"

///Container for the Nextion objects (components)
static Nextion_Object_t *Nextion_Object_List[NEX_MAX_OBJECTS];
static uint16_t Nextion_Object_Count = 0;

//CMSIS_RTOS components
const osThreadAttr_t hmiObjectTask_attributes = {
  .name = "hmiObjectTask",
  .priority = (osPriority_t) NEX_HMIOBJECTTASK_PRIORITY,
  .stack_size = NEX_HMIOBJECTTASK_STACK
};

const osThreadAttr_t hmiRxTask_attributes = {
  .name = "hmiRxTask",
  .priority = (osPriority_t) NEX_HMIRXTASK_PRIORITY,
  .stack_size = NEX_HMIRXTASK_STACK
};

const osMessageQueueAttr_t rxComQ_attributes = {
  .name = "rxComQ"
};

const osMessageQueueAttr_t txObjQ_attributes = {
  .name = "txObjQ"
};

//PRIVATE FUNCTION PROTOTYPES//
void ObjectHandlerTask(void *argument);
void StartHmiRxTask(void *argument);

static int8_t HmiCmdFromStream(uint8_t *buff, uint8_t buffSize);
static void validateCommand(uint8_t *cmdBuff);
static void findObject(uint8_t pid, uint8_t cid, uint8_t event);
static int8_t isItRawData(void);

//|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

/* FreeRTOS Task ObjectHandler*/
void ObjectHandlerTask(void *argument) {

	Ret_Command_t objCommand;
	//Always perform a display reset
	NxHmi_ResetDevice();
	//NxHmi_Verbosity(3);
	nextionHMI_h.errorCnt = 0;
  for(;;) {
	  	  // Block until an command arrives
	  if(xQueueReceive(nextionHMI_h.objectQueueHandle, &objCommand, portMAX_DELAY) == pdPASS) {
		  //Successfully received a command
		  //Lookup in object array for the received command and call the corresponding function
		  findObject(objCommand.pageId, objCommand.cmpntId, objCommand.event);
	  }//end if
  }//end for loop
}


/* FreeRTOS Task HmiRx*/
void StartHmiRxTask(void *argument) {

	uint8_t commandBuffer[20];
	int8_t retAnswer = 0;

	HAL_UART_Receive_IT(nextionHMI_h.pUart, &nextionHMI_h.rxBuff[nextionHMI_h.rxCounter], 1);

  /* Infinite loop */
  for(;;) {

	  /* Block indefinitely until a Serial data arrives or query timeouts*/
	  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
	  	  //loop thru RX buffer until no more data left or error occurs
	  do{
		  retAnswer = HmiCmdFromStream(commandBuffer, sizeof(commandBuffer) );
		  validateCommand(commandBuffer);
		  memset(&commandBuffer, BUFF_CLEAR_PATTERN, sizeof(commandBuffer));
	  }while(retAnswer > 0);//end while loop
  }//end for loop
}
//|||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

/**
 * @brief Initialize FreeRTOS components and variables .
 * @note  Call this function BEFORE osKernelStart().
 *
 * @param , communication line with Nextion display
 * @retval void
 */
void NxHmi_Init(UART_HandleTypeDef *huart) {
	nextionHMI_h.pUart = huart;
	nextionHMI_h.rxCounter = 0;
	nextionHMI_h.rxPosition = 0;
	nextionHMI_h.errorCnt = 0;
	nextionHMI_h.cmdCnt = 0;
	nextionHMI_h.ifaceVerbose = 2; // default is level 2, return data On Failure
	nextionHMI_h.hmiStatus = COMP_IDLE;
	nextionHMI_h.xTaskToNotify = NULL;  // no task is waiting

	  /* creation of hmiTasks */
	  hmiObjectTaskHandle = osThreadNew(ObjectHandlerTask, NULL, &hmiObjectTask_attributes);
	  hmiRxTaskHandle = osThreadNew(StartHmiRxTask, NULL, &hmiRxTask_attributes);

	  /* creation of queues */
	  nextionHMI_h.rxCommandQHandle = osMessageQueueNew (4, sizeof(Ret_Command_t), &rxComQ_attributes);
	  nextionHMI_h.objectQueueHandle = osMessageQueueNew (4, sizeof(Ret_Command_t), &txObjQ_attributes);

	  /* creation of timers */
	  nextionHMI_h.rxTimerHandle = xTimerCreate("RxTimer",         // Just a text name, not used by the kernel.
		  	  	  	  	  	  	  	TOUT_PERIOD_CALC(nextionHMI_h.pUart->Init.BaudRate) , // The timer period in ticks.
                                    pdFALSE,         // The timers will auto-reload themselves when they expire.
									( void * )0,     // Assign each timer a unique id equal to its array index.
                                    (TimerCallbackFunction_t) rxTimerCallback     // Timer callback when it expires.
                                    );
	  /* creation of TX timer */
	  nextionHMI_h.blockTx = xTimerCreate("TxTimer",         // Just a text name, not used by the kernel.
		  	  	  	  	  	  	  	TOUT_PERIOD_CALC(nextionHMI_h.pUart->Init.BaudRate) , // The minimum time between sending commands
                                    pdFALSE,         // The timers will auto-reload themselves when they expire.
									( void * )0,     // Assign each timer a unique id equal to its array index.
                                    (TimerCallbackFunction_t) txTimerCallback	  // Timer callback when it expires.
                                    );

	  /* creation of semaphore */
	  nextionHMI_h.hmiUartTxSem = xSemaphoreCreateBinary();

	  xSemaphoreGive(nextionHMI_h.hmiUartTxSem);

}

/**
 * @brief Add Nextion objects to object array .
 * @note
 *
 * @param *pOb_handle  Nextion object
 * @retval 0-failed, 1-success
 */
Ret_Status_t NxHmi_AddObject(Nextion_Object_t *pOb_handle) {
	if (Nextion_Object_Count < NEX_MAX_OBJECTS) {
		Nextion_Object_List[Nextion_Object_Count] = pOb_handle;
		Nextion_Object_Count++;

		return STAT_OK;
	}

	return STAT_FAILED;
}



/**
 * @brief Set text for txt type of Nextion object
 * @note  Send text
 *
 * @param *pOb_handle = Nextion object handler
 * @param *buffer = string pointer
 * @retval see @ref waitForAnswer() function for return value
 */
Ret_Status_t NxHmi_SetText(Nextion_Object_t *pOb_handle, const char *buffer) {

	prepareToSend();
	sprintf(txBuf, "%s.txt=\"%s\"", pOb_handle->Name, buffer);
	HmiSendCommand(txBuf);

	return waitForAnswer(NULL);
}

/**
 * @brief Set integer number for int type of Nextion object
 * @note  Send number
 *
 * @param *pOb_handle = Nextion object handler
 * @param number = integer
 * @retval see @ref waitForAnswer() function for return value
 */
Ret_Status_t NxHmi_SetIntValue(Nextion_Object_t *pOb_handle, int16_t number) {

	prepareToSend();
	sprintf(txBuf, "%s.val=%i", pOb_handle->Name, number);
	HmiSendCommand(txBuf);

	return waitForAnswer(NULL);
}

/**
 * @brief Set float number for int type of Nextion object
 * @note  Send float number
 *
 * @param *pOb_handle = Nextion object handler
 * @param number = float number
 * @retval see @ref waitForAnswer() function for return value
 */
Ret_Status_t NxHmi_SetFloatValue( Nextion_Object_t *pOb_handle, float number) {

	prepareToSend();
	sprintf(txBuf, "%s.txt=\"%.2f\"", pOb_handle->Name, number);
	HmiSendCommand(txBuf);

	return waitForAnswer(NULL);
}

/**
 * @brief Wait and return the displays answer if it's enabled
 * @note  Put in a comment the #define NEX_VERBOSE_COMM line in the header file
 * 		  if confirmation for successful command is not required
 *
 * @param  Pointer for returned data, use NULL if returned data is not required
 * @retval 	STAT_ERROR 		= <not used here>
 * 			STAT_TIMEOUT	= timeout occurred, no confirmation received
 * 			STAT_FAILED		= command execution failed
 * 			STAT_OK 		= command was successfully executed
 *
 */
Ret_Status_t waitForAnswer(Ret_Command_t *pRetCommand) {
	Ret_Command_t tmpCommand;
	if(pRetCommand == NULL){
		//Pass the address to pointer
		pRetCommand = &tmpCommand;
	}
		//This case the HMI is in silent mode, no command execution confirmation
	if(nextionHMI_h.ifaceVerbose == 0) {
		if(pRetCommand == &tmpCommand){
			//We not expecting any incoming data
			return STAT_OK;

		} else {
			//We expecting a return value
			if(xQueueReceive(nextionHMI_h.rxCommandQHandle, pRetCommand, NEX_ANSW_TIMEOUT) == pdFALSE){
				//Timeout
				return STAT_TIMEOUT;

			} else {
				// Return value arrived
				return STAT_OK;
			} // end if data has been received
		}// end if NULL and Verbose is 0

	} else {

		if(pRetCommand == &tmpCommand){
			//We not expecting any incoming data, but a return value is possible
			if( xQueueReceive(nextionHMI_h.rxCommandQHandle, pRetCommand, pdMS_TO_TICKS(5) ) == pdTRUE) {
				//Return value arrived before timeout occurred
				if(pRetCommand->cmdCode != NEX_EVENT_SUCCESS) {
					//If the returned command code is anything other than SUCCESS
					return STAT_FAILED;
				}
			}
		} else {
			//We expecting a return value
			xQueueReceive(nextionHMI_h.rxCommandQHandle, pRetCommand, NEX_ANSW_TIMEOUT);

		}
		return STAT_OK;
	}// end if verbose level is greater than 0
}


//////////////////////////STATIC FUNCTIONS////////////////////////////


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
				}//end if PressC nNULL
			}//end if TOUCH event
			else if (NEX_EVENT_RELEASE == event) {
				if (handle->ReleaseCallback != NULL) {
					handle->ReleaseCallback();
				}//end if ReleaseC nNULL
			}//end if RELEASE event

			break; //object found break the loop
		}//end if component IDs match
	}//end for loop
}

/**
 * @brief Search for return value or event from Nextion display
 * @note  Static function, intended for internal task
 *
 * @param *buff = Pointer for the (RX) incoming data buffer
 * @param buffSize = Amount of incoming data
 * @retval int8_t = 0-No more data, 1-More data in the stream, <0 -Error
 */
static int8_t HmiCmdFromStream(uint8_t *buff, uint8_t buffSize) {

	HAL_UART_AbortReceive_IT(nextionHMI_h.pUart);
	memset(buff, BUFF_CLEAR_PATTERN, buffSize);

	uint8_t termPatternCnt = 0;
	uint8_t isRawData = 0;
	uint8_t helper = nextionHMI_h.rxPosition;

	isRawData = isItRawData();

	if(isRawData == 0) {
		//loop till end of stream
		for(; nextionHMI_h.rxPosition < nextionHMI_h.rxCounter; nextionHMI_h.rxPosition++) {
			//if found termination character, break the loop
			if(nextionHMI_h.rxBuff[nextionHMI_h.rxPosition] == 0xFF) {
				break;
			} else {
				if((nextionHMI_h.rxPosition - helper) < buffSize) {
					//copy serial stream to command buffer
					buff[nextionHMI_h.rxPosition - helper] = nextionHMI_h.rxBuff[nextionHMI_h.rxPosition];
				} else {
					//buffer overflow, TODO: drop everything?
					memset(buff, BUFF_CLEAR_PATTERN, buffSize);
					nextionHMI_h.errorCnt++;
					return -3;
				}//end if

			} //end if 0xFF
		}//end for loop

		//count the terminator characters (Nextion always send 3 pcs)
		for( ; nextionHMI_h.rxBuff[nextionHMI_h.rxPosition] == 0xFF ; ) {
			//if we have reached the end of the stream
			if( (nextionHMI_h.rxPosition) >= nextionHMI_h.rxCounter) {
				break;
			}
			nextionHMI_h.rxPosition++;
			termPatternCnt++;
		}//end for loop

	} else {
		//Raw data
		buff[0] = NEX_RET_NUMBER_HEAD;
		for(uint8_t i = 0; i < 4; i++){
			buff[i+1] = nextionHMI_h.rxBuff[i];
		}//end for loop
		nextionHMI_h.rxPosition += 4;
	}//else raw data checking

	if( nextionHMI_h.rxPosition >= nextionHMI_h.rxCounter ) {
		//Reach the end
		nextionHMI_h.rxPosition = nextionHMI_h.rxCounter = 0;
		//to store data at beginning of the buffer
		//HAL_UART_AbortReceive_IT(nextionHMI_h.pUart);
		memset(nextionHMI_h.rxBuff, BUFF_CLEAR_PATTERN, NEX_RX_BUFF_SIZE);
		HAL_UART_Receive_IT(nextionHMI_h.pUart, &nextionHMI_h.rxBuff[nextionHMI_h.rxCounter], 1);

		if(termPatternCnt == 3 || (isRawData == 1) ) {
			nextionHMI_h.cmdCnt++;
			return 0; //no more data
		} else {
			nextionHMI_h.errorCnt++;
			return -2; // error
		}

	} else {
		//More data in the buffer
		if(termPatternCnt == 3 || (isRawData == 1)) {
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
static void validateCommand(uint8_t *cmdBuff) {
	Ret_Command_t command;
	memset(&command, 0x00, sizeof(Ret_Command_t));
	uint8_t sendQueue = 1;

	if(cmdBuff[0] == 0x00 && cmdBuff[1] == BUFF_CLEAR_PATTERN ) {
		//Invalid instruction
		command.cmdCode = 0x00;
		nextionHMI_h.errorCnt++;
	} else if(cmdBuff[0] > 0x01 && cmdBuff[0] <= 0x23) {
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
				break;

			default:
				command.cmdCode = 0x00;
				sendQueue = 0;
				break;
		}//end switch
	}//end if

	if(sendQueue) {
		xQueueSend(nextionHMI_h.rxCommandQHandle, &command, NEX_QUEUE_TIMEOUT);
		if(uxQueueSpacesAvailable(nextionHMI_h.rxCommandQHandle) == 0) {
			//If no task will "consume" the messages, then delete, otherwise will block
		//    the hmiRxTask permanently
			xQueueReset(nextionHMI_h.rxCommandQHandle);
		}//end if no free space in queue
	}//end if send queue

	if(uxQueueSpacesAvailable(nextionHMI_h.objectQueueHandle) == 0) {
		xQueueReset(nextionHMI_h.objectQueueHandle);
	}//end if no free space in queue

}

/**
 * @brief Attach the terminating characters at the end of the command string
 * @note  Static function, intended for internal task
 *
 * @param *cmd = command string value
 * @retval void
 */
void HmiSendCommand(const char *cmd) {

    char term[4] = {0xFF, 0xFF, 0xFF, 0x00};

	sprintf(txBuf, "%s%s", cmd, term);

    while (HAL_UART_GetState(nextionHMI_h.pUart) == HAL_UART_STATE_BUSY) {
    	osDelay(1);
    }
    //Start data transmission
    HAL_UART_Transmit_IT(nextionHMI_h.pUart,(uint8_t*)txBuf, strlen(txBuf) );
    //Block the task until data has been transmitted
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

}

/**
 * @brief Identification of raw data in an incoming stream
 * @note  RAW data sent in 4 byte 32-bit little endian order
 * TODO: improvement required!
 * @param void
 * @retval 1 - RAW data identified, not a RAW data
 */
static int8_t isItRawData(void) {
	uint32_t tempNr;

	if( ( nextionHMI_h.rxCounter == 4 ) && (nextionHMI_h.xTaskToNotify == NULL) ) {
		//Likely to be raw data
		tempNr = *(uint32_t*)&nextionHMI_h.rxBuff[nextionHMI_h.rxPosition];
		tempNr = tempNr & 0xFFFFFF00;
		if(tempNr != 0xFFFFFF00){
			//Raw data
			return 1;
		} else {
			//Command or Raw data value is higher or equal than 16 777 215 (0xFFFFFF)
			//TODO: For now let's assume, the returned data is not a raw data
			return 0;
		}

	}
	return 0;
}

/**
 * @brief Prepare to send a command
 * @note  Wait for semaphore and for the interface to be in IDLE mode
 *
 * @param void
 * @retval void
 */
void prepareToSend(void) {
	//Wait for the semaphore to get access to the UART
	xSemaphoreTake(nextionHMI_h.hmiUartTxSem, portMAX_DELAY);

    /* At this point xTaskToNotify should be NULL as no transmission
    is in progress. */
	configASSERT( nextionHMI_h.xTaskToNotify == NULL );

    /* Store the handle of the calling task. */
    nextionHMI_h.xTaskToNotify = xTaskGetCurrentTaskHandle();

}

