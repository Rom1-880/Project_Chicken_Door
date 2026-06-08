/*
 * ecran.h
 *
 *  Created on: Jun 5, 2026
 *      Author: tallam
 */

#ifndef INC_ECRAN_H_
#define INC_ECRAN_H_

#include "stm32u0xx_hal.h"
#include <stdbool.h>
#include <string.h>

//Prototypes des fonctions UART Bare-Metal
void USART2_SendChar(char c);
void USART2_SendString(const char *str);

// Prototype de la fonction de décodage des commandes
void lire_commande();

// Prototype de la fonction de mise en veille
void mise_veille_aff();

void Affichage_Init_Demarrage(void);

void GESTION_Systeme(void);

//Ancient design
//void rafraichir_ephemeride(void);

#endif /* INC_ECRAN_H_ */
