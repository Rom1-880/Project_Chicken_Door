/*
 * variables_globales.h
 * Fichier centralisant les variables partagées entre les modules
 *
 *  Created on: Apr 22, 2026
 *      Author: ELITEBOOK
 *      for STM32 Portage
 */

#ifndef INC_VARIABLES_GLOBALES_H_
#define INC_VARIABLES_GLOBALES_H_

#include <time.h> // Nécessaire pour time_t et struct tm

// ---- Variables de temps (RTC) ----
extern time_t temps;
extern struct tm tm_temps;
extern signed char UTC;
extern signed char absUTC;

// ---- Variables Système / Matériel ----
extern long tensionpile;
extern char langue;              // 1:EN, 2:FR, 3:DE, 4:ES, 5:IT

// ---- Variables d'états et menus ----
extern char modeO;               // Ouverture - 1: fixe, 2: capteur, 3: GPS
extern char modeF;               // Fermeture - 1: fixe, 2: capteur, 3: GPS
extern signed char heureO;
extern signed char minO;
extern signed char heureF;
extern signed char minF;
extern char signed minretard;    // Retard de fermeture (0 à 90 min)

// ---- Variables GPS ----
extern signed char latitude;
extern int longitude;
extern char choixi;              // Choix réglage lat/long

#endif /* INC_VARIABLES_GLOBALES_H_ */
