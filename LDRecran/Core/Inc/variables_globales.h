/*
 * variables_globales.h
 * Fichier centralisant les variables partagées entre les modules
 *
 *  Created on: Apr 22, 2026
 *      Author: ELITEBOOK
 *      for STM32 Portage   test
 *
 *  MODIFIED ON JUNE 2 2026
 *  Author COVES Clément
 *  for adaptation for my code for % Battery
 */

#ifndef INC_VARIABLES_GLOBALES_H_
#define INC_VARIABLES_GLOBALES_H_

#include <time.h> // Nécessaire pour time_t et struct tm
#include <stdint.h>

// ---- Variables de temps (RTC) ----
extern time_t temps;
extern struct tm tm_temps;
extern int8_t UTC;
extern int8_t absUTC;

// ---- Variables Système / Matériel ----
//extern uint32_t tensionpile;
extern uint32_t bat_pourcentage;
extern uint8_t langue;              // 1:EN, 2:FR, 3:DE, 4:ES, 5:IT

// ---- Variables d'états et menus ----
extern uint8_t modeO;               // Ouverture - 1: fixe, 2: capteur, 3: GPS
extern uint8_t modeF;               // Fermeture - 1: fixe, 2: capteur, 3: GPS
extern int8_t heureO;
extern int8_t minO;
extern int8_t heureF;
extern int8_t minF;
extern int8_t minretard;    // Retard de fermeture (0 à 90 min)
extern int8_t minretardchange;
// ---- Variables GPS ----
extern int8_t latitude;
extern int16_t longitude;
extern uint8_t choixi;              // Choix réglage lat/long

// ---- Variables de Navigation (Ajoutées car utilisées dans menu.c) ----
extern uint8_t ON;
extern uint8_t DWN;
extern uint8_t HUP;
extern uint8_t RTN;
extern uint8_t ecran;

#endif /* INC_VARIABLES_GLOBALES_H_ */
