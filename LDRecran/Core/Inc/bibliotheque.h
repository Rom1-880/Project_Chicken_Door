/*
 * bibliotheque.h
 *
 * Created on: 17 mai 2019
 * Author: Joel
 *
 * Modified on: 19 Avril 2026
 * Author: ALLAM Tarek
 * for STM32 Portage
 */

#ifndef BIBLIOTHEQUE_H_
#define BIBLIOTHEQUE_H_

// --- NOUVEAU (Portage STM32) ---
#include "main.h"    // Remplace msp430g2955.h, contient tout le HAL du STM32
#include <stdint.h>  // Pour les types uint8_t, uint16_t, etc.
#include <time.h>

// --- ANCIEN (Commenté car incompatible avec STM32) ---
/*
#include <msp430g2955.h>
#include "msp430.h"
#include <msp.h>
*/

// --- INCLUSIONS DU PROJET (Conservées) ---
#include "typedefs.h"
#include "lcd.h"
#include "config.h"
//#include "horloge.h"  // A décommenter si on a ce fichier
#include "graphics.h"
//#include "PWM.h"
//#include "mesure_analogique.h"
//#include "menu.h"
#include "affichagemenu.h"
#include "serial.h"
//#include "ephemeride.h"
//#include "flash.h"

#endif /* BIBLIOTHEQUE_H_ */
