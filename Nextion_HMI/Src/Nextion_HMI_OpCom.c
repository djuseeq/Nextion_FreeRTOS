/*
 * Nextion_HMI_OpCom.c
 *
 *  Created on: Feb 15, 2021
 *      Author: György Kovács
 *
 *      Operational commands
 */

#include "Nextion_HMI.h"


/**
 * @brief Force display to refresh/redraw component
 * @note  Not mandatory, "auto-refresh when attribute changes since v0.38"
 *
 * @param *pOb_handle = Nextion object handler,
 * 			if the passed value is NULL, then refresh the complete page
 * @retval see @ref waitForAnswer() function for return value
 */
FNC_Ret_Status_t NxHmi_ForceRedrawComponent(Nextion_Object_t *pOb_handle) {

	prepareToSend();
	if(pOb_handle != NULL) {
		sprintf(txBuf, "ref %s", pOb_handle->Name); //refresh just the selected component
	} else {
		sprintf(txBuf, "ref %i", 0);	//refresh the current page
	}
	HmiSendCommand(txBuf);

	return waitForAnswer(NULL);
}

/**
 * @brief Start touch sensor calibration procedure
 * @note After acknowledging the command, the MCU sends a command, and
 * 			when the calibration is complete, the display will respond so we know
 * 			the display is ready to receive further commands.
 *
 * @param void
 * @retval void
 */
void NxHmi_CalibrateTouchSensor(void) {

	prepareToSend();
	xQueueReset(nextionHMI_h.objectQueueHandle);
	xQueueReset(nextionHMI_h.rxCommandQHandle);
	sprintf(txBuf, "touch_j");
	HmiSendCommand( txBuf);
	if(waitForAnswer(NULL) == NEX_RET_INVALID_CMD ) {
		nextionHMI_h.hmiStatus = COMP_IDLE;
			//NxHmi_GetCurrentPageId();
	} else {
		// polling the display ,when is ready
		uint8_t ttt;
		FNC_Ret_Status_t ret;

		ret = NxHmi_GetCurrentPageId(&ttt);
		if(ret != STAT_OK){
			for(uint8_t i = 0; i < 10; i++){
				nextionHMI_h.hmiStatus = COMP_WAITANSW;
				if(waitForAnswer(NULL) != STAT_OK){
					//wait until the calibration is finished,
					// or give up after a certain amount of time. (i * NEX_ANSW_TIMEOUT)
				} else {
					//Give up
					break;
				}
			}
		}

		nextionHMI_h.hmiStatus = COMP_IDLE;
	}


}

/**
 * @brief Set Nextion object visibility
 * @note  Show/hide object
 *
 * @param *pOb_handle = Nextion object handler
 * @param visible = OBJ_HIDE, OBJ_SHOW
 * @retval see @ref waitForAnswer() function for return value
 */
FNC_Ret_Status_t NxHmi_SetObjectVisibility(Nextion_Object_t *pOb_handle, Ob_visibility_t visible) {

	prepareToSend();
	sprintf(txBuf, "vis %s,%d", pOb_handle->Name, visible);
	HmiSendCommand(txBuf);

	return waitForAnswer(NULL);
}

/**
 * @brief Change to the specified page
 * @note  Default page is 0
 *
 * @param pageId = Page number, 0-default
 * @retval see @ref waitForAnswer() function for return value
 */
FNC_Ret_Status_t NxHmi_GotoPage(uint8_t pageId) {

	prepareToSend();
	sprintf(txBuf, "page %i", pageId);
	HmiSendCommand(txBuf);

	return waitForAnswer(NULL);
}

/**
 * @brief Get a Nextion objects value
 * @note  TODO: implement string ret value
 *
 * @param *pOb_handle = Nextion object handler
 * @param *pValue = Pointer for the returned 32bit number
 * @retval see @ref waitForAnswer() function for return value
 */
FNC_Ret_Status_t NxHmi_GetObjValue(Nextion_Object_t *pOb_handle, uint32_t *pValue) {

	prepareToSend();
	*pValue = 0;
	Ret_Command_t retNumber;

	switch (pOb_handle->dataType) {
		case OBJ_TYPE_INT:
			sprintf(txBuf, "get %s.val", pOb_handle->Name);
			break;

		case OBJ_TYPE_TXT: //TODO: not implemented yet, change "ref" to "get"
			sprintf(txBuf, "ref %s.txt", pOb_handle->Name);
			break;
		default:
			break;
	} //end switch
	HmiSendCommand( txBuf);

	FNC_Ret_Status_t retValue = waitForAnswer(&retNumber);//TODO: ?
	if( retValue == STAT_FAILED){
		return retValue;
	}
	*pValue = retNumber.numData;
	return retValue;
}

/**
 * @brief Perform a soft reset
 * @note  Reboot the display. When it is ready, returns: 00 00 00 FF FF FF, 88 FF FF FF
 * 			takes approximately 250ms
 *
 * @param void
 * @retval see @ref waitForAnswer() function for return value
 */
FNC_Ret_Status_t NxHmi_ResetDevice(void) {

	prepareToSend();
	//HmiSendCommand("");
	//xSemaphoreTake(nextionHMI_h.hmiUartTxSem, portMAX_DELAY);

	Ret_Command_t retNumber;
	sprintf(txBuf, "rest");
	HmiSendCommand(txBuf);

	if(waitForAnswer(&retNumber) == STAT_FAILED){
		return STAT_ERROR; //TODO: the interface will stuck in BUSY mode
	}
	if(retNumber.cmdCode == NEX_EVENT_INIT_OK) {
		nextionHMI_h.hmiStatus = COMP_IDLE;
	}

	return (int8_t)retNumber.numData;
}

/**
 * @brief Get the current active page on the display
 * @note  --
 *
 * @param pValue - Pointer for the returned 8bit number (actual page ID)
 * @retval see @ref waitForAnswer() function for return value
 */
FNC_Ret_Status_t NxHmi_GetCurrentPageId(uint8_t *pValue) {

	prepareToSend();
	Ret_Command_t tmpCommand;
	FNC_Ret_Status_t tmpRet;

	sprintf(txBuf, "sendme");
	HmiSendCommand(txBuf);
	tmpRet = waitForAnswer(&tmpCommand);
	if(tmpRet == STAT_OK){
		*pValue = tmpCommand.pageId;
	}
	return tmpRet;
}
