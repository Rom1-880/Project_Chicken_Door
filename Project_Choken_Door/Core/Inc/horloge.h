/*
 * horloge.h
 * Created on: 2 juin 2019
 * Author: Joel
 *
 * Modified on: 19 Avril 2026
 * Author: ALLAM Tarek
 * Modified for STM32
 */

#ifndef HORLOGE_H_
#define HORLOGE_H_

// --- ANCIEN (MSP430) ---
/*
void init_horloge(); // Obsolète : géré par SystemClock_Config() du STM32
*/

// --- NOUVEAU ---
// Fichier gardé vide pour maintenir la compatibilité des anciens #include
// La gestion du temps (10s) se fera via HAL_GetTick() ou un Timer matériel.

#endif /* HORLOGE_H_ */
