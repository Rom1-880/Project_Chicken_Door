/*
 * graphics.h
 *
 *  Created on: Mar 19, 2012
 *      Author: RobG
 */
/*
 * Modified on:April 19,2026
 * 		Author:Tarek ALLAM
 */
#ifndef GRAPHICS_H_
#define GRAPHICS_H_
#include "typedefs.h"
//--------Nouveau------
#include <stdint.h>
// font sizes
#define FONT_SM		0
#define FONT_MD		1
#define FONT_LG		2
#define FONT_SM_BKG	3
#define FONT_MD_BKG	4
#define FONT_LG_BKG	5
//--------Ancien-------
/*void setColor(uint16_t color);
void setBackgroundColor(uint16_t color);*/
//--------Nouveau------
void setColor(uint16_t color);
void setBackgroundColor(uint16_t color);
//
void clearScreen(u_char blackWhite);
void drawPixel(u_char x, u_char y);
void drawString(u_char x, u_char y, char type, char *string);
void drawCharLg(u_char x, u_char y, char c);
void drawCharMd(u_char x, u_char y, char c);
void drawCharSm(u_char x, u_char y, char c);
void drawCharLgBkg(u_char x, u_char y, char c);
void drawCharMdBkg(u_char x, u_char y, char c);
void drawCharSmBkg(u_char x, u_char y, char c);
void drawLine(u_char xStart, u_char yStart, u_char xEnd, u_char yEnd);
void drawRect(u_char xStart, u_char yStart, u_char xEnd, u_char yEnd);
void drawCircle(u_char x, u_char y, u_char radius);
//
void fillRect(u_char xStart, u_char yStart, u_char xEnd, u_char yEnd);
void fillCircle(u_char x, u_char y, u_char radius);
//
void drawLogicLine(u_char x, u_char y, u_char length, u_char height, u_char * data);
//
void drawImage(u_char x, u_char y, u_char w, u_char h, uint16_t * data);
void drawImageLut(u_char x, u_char y, u_char w, u_char h, u_char * data, uint16_t * lut);
void drawImageMono(u_char x, u_char y, u_char w, u_char h, u_char * data);

#endif /* GRAPHICS_H_ */

