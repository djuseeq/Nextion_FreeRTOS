/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "Nextion_HMI.h"
#include "stdlib.h" // for rand() function
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
Nextion_Object_t txtObj1, txtObj2, intObj1, slider1Obj, gauge1Obj, prgressBarObj, btn1Obj, pgButton,
					waveForm;
uint8_t currentPage = 0xFF;
/* USER CODE END Variables */
/* Definitions for Display1 */
osThreadId_t Display1Handle;
const osThreadAttr_t Display1_attributes = {
  .name = "Display1",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Blinky */
osThreadId_t BlinkyHandle;
const osThreadAttr_t Blinky_attributes = {
  .name = "Blinky",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Display2 */
osThreadId_t Display2Handle;
const osThreadAttr_t Display2_attributes = {
  .name = "Display2",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void initObjects(void);
void sendBack(void);
void changeSpeed(void);
void askSliderValue(void);
void btnPress(void);
void nextPage(void);
/* USER CODE END FunctionPrototypes */

void StartDisplay1Task(void *argument);
void StartTaskBlinky(void *argument);
void StartDisplay2Task(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
	initObjects();
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of Display1 */
  Display1Handle = osThreadNew(StartDisplay1Task, NULL, &Display1_attributes);

  /* creation of Blinky */
  BlinkyHandle = osThreadNew(StartTaskBlinky, NULL, &Blinky_attributes);

  /* creation of Display2 */
  Display2Handle = osThreadNew(StartDisplay2Task, NULL, &Display2_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDisplay1Task */
/**
  * @brief  Function implementing the Display1 thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDisplay1Task */
void StartDisplay1Task(void *argument)
{
  /* USER CODE BEGIN StartDisplay1Task */
	uint8_t progressBarVal = 0;

	NxHmi_GetCurrentPageId(&currentPage);

	//NxHmi_SetAutoSleep(0, 10, 0, 1);
	/* Infinite loop */
	for (;;) {
		switch (currentPage) {

		case 0:
			//PULSE();
			NxHmi_SetIntValue(&prgressBarObj, progressBarVal);
			// PULSE();
			if (progressBarVal++ >= 100) {
				progressBarVal = 0;
			}
			break;

		case 1:
			//Waveform component height is 200px
			//Upper (channel 0) part (100 - 200 px)
			NxHmi_WaveFormAddValue(&waveForm, 0, 100 + rand() % 100);

			//Lower ( channel 1) part (0 - 100 px)
			NxHmi_WaveFormAddValue(&waveForm, 1, rand() % 100);
			break;

		default:
			break;
		}//end switch
		osDelay(10);
	}
  /* USER CODE END StartDisplay1Task */
}

/* USER CODE BEGIN Header_StartTaskBlinky */
/**
* @brief Function implementing the Blinky thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTaskBlinky */
void StartTaskBlinky(void *argument)
{
  /* USER CODE BEGIN StartTaskBlinky */
  /* Infinite loop */
  for(;;)
  {
	  HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    osDelay(500);
  }
  /* USER CODE END StartTaskBlinky */
}

/* USER CODE BEGIN Header_StartDisplay2Task */
/**
* @brief Function implementing the Display2 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDisplay2Task */
void StartDisplay2Task(void *argument)
{
  /* USER CODE BEGIN StartDisplay2Task */
  /* Infinite loop */
	uint8_t hossz = 0;

	for (;;) {
		switch (currentPage) {

		case 0:
			//PULSE();
			//PULSE();
			NxHmi_SetBcoColourRGB(&txtObj2, rand() % 100, rand() % 100, rand() % 100);
			//PULSE();
			//PULSE();
			break;

		case 1:
			//NxHmi_ForceRedrawComponent(NULL);
			//NxHmi_DrawImage(0, rand() % 100, 220 + rand() % 100);
			//NxHmi_DrawImage(0, 0, 350);
			//NxHmi_DrawCropImage(1, 20, 276, hossz++, 40, 0, 0);
			//NxHmi_DrawLine(10, 240, hossz++, 220 + rand() % 20, NEX_BLUE);
			NxHmi_DrawRect(10, 240, 200, 250, NEX_BLUE, 1);
			NxHmi_DrawRect(20, 290, 100, 300, NEX_GREEN, 0);
			NxHmi_DrawCircle(100, 270, 60, NEX_RED, 1);
			NxHmi_DrawCircle(150, 290, 30, NEX_GREEN, 0);
			if (hossz == 220) {
				hossz = 0;
				NxHmi_ForceRedrawComponent(NULL);
			}
			break;

		default:
			break;
		}				//end switch
		osDelay(10);
	}
  /* USER CODE END StartDisplay2Task */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void initObjects(void){

	txtObj1.Name = "t0";		//component name, from Nextion editor
	txtObj1.Component_ID = 1;	//component id, from Nextion editor
	txtObj1.Page_ID = 0;		//page number where the component exists, from Nextion editor
	txtObj1.dataType = OBJ_TYPE_TXT;	//component value type
	txtObj1.PressCallback = &sendBack;	//press event function pointer
	txtObj1.ReleaseCallback = NULL;		//no function is assigned to release event
	NxHmi_AddObject(&txtObj1);	//finally add the component to the object array

	intObj1.Name = "n0";
	intObj1.Page_ID = 0;
	intObj1.Component_ID = 2;
	intObj1.PressCallback = &changeSpeed;
	intObj1.ReleaseCallback = NULL;
	NxHmi_AddObject(&intObj1);

	slider1Obj.Name = "h0";
	slider1Obj.Page_ID = 0;
	slider1Obj.Component_ID = 7;
	slider1Obj.dataType = OBJ_TYPE_INT;
	slider1Obj.PressCallback = NULL;
	slider1Obj.ReleaseCallback = &askSliderValue;
	NxHmi_AddObject(&slider1Obj);

	btn1Obj.Name = "bt0";
	btn1Obj.Page_ID = 0;
	btn1Obj.Component_ID = 3;
	btn1Obj.dataType = OBJ_TYPE_INT;
	btn1Obj.PressCallback = &btnPress;
	btn1Obj.ReleaseCallback = NULL;
	NxHmi_AddObject(&btn1Obj);

	pgButton.Name = "b0";
	pgButton.Page_ID = 0;
	pgButton.Component_ID = 8;
	pgButton.dataType = OBJ_TYPE_INT;
	pgButton.PressCallback = NULL;
	pgButton.ReleaseCallback = &nextPage;
	NxHmi_AddObject(&pgButton);

	pgButton.Name = "b0";
	pgButton.Page_ID = 0;
	pgButton.Component_ID = 8;
	pgButton.dataType = OBJ_TYPE_INT;
	pgButton.PressCallback = NULL;
	pgButton.ReleaseCallback = &nextPage;
	NxHmi_AddObject(&pgButton);

	txtObj2.Name = "t1";
	txtObj2.Component_ID = 4;
	txtObj2.Page_ID = 0;
	txtObj2.PressCallback = NULL;
	txtObj2.ReleaseCallback = NULL;
	//Not required to add to Object Array List, no callback is associated for this object.
	//NxHmi_AddObject(&txtObj2);

	waveForm.Name = "s0";
	waveForm.Page_ID = 1;
	waveForm.Component_ID = 1;
	waveForm.dataType = OBJ_TYPE_INT;
	waveForm.PressCallback = NULL;
	waveForm.ReleaseCallback = NULL;
	//NxHmi_AddObject(&waveForm);

	gauge1Obj.Name = "z0";
	gauge1Obj.Page_ID = 0;
	gauge1Obj.Component_ID = 5;
	gauge1Obj.dataType = OBJ_TYPE_INT;
	gauge1Obj.PressCallback = NULL;
	gauge1Obj.ReleaseCallback = NULL;
	//NxHmi_AddObject(&slider1Obj);

	prgressBarObj.Name = "j0";
	prgressBarObj.Page_ID = 0;
	prgressBarObj.Component_ID = 6;
	prgressBarObj.dataType = OBJ_TYPE_INT;
	prgressBarObj.PressCallback = NULL;
	prgressBarObj.ReleaseCallback = NULL;
	//NxHmi_AddObject(&prgressBarObj);
}

void sendBack(void){
	uint16_t rndszam = 0;
	//SEGGER_SYSVIEW_Start();
	  NxHmi_SetText(&txtObj1, "Sok");

	  rndszam = (10 + rand() ) % 90;
	  NxHmi_SetIntValue(&intObj1, rndszam);
	  //NxHmi_SetBcoColour(&txtObj1, rand() % 65535);
	  NxHmi_SetBcoColourRGB(&txtObj1, rand() % 100, rand() % 100, rand() % 100);
	  NxHmi_SetBacklight(50, SET_TEMPORARY);
}

void btnPress(void){
	uint32_t tmpInt;
	if(NxHmi_GetObjValue(&btn1Obj, &tmpInt) == NEX_EVENT_SUCCESS) {
		if(!tmpInt){
			NxHmi_SetBcoColour(&gauge1Obj, 65504); // yellow
		} else {
			NxHmi_SetBcoColour(&gauge1Obj, 2016); // green
		}
	}//end if success
}

void changeSpeed(void){
	//NxHmi_Baud_Rate(9600, 0);
	NxHmi_CalibrateTouchSensor();
}

void askSliderValue(void){
	uint32_t tempValue;
	Ret_Status_t retAnswer = STAT_ERROR;
	//PULSE();
	//PULSE();
	retAnswer = NxHmi_GetObjValue(&slider1Obj, &tempValue);
	while(retAnswer == STAT_ERROR);
	if(retAnswer == STAT_OK){
		NxHmi_SetIntValue(&gauge1Obj, MAP_NR(tempValue, 0, 100, 0, 360));
		char bufff[10];
		//uint8_t ttmmpp[700]; // to causing stack overflow
		sprintf(bufff, "%ld", tempValue);
		NxHmi_SetText(&txtObj2, bufff);
		//PULSE();
	}
}

void nextPage(void){
	//NxHmi_ResetDevice();
	if(NxHmi_GotoPage(1) == STAT_OK ){
		currentPage = 1;
	}

}
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
