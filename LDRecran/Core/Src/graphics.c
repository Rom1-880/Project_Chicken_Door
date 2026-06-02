/*
 * graphics.c
 *
 * Created on: Mar 20, 2012
 * Author: RobG
 *
 * Modified on: April 20, 2026
 * Author: Tarek ALLAM
 * for STM32 Portage
 */

#include "graphics.h"
#include "fonts.h"
#include "bibliotheque.h" // Inclut config.h et lcd.h

// --- ANCIEN ---
/*
extern void setArea(u_char xStart, u_char yStart, u_char xEnd, u_char yEnd);
extern void writeData(u_char data);
extern u_char getScreenWidth();
extern u_char getScreenHeight();

u_char colorLowByte = 0;
u_char colorHighByte = 0;
u_char bgColorLowByte = 0;
u_char bgColorHighByte = 0;
*/

// --- NOUVEAU ---
// Plus besoin de "extern" si on inclut correctement lcd.h et config.h dans bibliotheque.h
uint8_t colorLowByte = 0;
uint8_t colorHighByte = 0;
uint8_t bgColorLowByte = 0;
uint8_t bgColorHighByte = 0;

//////////////////////
// color
//////////////////////

// --- ANCIEN ---
/*
void setColor(uint16_t color) { ... }
void setBackgroundColor(uint16_t color) { ... }
*/

// --- NOUVEAU ---
void setColor(uint16_t color) {
	colorLowByte = (uint8_t)color;
	colorHighByte = (uint8_t)(color >> 8);
}

void setBackgroundColor(uint16_t color) {
	bgColorLowByte = (uint8_t)color;
	bgColorHighByte = (uint8_t)(color >> 8);
}

/////////////////
// drawing
/////////////////

void clearScreen(uint8_t blackWhite) {
	uint8_t w = getScreenWidth();
	uint8_t h = getScreenHeight();
	setArea(0, 0, w - 1, h - 1);
	setBackgroundColor(blackWhite ? 0x0000 : 0xFFFF);

	while (h != 0) {
		while (w != 0) {
			writeData(bgColorHighByte);
			writeData(bgColorLowByte);
			w--;
		}
		w = getScreenWidth(); // Recharge la largeur pour la ligne suivante
		h--;
	}
}

void drawPixel(uint8_t x, uint8_t y) {
	setArea(x, y, x, y);
	writeData(colorHighByte);
	writeData(colorLowByte);
}

/////////////////////////////
// Draw String - type: 0=Sm, 1=Md, 2=Lg, 3=Sm/Bkg, 4=Md/Bkg, 5=Lg/Bkg
/////////////////////////////
void drawString(uint8_t x, uint8_t y, char type, char *string) {
	uint8_t xs = x;
	switch (type) {
	case FONT_SM:
		while (*string) {
			drawCharSm(xs, y, *string++);
			xs += 6;
		}
		break;
	case FONT_MD:
		while (*string) {
			drawCharMd(xs, y, *string++);
			xs += 8;
		}
		break;
	case FONT_LG:
		while (*string) {
			drawCharLg(xs, y, *string++);
			xs += 12;
		}
		break;
	case FONT_SM_BKG:
		while (*string) {
			drawCharSmBkg(xs, y, *string++);
			xs += 6;
		}
		break;
	case FONT_MD_BKG:
		while (*string) {
			drawCharMdBkg(xs, y, *string++);
			xs += 8;
		}
		break;
	case FONT_LG_BKG:
		while (*string) {
			drawCharLgBkg(xs, y, *string++);
			xs += 12;
		}
		break;
	}
}

//////////////////////////
// Fonts (Mise à jour des types uint8_t et uint16_t)
//////////////////////////

void drawCharSm(uint8_t x, uint8_t y, char c) {
	uint8_t col = 0, row = 0, bit = 0x01;
	uint8_t oc = c - 0x20;
	while (row < 8) {
		while (col < 5) {
			if (font_5x7[oc][col] & bit) drawPixel(x + col, y + row);
			col++;
		}
		col = 0; bit <<= 1; row++;
	}
}

void drawCharSmBkg(uint8_t x, uint8_t y, char c) {
	uint8_t col = 0, row = 0, bit = 0x01;
	uint8_t oc = c - 0x20;
	setArea(x, y, x + 4, y + 7);
	while (row < 8) {
		while (col < 5) {
			if (font_5x7[oc][col] & bit) {
				writeData(colorHighByte); writeData(colorLowByte);
			} else {
				writeData(bgColorHighByte); writeData(bgColorLowByte);
			}
			col++;
		}
		col = 0; bit <<= 1; row++;
	}
}

void drawCharMd(uint8_t x, uint8_t y, char c) {
	uint8_t col = 0, row = 0, bit = 0x80;
	uint8_t oc = c - 0x20;
	while (row < 12) {
		while (col < 8) {
			if (font_8x12[oc][row] & bit) drawPixel(x + col, y + row);
			bit >>= 1; col++;
		}
		bit = 0x80; col = 0; row++;
	}
}

void drawCharMdBkg(uint8_t x, uint8_t y, char c) {
	uint8_t col = 0, row = 0, bit = 0x80;
	uint8_t oc = c - 0x20;
	setArea(x, y, x + 7, y + 11);
	while (row < 12) {
		while (col < 8) {
			if (font_8x12[oc][row] & bit) {
				writeData(colorHighByte); writeData(colorLowByte);
			} else {
				writeData(bgColorHighByte); writeData(bgColorLowByte);
			}
			bit >>= 1; col++;
		}
		bit = 0x80; col = 0; row++;
	}
}

void drawCharLg(uint8_t x, uint8_t y, char c) {
	uint8_t col = 0, row = 0;
	uint16_t bit = 0x0001;
	uint8_t oc = c - 0x20;
	while (row < 16) {
		while (col < 11) {
			if (font_11x16[oc][col] & bit) drawPixel(x + col, y + row);
			col++;
		}
		col = 0; bit <<= 1; row++;
	}
}

void drawCharLgBkg(uint8_t x, uint8_t y, char c) {
	uint8_t col = 0, row = 0;
	uint16_t bit = 0x0001;
	uint8_t oc = c - 0x20;
	setArea(x, y, x + 10, y + 15);
	while (row < 16) {
		while (col < 11) {
			if (font_11x16[oc][col] & bit) {
				writeData(colorHighByte); writeData(colorLowByte);
			} else {
				writeData(bgColorHighByte); writeData(bgColorLowByte);
			}
			col++;
		}
		col = 0; bit <<= 1; row++;
	}
}

////////////////////////
// Shapes & Lines
////////////////////////

void drawLine(uint8_t xStart, uint8_t yStart, uint8_t xEnd, uint8_t yEnd) {
	uint8_t x0, x1, y0, y1, d = 0;
	if (yStart > yEnd) { y0 = yEnd; y1 = yStart; } else { y1 = yEnd; y0 = yStart; }
	if (xStart > xEnd) { x0 = xEnd; x1 = xStart; } else { x1 = xEnd; x0 = xStart; }

	if (y0 == y1) {
		d = x1 - x0 + 1; setArea(x0, y0, x1, y1);
		while (d-- > 0) { writeData(colorHighByte); writeData(colorLowByte); }
	} else if (x0 == x1) {
		d = y1 - y0 + 1; setArea(x0, y0, x1, y1);
		while (d-- > 0) { writeData(colorHighByte); writeData(colorLowByte); }
	} else {
		char dx, dy; int sx, sy;
		if (xStart < xEnd) { sx = 1; dx = xEnd - xStart; } else { sx = -1; dx = xStart - xEnd; }
		if (yStart < yEnd) { sy = 1; dy = yEnd - yStart; } else { sy = -1; dy = yStart - yEnd; }
		int e1 = dx - dy, e2;
		while (1) {
			drawPixel(xStart, yStart);
			if (xStart == xEnd && yStart == yEnd) break;
			e2 = 2 * e1;
			if (e2 > -dy) { e1 = e1 - dy; xStart = xStart + sx; }
			if (e2 < dx) { e1 = e1 + dx; yStart = yStart + sy; }
		}
	}
}

void drawRect(uint8_t xStart, uint8_t yStart, uint8_t xEnd, uint8_t yEnd) {
	drawLine(xStart, yStart, xEnd, yStart);
	drawLine(xStart, yEnd, xEnd, yEnd);
	drawLine(xStart, yStart, xStart, yEnd);
	drawLine(xEnd, yStart, xEnd, yEnd);
}
////////////////////////
// images
////////////////////////

// --------Ancien-------
// void drawImage(u_char x, u_char y, u_char w, u_char h, uint16_t * data) { }
// void drawImageLut(u_char x, u_char y, u_char w, u_char h, u_char * data, uint16_t * lut) { }
// void drawImageMono(u_char x, u_char y, u_char w, u_char h, u_char * data) { }
// --------Nouveau------

void drawImage(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint16_t * data) {
    // A remplir plus tard si besoin
}

void drawImageLut(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t * data, uint16_t * lut) {
    // A remplir plus tard si besoin
}

void drawImageMono(uint8_t x, uint8_t y, uint8_t w, uint8_t h, uint8_t * data) {
    // A remplir plus tard si besoin
}

////////////////////////
// Additional Shapes (Circles & Logic Line)
////////////////////////

// --------Ancien-------
// void drawLogicLine(u_char x, u_char y, u_char length, u_char height, u_char * data)
// --------Nouveau------
void drawLogicLine(uint8_t x, uint8_t y, uint8_t length, uint8_t height, uint8_t * data) {
	uint8_t last = (*data & 0x80);
	uint8_t counter = 0;
	while (counter < length) {
		uint8_t bitCounter = 0;
		uint8_t byte = *data;
		while (bitCounter < 8 && counter < length) {
			if (last == (byte & 0x80)) {
				uint8_t h = (byte & 0x80) ? (height + y) : y;
				drawPixel(x + counter, h);
			} else {
				if (byte & 0x80) {
					drawLine(x + counter - 1, y, x + counter, y + height);
				} else {
					drawLine(x + counter - 1, y + height, x + counter, y);
				}
			}
			last = byte & 0x80;
			byte <<= 1;
			bitCounter++;
			counter++;
		}
		data++; // Corrigé : *data++ en data++
	}
}

// --------Ancien-------
// void drawCircle(u_char x, u_char y, u_char radius)
// --------Nouveau------
void drawCircle(uint8_t x, uint8_t y, uint8_t radius) {
	int dx = radius;
	int dy = 0;
	int xChange = 1 - 2 * radius;
	int yChange = 1;
	int radiusError = 0;
	while (dx >= dy) {
		drawPixel(x + dx, y + dy);
		drawPixel(x - dx, y + dy);
		drawPixel(x - dx, y - dy);
		drawPixel(x + dx, y - dy);
		drawPixel(x + dy, y + dx);
		drawPixel(x - dy, y + dx);
		drawPixel(x - dy, y - dx);
		drawPixel(x + dy, y - dx);
		dy++;
		radiusError += yChange;
		yChange += 2;
		if (2 * radiusError + xChange > 0) {
			dx--;
			radiusError += xChange;
			xChange += 2;
		}
	}
}

// --------Ancien-------
// void fillCircle(u_char x, u_char y, u_char radius)
// --------Nouveau------
void fillCircle(uint8_t x, uint8_t y, uint8_t radius) {
	int dx = radius;
	int dy = 0;
	int xChange = 1 - 2 * radius;
	int yChange = 1;
	int radiusError = 0;
	while (dx >= dy) {
		drawLine(x + dy, y + dx, x - dy, y + dx);
		drawLine(x - dy, y - dx, x + dy, y - dx);
		drawLine(x - dx, y + dy, x + dx, y + dy);
		drawLine(x - dx, y - dy, x + dx, y - dy);
		dy++;
		radiusError += yChange;
		yChange += 2;
		if (2 * radiusError + xChange > 0) {
			dx--;
			radiusError += xChange;
			xChange += 2;
		}
	}
}
void fillRect(uint8_t xStart, uint8_t yStart, uint8_t xEnd, uint8_t yEnd) {
	setArea(xStart, yStart, xEnd, yEnd);
	uint16_t total = (xEnd - xStart + 1) * (yEnd - yStart + 1);
	uint16_t c = 0;
	while (c < total) {
		writeData(colorHighByte);
		writeData(colorLowByte);
		c++;
	}
}

