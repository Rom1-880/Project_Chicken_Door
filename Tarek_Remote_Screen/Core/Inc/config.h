/*
 * config.h
 *
 *  Created on: Jul 12, 2013
 *      Author: RobG
 *  Modified on: 19 Avril 2026
 * 		Author: ALLAM Tarek
 * 		for STM32 Portage
 */

#ifndef CONFIG_H_
#define CONFIG_H_

// --- NOUVEAU (Configuration de l'écran conservée) ---
#define ST7735
#define ORIENTATION 1 // 0=Portrait, 1=Paysage (Horizontal)

// --- ANCIEN (Spécifique au MSP430 - Désactivé) ---
/*
#define HARDWARE_SPI
#define G2955_ROBG

#define LCD_SCLK_PIN BIT3
#define LCD_SCLK_PORT P3
#define LCD_MOSI_PIN BIT1
#define LCD_MOSI_PORT P3
#define LCD_CS_PIN BIT0
#define LCD_CS_PORT P2
#define LCD_DC_PIN BIT0
#define LCD_DC_PORT P3
*/

#endif /* CONFIG_H_ */
