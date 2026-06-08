/*
 * serial.h
 * Created on: 6 mars 2019
 * Author: Joel
 *
 * Modified on: 19 Avril 2026
 * Author: ALLAM Tarek
 * Modified for STM32
 */

#ifndef SERIAL_H_
#define SERIAL_H_

#include <stdint.h>
#include <stdio.h>
#include "main.h" // NOUVEAU : Pour inclure la couche HAL UART

// --- ANCIEN (MSP430 - Obsolète ou géré différemment) ---
/*
int getchar(void);
int putchar(int c);
void configureUART(void); // Fait par CubeMX maintenant
*/

// --- NOUVEAU / CONSERVÉ (Enveloppes pour le STM32) ---
// Ces fonctions seront recréées dans serial.c en utilisant HAL_UART_Transmit
void print(const char *s);
void printx(const uint8_t c);
void printnum99(const uint8_t c);

#endif /* SERIAL_H_ */
