/*
 * affichagemenu.h
 *
 *  Created on: 8 mai 2019
 *      Author: Joel
 *
 *       * Modified on: 19 Avril 2026
 *		 Author: ALLAM Tarek
 * 		for STM32 Portage
 */

#ifndef AFFICHAGEMENU_H_
#define AFFICHAGEMENU_H_

void affichecarre(char choix);      // encadrement rouge

void effaceligne(void);              // efface la ligne correspondant � l'ann�e.

void effaceligne2(void);            // efface la ligne correspondant � la latitude

void effaceligne3(void);            // efface la ligne correspondant � la longitude

void afficheecran1(void);   // langue

void afficheecran2(void);   // r�glages

void afficheecran3(void);   // r�glage ann�e

void afficheecran4(void);   // r�glage mois

void afficheecran5(void);   // r�glage jour

void afficheecran6(void);   // r�glage heure

void afficheecran7(void);   // r�glage min

void afficheecran8(void);   // Ecran principal (apr�s un appui de 5s sur le BP ON)

void afficheecran9(void);   // r�glage GPS

void afficheecran10(void);  // choix mode ouverture

void afficheecran11(void);  // Reglage heure mode ouverture fixe

void afficheecran12(void);  // Reglage min mode ouverture fixe

void afficheecran13(void);  // choix sensibilit� capteur J/N Ouverture

void afficheecran20(void);  // choix mode fermeture

void afficheecran21(void);  // Reglage heure mode fermeture fixe

void afficheecran22(void);  // Reglage min mode fermeture fixe

void afficheecran23(void);  // choix sensibilit� capteur J/N Fermeture

void afficheecran24(void);  // r�glage delai fermeture



#endif /* AFFICHAGEMENU_H_ */
