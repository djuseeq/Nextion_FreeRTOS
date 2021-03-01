/*
 * Nextion_HMI.h
 *
 *  Created on: Jan 19, 2021
 *      Author: György Kovács
 *      Copyright (c) 2019, György Kovács
 * The MIT License (MIT)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _NEXTION_HMI_H_
#define _NEXTION_HMI_H_

#ifdef __cplusplus
extern "C" {
#endif

///INCLUDES
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "timers.h"
#include "semphr.h"
#include "string.h"
#include "stdio.h"

//DEFINES
#define NEX_DEFAULT_SPEED 			(9600U)

//#define NEX_VERBOSE_COMM

#define NEX_RX_BUFF_SIZE 			(20) // UART RX buffer size
#define NEX_MAX_OBJECTS 			(50) //maximum objects on the display

#define NEX_ANSW_TIMEOUT 			pdMS_TO_TICKS(3000) // in milliseconds
#define NEX_QUEUE_TIMEOUT 			pdMS_TO_TICKS(1000) // in milliseconds

#define NEX_HMIOBJECTTASK_STACK 	(256 * 4) // stack size
#define NEX_HMIOBJECTTASK_PRIORITY 	osPriorityNormal

#define NEX_HMIRXTASK_STACK 		(128 * 4) // stack size
#define NEX_HMIRXTASK_PRIORITY 		osPriorityNormal
#define BUFF_CLEAR_PATTERN 			(0xAA)

#define NEX_EVENT_SUCCESS 			(0x01)
#define NEX_EVENT_INIT_OK 			(0x88)
#define NEX_EVENT_UPGRADE 			(0x89)
#define NEX_EVENT_TOUCH_HEAD 		(0x65)
#define NEX_EVENT_POSITION_HEAD 	(0x67)

#define NEX_RET_CURRENT_PAGEID_HEAD (0x66)
#define NEX_RET_STRING_HEAD 		(0x70)
#define NEX_RET_NUMBER_HEAD 		(0x71)

#define NEX_RET_INVALID_CMD			(0x00)
#define NEX_RET_INVALID_COMPONENT_ID (0x02)
#define NEX_RET_INVALID_PAGE_ID 	(0x03)
#define NEX_RET_INVALID_PICTURE_ID 	(0x04)
#define NEX_RET_INVALID_FONT_ID 	(0x05)
#define NEX_RET_INVALID_BAUD 		(0x11)
#define NEX_RET_INVALID_VARIABLE 	(0x1A)
#define NEX_RET_INVALID_OPERATION 	(0x1B)

#define NEX_EVENT_TOUCH 			(0x01)
#define NEX_EVENT_RELEASE 			(0x00)

///16BIT COLOR CODES
#define NEX_BLACK					(0)
#define NEX_WHITE					(65535)
#define NEX_BLUE					(31)
#define NEX_GREEN					(2016)
#define NEX_RED						(63488)
#define NEX_GRAY					(33840)
#define NEX_YELLOW					(65504)
#define NEX_BROWN					(48192)

// 1000ms/bps * 10 bit + 2ms is extra safety, for RX timer timeout
#define TOUT_PERIOD_CALC(bps) 		pdMS_TO_TICKS( ( ( (1000000U/bps) * 10U) / 1000U) + 2U )
#define MAP_NR(x, iMin, iMax, oMin, oMax) 	( (x - iMin) * (oMax - oMin) / (iMax - iMin) + oMin)

typedef enum {
	OBJ_HIDE = 0,
	OBJ_SHOW = 1
} Ob_visibility_t;


typedef enum {
	SET_TEMPORARY = 0,
	SET_PERMANENT = 1
} Cnf_permanence_t;


typedef enum {
	OBJ_TYPE_BTN = 0,
	OBJ_TYPE_INT,
	OBJ_TYPE_TXT,
} Nx_Object_Type_t;


typedef enum {
	COMP_IDLE = 0,
	COMP_BUSY_TX,
	COMP_BUSY_RX
} NxCompRetStatus_t;


typedef enum {
	STAT_ERROR = -2,
	STAT_TIMEOUT,
	STAT_FAILED,
	STAT_OK
} Ret_Status_t;


typedef struct Nextion_Object_t {
	char *Name;
	uint8_t Page_ID;
	uint8_t Component_ID;
	//uint8_t Visible;
	//uint8_t Enabled;
	Nx_Object_Type_t dataType;
		//If no function is assigned to event pointers, then initialize the function pointer wit NULL
	void (*PressCallback)();	//Press event callback function pointer
	void (*ReleaseCallback)();	//Release event callback function pointer

} Nextion_Object_t;


typedef struct Nextion_HMI_Handler_t {
	UART_HandleTypeDef *pUart;

	uint8_t rxBuff[NEX_RX_BUFF_SIZE];
	uint8_t rxCounter;
	uint8_t rxPosition;
	uint16_t errorCnt;
	uint16_t cmdCnt;
	uint8_t ifaceVerbose;
	NxCompRetStatus_t hmiStatus;

	///RTOS stuff
	TaskHandle_t xTaskToNotify;
	xTimerHandle blockTx;
	xTimerHandle rxTimerHandle;
	SemaphoreHandle_t hmiUartTxSem;
	osMessageQueueId_t rxCommandQHandle;
	osMessageQueueId_t objectQueueHandle;

} Nextion_HMI_Handler_t;


typedef struct Ret_Command_t {
	uint8_t cmdCode;
	uint8_t event;
	uint8_t pageId;
	uint8_t cmpntId;
	uint8_t touchEvent;
	uint16_t xCoordinate;
	uint16_t yCoordinate;
	uint32_t numData;
	char *stringData;

}Ret_Command_t;


/* Definitions for hmiTask */
osThreadId_t hmiObjectTaskHandle;
osThreadId_t hmiRxTaskHandle;

///Variables
Nextion_HMI_Handler_t nextionHMI_h;
char txBuf[30];

void HmiSendCommand(const char *cmd);
Ret_Status_t waitForAnswer(Ret_Command_t *pRetCommand);
void rxTimerCallback(void *argument);
void txTimerCallback(void *argument);
void prepareToSend(void);

	///Public function prototypes
void NxHmi_Init(UART_HandleTypeDef *huart);
Ret_Status_t NxHmi_AddObject(Nextion_Object_t *pOb_handle);
Ret_Status_t NxHmi_SetText(Nextion_Object_t *pOb_handle,const char *buffer);
Ret_Status_t NxHmi_SetIntValue(Nextion_Object_t *pOb_handle, int16_t number);
Ret_Status_t NxHmi_SetFloatValue(Nextion_Object_t *pOb_handle, float number);

//System commands
void NxHmi_Verbosity(uint8_t vLevel);
Ret_Status_t NxHmi_SetBacklight(uint8_t value, Cnf_permanence_t cnfSave);
Ret_Status_t NxHmi_SendXYcoordinates(uint8_t status);
Ret_Status_t NxHmi_Sleep(uint8_t status);
Ret_Status_t NxHmi_SetAutoSleep(uint16_t slNoSer, uint16_t slNoTouch, uint8_t wkpSer, uint8_t wkpTouch);
//void NxHmi_ComSpeed(uint32_t baud, Cnf_permanence_t cnfSave);

//Operational commands
Ret_Status_t NxHmi_ForceRedrawComponent(Nextion_Object_t *pOb_handle);
void NxHmi_CalibrateTouchSensor(void);
Ret_Status_t NxHmi_GotoPage(uint8_t pageId);
Ret_Status_t NxHmi_SetObjectVisibility(Nextion_Object_t *pOb_handle, Ob_visibility_t visible);
Ret_Status_t NxHmi_GetObjValue(Nextion_Object_t *pOb_handle, uint32_t *pValue);
Ret_Status_t NxHmi_ResetDevice(void);
Ret_Status_t NxHmi_GetCurrentPageId(uint8_t *pValue);
void NxHmi_WaveFormAddValue(Nextion_Object_t *pOb_handle, uint8_t channel, uint8_t value);
Ret_Status_t NxHmi_WaveFormClearChannel(Nextion_Object_t *pOb_handle, uint8_t channel);

//GUI commands
Ret_Status_t NxHmi_SetBcoColour(Nextion_Object_t *pOb_handle, uint16_t color);
Ret_Status_t NxHmi_SetBcoColourRGB(Nextion_Object_t *pOb_handle, uint8_t red, uint8_t green, uint8_t blue);
Ret_Status_t NxHmi_DrawImage(uint8_t picId, uint16_t xAxis, uint16_t yAxis);

Ret_Status_t NxHmi_DrawCropImage(uint8_t picId, uint16_t xPane, uint16_t yPane,
									uint16_t width, uint16_t height, uint16_t xImg, uint16_t yImg);

Ret_Status_t NxHmi_DrawLine(uint16_t startX, uint16_t startY, uint16_t endX, uint16_t endY, uint16_t color);

Ret_Status_t NxHmi_DrawRect(uint16_t startX, uint16_t startY, uint16_t endX,
								uint16_t endY, uint16_t color, uint8_t fMode);

Ret_Status_t NxHmi_DrawCircle(uint16_t centX, uint16_t centY, uint16_t radius, uint16_t color, uint8_t fMode);


#ifdef __cplusplus
}
#endif


#endif /* _NEXTION_HMI_H_ */
