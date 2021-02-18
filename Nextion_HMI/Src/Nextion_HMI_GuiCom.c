/*
 * Nextion_HMI_GuiCom.c
 *
 *  Created on: Feb 15, 2021
 *      Author: György Kovács
 *
 *      GUI Commands
 */

#include "Nextion_HMI.h"

/**
 * @brief Set Nextion object background color
 * @note  16bit color code ( https://nextion.tech/instruction-set/#s5 )
 *
 * @param *pOb_handle = Nextion object handler
 * @param colour = 16bit RGB color code, R-5bit G-6bit, B-5bit
 * @retval see @ref waitForAnswer() function for return value
 */
FNC_Ret_Status_t NxHmi_SetBcoColour(Nextion_Object_t *pOb_handle, uint16_t color) {

	prepareToSend();
	sprintf(txBuf, "%s.bco=%u", pOb_handle->Name, color);
	HmiSendCommand(txBuf);

	return waitForAnswer(NULL);
}

/**
 * @brief Set Nextion object background color
 * @note  16bit color code ( https://nextion.tech/instruction-set/#s5 )
 *
 * @param *pOb_handle = Nextion object handler
 * @param red = 5 bit, MAX value 31
 * @param green = 6 bit, MAX value 63
 * @param blue = 5 bit, MAX value 31
 * @retval see @ref waitForAnswer() function for return value
 */
FNC_Ret_Status_t NxHmi_SetBcoColourRGB(Nextion_Object_t *pOb_handle, uint8_t red, uint8_t green, uint8_t blue){

	prepareToSend();
	uint16_t color = (red & 0x1F);
	color = color << 6;
	color += (green & 0x3F);
	color = color << 5;
	color += (blue & 0x1F);
	sprintf(txBuf, "%s.bco=%u", pOb_handle->Name, color);
	HmiSendCommand(txBuf);

	return waitForAnswer(NULL);
}
