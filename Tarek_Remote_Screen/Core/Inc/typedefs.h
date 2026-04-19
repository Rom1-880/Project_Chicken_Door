/*
 * typedefs.h
 *
 *  Created on: Nov 2, 2012
 *      Author: RobG
 *
 *  Modified on: 19 Avril 2026
 * 		Author: ALLAM Tarek
 * 		for STM32 Portage
 */

#ifndef TYPEDEFS_H_
#define TYPEDEFS_H_

#include <stdint.h> // Nouveau : Ajout pour utiliser les types standards sécurisés

#ifndef U_TYPES
#define U_TYPES

// --- ANCIEN (Incertain sur STM32) ---
/*
typedef unsigned char u_char;
typedef unsigned int u_int;
*/

// --- NOUVEAU (Sécurisé pour STM32) ---
typedef uint8_t  u_char; // Force 8 bits (0-255)
typedef uint16_t u_int;  // Force 16 bits (0-65535) -> Vital pour les couleurs !

#endif

#endif /* TYPEDEFS_H_ */
