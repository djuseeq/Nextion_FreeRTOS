/*
 * Nextion_HMI.h
 *
 *  Created on: Jan 19, 2021
 *      Author: György Kovács
 */

#ifndef NEXTION_HMI_INC_NEXTION_HMI_H_
#define NEXTION_HMI_INC_NEXTION_HMI_H_

#ifdef __cplusplus
extern "C" {
#endif


#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "timers.h"
#include "semphr.h"

#include "string.h"
#include "stdio.h"

#define NEX_DEFAULT_SPEED (9600U)
#define NEX_RX_BUFF_SIZE 20
#define NEX_MAX_OBJECTS 50
#define NEX_ANSW_TIMEOUT pdMS_TO_TICKS(3000) // in milliseconds
#define NEX_QUEUE_TIMEOUT pdMS_TO_TICKS(1000) // in milliseconds

#define NEX_EVENT_SUCCESS (0x01)
#define NEX_EVENT_INIT_OK (0x88)
#define NEX_EVENT_UPGRADE (0x89)
#define NEX_EVENT_TOUCH_HEAD (0x65)
#define NEX_EVENT_POSITION_HEAD (0x67)

#define NEX_RET_CURRENT_PAGEID_HEAD (0x66)
#define NEX_RET_STRING_HEAD (0x70)
#define NEX_RET_NUMBER_HEAD (0x71)

#define NEX_RET_INVALID_CMD (0x00)
#define NEX_RET_INVALID_COMPONENT_ID (0x02)
#define NEX_RET_INVALID_PAGE_ID (0x03)
#define NEX_RET_INVALID_PICTURE_ID (0x04)
#define NEX_RET_INVALID_FONT_ID (0x05)
#define NEX_RET_INVALID_BAUD (0x11)
#define NEX_RET_INVALID_VARIABLE (0x1A)
#define NEX_RET_INVALID_OPERATION (0x1B)

#define NEX_EVENT_TOUCH (0x01)
#define NEX_EVENT_RELEASE (0x00)

// 1000ms/bps * 10 bit + 2ms is extra safety, for RX timer timeout
#define TOUT_PERIOD_CALC(bps) pdMS_TO_TICKS( ( ( (1000000U/bps) * 10U) / 1000U) + 2U )
#define MAP_NR(x, iMin, iMax, oMin, oMax) ( (x - iMin) * (oMax - oMin) / (iMax - iMin) + oMin)

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
	COMP_BUSY,
	COMP_WAITANSW
} NxCompRetStatus_t;

typedef struct Nextion_Object_t {
	char *Name;
	uint8_t Page_ID;
	uint8_t Component_ID;
	//uint8_t Visible;
	//uint8_t Enabled;
	Nx_Object_Type_t dataType;
	void (*PressCallback)(); //callback function ptr
	void (*ReleaseCallback)();	 //callback function ptr

} Nextion_Object_t;

typedef struct Nextion_HMI_Handler_t {
	UART_HandleTypeDef *pUart;

	//uint8_t backLight; // 0 - 100 %
	//uint16_t noComSleep; //No serial event. Seconds, 3 - 65535, 0 - disabled
	//uint16_t noTchSleep; //No touch event. Seconds, 3 - 65535, 0 - disabled
	//uint8_t autoWake;    //
	uint8_t rxBuff[NEX_RX_BUFF_SIZE];
	uint8_t rxCounter;
	uint8_t rxPosition;
	uint16_t errorCnt;
	uint16_t cmdCnt;
	NxCompRetStatus_t hmiStatus;

	///RTOS stuff
	xTimerHandle rxTimerHandle;
	SemaphoreHandle_t hmiSmphrHandle;
	osMessageQueueId_t rxCommandQHandle;
	osMessageQueueId_t rxNumber;
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

Nextion_HMI_Handler_t nextionHMI_h;

char txBuf[30];

void NxHmi_Init(UART_HandleTypeDef *huart);
uint8_t NxHmi_AddObject(Nextion_Object_t *pOb_handle);
void NxHmi_GotoPage(uint8_t pageId);
void NxHmi_SetText(Nextion_Object_t *pOb_handle,const char *buffer);
void NxHmi_SetIntValue(Nextion_Object_t *pOb_handle, int16_t number);
void NxHmi_SetFloatValue(Nextion_Object_t *pOb_handle, float number);
void NxHmi_SetBcoColour(Nextion_Object_t *pOb_handle, int16_t color);
void NxHmi_SetObjectVisibility(Nextion_Object_t *pOb_handle, Ob_visibility_t visible);
void NxHmi_SetBacklight(uint8_t value, Cnf_permanence_t cnfSave);
int32_t NxHmi_GetObjValue(Nextion_Object_t *pOb_handle);
int32_t NxHmi_ResetDevice(void);
//void NxHmi_ComSpeed(uint32_t baud, Cnf_permanence_t cnfSave);
void rxTimerCallback(void *argument);



#ifdef __cplusplus
}
#endif


#endif /* NEXTION_HMI_INC_NEXTION_HMI_H_ */
