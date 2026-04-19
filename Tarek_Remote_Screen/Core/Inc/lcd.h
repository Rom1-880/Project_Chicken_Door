/*
 * lcd.h
 *
 *  Created on: Jul 12, 2013
 *      Author: RobG
 */
/*
 * Modified on:April 19,2026
 * 		Author:Tarek ALLAM
 */
#ifndef LCD_H_
#define LCD_H_
/*--------Ancien-------
#include "typedefs.h"
#include "config.h"*/
//--------Nouveau------
#include "main.h" // Indispensable pour connaître les broches CS, etc.
#include <stdint.h>
void initialise_LCD(void);

void initLCD();
void setArea(u_char xStart, u_char yStart, u_char xEnd, u_char yEnd);

u_char getScreenWidth();
u_char getScreenHeight();
void setOrientation(u_char orientation);

#ifdef ST7735
//

/*
#define LONG_EDGE_PIXELS 160
#define SHORT_EDGE_PIXELS 128
 */

// Augmentation de la taille de l'�cran, sinon, on n'arrive pas � effacer le bord droit!
#define LONG_EDGE_PIXELS 160
#define SHORT_EDGE_PIXELS 130

#define LCD_OFFSET_HEIGHT 0
#define LCD_OFFSET_WIDTH 0
//
#elif defined HX8340
//
#define SPI9BIT
#define LONG_EDGE_PIXELS 220
#define SHORT_EDGE_PIXELS 176
#define LCD_OFFSET_HEIGHT 0
#define LCD_OFFSET_WIDTH 0
//
#elif defined ILI9225
//
#define LONG_EDGE_PIXELS 220
#define SHORT_EDGE_PIXELS 176
#define LCD_OFFSET_HEIGHT 0
#define LCD_OFFSET_WIDTH 0
//
#endif

// orientation
#define ORIENTATION_VERTICAL 0
#define ORIENTATION_HORIZONTAL 1
#define ORIENTATION_VERTICAL_ROTATED 2
#define ORIENTATION_HORIZONTAL_ROTATED 3

// driver specific
#define SWRESET		0x01
#define	BSTRON		0x03
#define RDDIDIF		0x04
#define RDDST		0x09
#define SLEEPIN     0x10
#define	SLEEPOUT	0x11
#define	NORON		0x13
#define	INVOFF		0x20
#define INVON      	0x21
#define	SETCON		0x25
#define DISPOFF     0x28
#define DISPON      0x29
#define CASETP      0x2A
#define PASETP      0x2B
#define RAMWRP      0x2C
#define RGBSET	    0x2D
#define	MADCTL		0x36
#define SEP			0x37
#define	COLMOD		0x3A
#define DISCTR      0xB9
#define DOR			0xBA
#define	EC			0xC0
#define RDID1		0xDA
#define RDID2		0xDB
#define RDID3		0xDC
#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

#define SETOSC		0xB0
#define SETPWCTR4	0xB4
#define SETPWCTR5	0xB5
#define SETEXTCMD	0xC1
#define SETGAMMAP	0xC2
#define SETGAMMAN	0xC3
// ILI9225B specific
#define ILIGRAMADDRX	0x20
#define ILIGRAMADDRY 	0x21
#define ILIWRDATA   	0x22
#define ILIGRAMHEA		0x36
#define ILIGRAMHSA		0x37
#define ILIGRAMVEA		0x38
#define ILIGRAMVSA		0x39

#endif /* LCD_H_ */
