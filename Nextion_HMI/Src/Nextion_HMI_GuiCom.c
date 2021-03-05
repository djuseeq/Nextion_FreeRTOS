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
 * @param color = 16bit RGB color code, R-5bit G-6bit, B-5bit
 * @retval see @ref waitForAnswer() function for return value
 */
Ret_Status_t NxHmi_SetBcoColour(Nextion_Object_t *pOb_handle, uint16_t color) {

	prepareToSend(0);
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
Ret_Status_t NxHmi_SetBcoColourRGB(Nextion_Object_t *pOb_handle, uint8_t red, uint8_t green, uint8_t blue){

	prepareToSend(0);
	uint16_t color = (red & 0x1F);
	color = color << 6;
	color += (green & 0x3F);
	color = color << 5;
	color += (blue & 0x1F);
	sprintf(txBuf, "%s.bco=%u", pOb_handle->Name, color);
	HmiSendCommand(txBuf);

	return waitForAnswer(NULL);
}

/**
 * @brief Display a resource image at specified location
 * @note  The XY coordinates are the upper left corner of the image that is drawn on the display
 *		  The origin(0,0) is the display's upper left corner
 * @param picId = Image ID from the resource
 * @param xAxis = X axis on the display
 * @param yAxis = Y axis on the display
 * @retval see @ref waitForAnswer() function for return value
 */
Ret_Status_t NxHmi_DrawImage(uint8_t picId, uint16_t xAxis, uint16_t yAxis) {

	prepareToSend(0);
	sprintf(txBuf, "pic %u,%u,%i", xAxis, yAxis, picId);
	HmiSendCommand(txBuf);

	return waitForAnswer(NULL);
}

/**
 * @brief Display and crop a resource image at specified location
 * @note  The XY coordinates are the upper left corner of the image that is drawn on the display
 *		  The origin(0,0) is the display's upper left corner
 * @param picId = Image ID from the resource
 * @param xPane = X axis on the display
 * @param yPane = Y axis on the display
 * @param width = X axis on the display
 * @param height = Y axis on the display
 * @param xImg = X axis on the image
 * @param yImg = Y axis on the image
 * @retval see @ref waitForAnswer() function for return value
 */
Ret_Status_t NxHmi_DrawCropImage(uint8_t picId, uint16_t xPane, uint16_t yPane,
					uint16_t width, uint16_t height, uint16_t xImg, uint16_t yImg)
{

	prepareToSend(0);
	sprintf(txBuf, "xpic %u,%u,%u,%u,%u,%u,%i", xPane, yPane, width, height, xImg, yImg, picId);
	HmiSendCommand(txBuf);

	return waitForAnswer(NULL);
}

/**
 * @brief Draw a line on the display
 * @note  Give the start XY and the end XY coordinates
 *
 * @param startX = Line start point on the X axis
 * @param startY = Line start point on the Y axis
 * @param endX   = Line end point on the X axis
 * @param endY   = Line end point on the Y axis
 * @param color  = The color of the line
 * @retval see @ref waitForAnswer() function for return value
 */
Ret_Status_t NxHmi_DrawLine(uint16_t startX, uint16_t startY, uint16_t endX, uint16_t endY, uint16_t color) {

	prepareToSend(0);
	sprintf(txBuf, "line %u,%u,%u,%u,%u", startX, startY, endX, endY, color);
	HmiSendCommand(txBuf);

	return waitForAnswer(NULL);
}

/**
 * @brief Draw a rectangle on the display
 * @note  Give the start XY and the end XY coordinates
 *
 * @param startX = Rectangle start point on the X axis
 * @param startY = Rectangle start point on the Y axis
 * @param endX   = Rectangle end point on the X axis
 * @param endY   = Rectangle end point on the Y axis
 * @param color  = The color of the rectangle
 * @param fMode  = Rectangle draw mode, 1 - filled, 0 - hollow
 * @retval see @ref waitForAnswer() function for return value
 */
Ret_Status_t NxHmi_DrawRect(uint16_t startX, uint16_t startY, uint16_t endX,
								uint16_t endY, uint16_t color, uint8_t fMode)
{

	prepareToSend(0);
	if(fMode) {
		sprintf(txBuf, "fill %u,%u,%u,%u,%u", startX, startY, endX, endY, color);
	} else {
		sprintf(txBuf, "draw %u,%u,%u,%u,%u", startX, startY, endX, endY, color);
	}

	HmiSendCommand(txBuf);

	return waitForAnswer(NULL);
}

/**
 * @brief Draw a circle on the display
 * @note  Give the center XY coordinates end the radius
 *
 * @param centX  = Circle center point on the X axis
 * @param centY  = Circle center point on the Y axis
 * @param radius = Radius of the circle
 * @param color  = The color of the circle
 * @param fMode  = Circle draw mode, 1 - filled, 0 - hollow
 * @retval see @ref waitForAnswer() function for return value
 */
Ret_Status_t NxHmi_DrawCircle(uint16_t centX, uint16_t centY, uint16_t radius, uint16_t color, uint8_t fMode) {

	prepareToSend(0);
	if(fMode) {
		sprintf(txBuf, "cirs %u,%u,%u,%u", centX, centY, radius, color);
	} else {
		sprintf(txBuf, "cir %u,%u,%u,%u", centX, centY, radius, color);
	}
	HmiSendCommand(txBuf);

	return waitForAnswer(NULL);
}



