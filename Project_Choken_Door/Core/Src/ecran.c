/*
 * ecran.c
 *
 *  Created on: Jun 5, 2026
 *      Author: tarek allam
 */
#include <time.h>  // OBLIGATOIRE : pour la structure tm (tm_temps)
#include <stdio.h>
#include <stdbool.h>
#include "ecran.h"
#include "main.h"
#include "graphics.h"
#include <string.h>

// Déclaration des variables globales partagées
// 'extern' indique au compilateur que ces variables existent déjà ailleurs (dans main.c)
extern bool Drapeau_commande_recue;
extern char rx_buffer[10];
extern uint8_t index_buffer;
extern uint8_t touche_ON, touche_DWN, touche_HUP, touche_RTN;
extern UART_HandleTypeDef huart2; // Permet d'utiliser le huart2 du main.c
extern uint8_t rx_char;
extern SPI_HandleTypeDef hspi1;
extern RTC_TimeTypeDef sTime;
extern RTC_HandleTypeDef hrtc;     // Permet d'accéder au composant RTC du main.c
extern RTC_DateTypeDef sDate;      // Permet de lire la date
extern struct tm tm_temps;         // Structure de temps universelle C

// Variables d'état
extern int calcul_fait_aujourdhui;
extern unsigned char heureGPSO, minGPSO, heureGPSF, minGPSF;
extern uint32_t dernier_temps_actif;
extern bool ecran_allume;

// Prototypes des fonctions externes
extern void MX_GPIO_Init(void);
extern void MX_SPI1_Init(void);
extern void rafraichir_ephemeride(void);
extern void initialise_LCD(void);
extern void setOrientation(int orientation);
extern void writeCommand(uint8_t cmd);
extern void menu(void);
extern void calculerEphemeride(void);

// =========================================================================
// FONCTIONS BARE-METAL POUR LA SÉRIE (UART2)
// =========================================================================

void UART2_SendChar(char c) {
    // On attend que le registre de transmission soit vide (TXE = 1)
	while (!(USART2->ISR & USART_ISR_TXE_TXFNF));

    // On écrit le caractère directement dans le registre de données
    USART2->TDR = c;
}

// Fonction Bare-Metal pour envoyer une chaîne complète (string)
void UART2_SendString(const char *str) {
    while (*str) {
        UART2_SendChar(*str++);
    }
}

void lire_commande(void) {
	if (Drapeau_commande_recue == true) {
		if (strcmp(rx_buffer, "ON") == 0) {
			touche_ON = 1;
			UART2_SendString("--> Action: ON\r\n");
		}
		else if (strcmp(rx_buffer, "DWN") == 0) {
			touche_DWN = 1;
			UART2_SendString("--> Action: DWN\r\n");
		}
		else if (strcmp(rx_buffer, "HUP") == 0) {
			touche_HUP = 1;
			UART2_SendString("--> Action: HUP\r\n");
		}
		else if (strcmp(rx_buffer, "RTN") == 0) {
			touche_RTN = 1;
			UART2_SendString("--> Action: RTN\r\n");
		}

		index_buffer = 0;
		Drapeau_commande_recue = false;
	}
}

// =========================================================================
// OPTIMISATION ÉNERGÉTIQUE : MISE EN VEILLE DE L'AFFICHAGE
// =========================================================================

void mise_veille_aff(void) {
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	// Activation des horloges des ports A et B
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	// Configuration en mode ANALOGIQUE sans résistance de tirage
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;

	// Configuration de la broche PA12
	GPIO_InitStruct.Pin = GPIO_PIN_12;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	// Configuration des broches PB5 et PB6
	GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}


// =========================================================================
// FONCTION D'INITIALISATION DE L'AFFICHAGE ET DE LA SÉRIE
// =========================================================================
void Affichage_Init_Demarrage(void) {
	setColor(0xFFFF);

	setBackgroundColor(0x0000);
	drawString(20, 30, FONT_LG, "The");

	setColor(0x07E0);
	drawString(20, 60, FONT_LG, "ScreenV12");

	setColor(0xF800);
	drawString(20, 90, FONT_LG, "is good!");

	//ENVOI DU MESSAGE D'ACCUEIL (Remplacement du HAL par du Bare-Metal) = beaucoup plus propre, on passe directement le texte sans s'occuper de sa taille !
	UART2_SendString("\r\n=== LIAISON SERIE INITIALISEE ===\r\nEnvoyez ON, DWN, HUP ou RTN pour naviguer.\r\n");

	//ARMEMENT DE LA RÉCEPTION SÉRIE PAR INTERRUPTION = On demande au composant de déclencher une interruption dès qu'il reçoit 1 caractère
	HAL_UART_Receive_IT(&huart2, &rx_char, 1);
}

void GESTION_Systeme(void) {
	// =========================================================================
	// GESTION DE L'ÉPHÉMÉRIDE
	// =========================================================================
	if (sTime.Hours == 0 && sTime.Minutes == 0) {
		if (calcul_fait_aujourdhui == 0) {

			calculerEphemeride();

			calcul_fait_aujourdhui = 1; // Bloque pour éviter de recalculer en boucle pendant 1 min

			// Envoi sur la liaison série (Bare-Metal)
			char msg_eph[60];
			sprintf(msg_eph, "Ephemeride - Lever: %02d:%02d | Coucher: %02d:%02d\r\n",heureGPSO, minGPSO, heureGPSF, minGPSF);

			// Remplacement HAL_UART_Transmit par notre fonction Bare-Metal
			UART2_SendString(msg_eph);
		}
	} else {
		// Dès qu'il est 00h01, on réarme le système pour la nuit prochaine
		calcul_fait_aujourdhui = 0;
	}

	// =========================================================================
	// LOGIQUE DE RÉVEIL MATÉRIEL ET LOGICIEL (Appui sur une touche ou reçevoir une trame)
	// =========================================================================
	if (touche_ON || touche_DWN || touche_HUP || touche_RTN) {

		dernier_temps_actif = HAL_GetTick(); // On mémorise l'instant réel de l'action

		if (ecran_allume == false) {
			// Ces réinitialisations lourdes (SPI, GPIO globaux) restent en HAL car les refaire en Bare-Metal serait trop long sans gain de performance.
			HAL_SPI_DeInit(&hspi1);
			MX_GPIO_Init();
			MX_SPI1_Init();

			// --- DEVIENT BARE-METAL ---
			// Au lieu de HAL_GPIO_WritePin, on écrit directement dans le registre BSRR du port.
			// Pour faire un RESET (0V) sur STM32, on décale le numéro de la broche de 16 bits.

			ALIM_AFF_GPIO_Port->BSRR = (ALIM_AFF_Pin << 16); // Ce délai reste nécessaire pour laisser l'écran s'allumer électriquement

			initialise_LCD();
			setOrientation(4);
			clearScreen(1);

			ecran_allume = true;

			// Remplacement de HAL_UART_Transmit par le Bare-Metal
			UART2_SendString("[SYSTEM] Ecran rallume et initialise !\r\n");
		}
	}
	// =========================================================================
	// LOGIQUE DE MISE EN VEILLE AUTOMATIQUE DE L'ÉCRAN
	// =========================================================================
	if (ecran_allume == true) {
		if ((HAL_GetTick() - dernier_temps_actif) >= 10000) { // Après 10 secondes d'inactivité

			// VEILLE LOGIQUE (Commandes SPI pour éteindre le contrôleur TFT)
			writeCommand(0x28);   // Display OFF
			writeCommand(0x10);   // Sleep In

			// VEILLE MATÉRIELLE BARE-METAL
			// Remplace HAL_GPIO_WritePin pour forcer la broche à l'état HAUT (3.3V).
			// Sur le registre BSRR, écrire sur les 16 premiers bits met la broche à 1 (SET).
			ALIM_AFF_GPIO_Port->BSRR = ALIM_AFF_Pin;

			HAL_Delay(100);

			mise_veille_aff(); // Bascule les broches en mode analogique (Low-Power)
			ecran_allume = false;

			// Remplacement HAL par Bare-Metal
			UART2_SendString("[SYSTEM] Mode Veille Ecran active\r\n");
		}
	}

	// =========================================================================
	// GESTION DE L'AFFICHAGE DE LA MACHINE À ÉTATS
	// =========================================================================
	if (ecran_allume == true) {
		menu(); // Affiche les différents menus si l'écran est allumé
	}

	HAL_Delay(1);

	// =========================================================================
	//  MISE EN VEILLE DU MICROCONTRÔLEUR (Basse conso)
	// =========================================================================

	// On réinitialise les "drapeaux" des touches pour le prochain cycle
	touche_ON = 0;
	touche_DWN = 0;
	touche_HUP = 0;
	touche_RTN = 0;

	if (ecran_allume == false) {
		//index_buffer = 0;
		mise_veille_aff(); // Sécurité : on s'assure que les broches sont isolées
		/* * MISE EN VEILLE PROFONDE DU STM32 (DÉSACTIVÉE POUR L'INTÉGRATION)
		* -----------------------------------------------------------------
		* Explication : On commente ce bloc pendant la phase de développement et de test.
		* Si on laisse HAL_PWR_EnterSLEEPMode, le STM32 coupe ses horloges. Cela va déconnecter le ST-Link (débogueur) et rendre la recherche de bugs très difficile.
		* À décommenter uniquement lors de la compilation finale pour le déploiement.
		*/
		/*
		HAL_SuspendTick(); // Arrête le chronomètre système (HAL_GetTick)
		HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI); // Mode WFI (Wait For Interrupt)
		// --- LE REVEIL DU MICROCONTRÔLEUR A LIEU ICI ---
		// Quand une interruption (comme le tactile) survient, le code reprend ici
		HAL_ResumeTick(); // Relance le chronomètre système
		HAL_Delay(100);
		dernier_temps_actif = HAL_GetTick(); // On mémorise l'heure du réveil
		 */
	}
}

// =========================================================================
// GESTION DES INTERRUPTIONS DE LA LIAISON SÉRIE (UART)
// =========================================================================

/* Cette fonction est appelée automatiquement par le microcontrôleur dès qu'un nouveau caractère arrive sur le port série. */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART2) {

		// Si on reçoit le caractère de fin de trame
		if (rx_char == '\n' || rx_char == '\r') {
			if (index_buffer > 0) {
				rx_buffer[index_buffer] = '\0'; // On ferme proprement la chaîne de caractères
				Drapeau_commande_recue = true;  // Déclenche l'analyse dans la boucle principale
				index_buffer = 0;               // Réinitialise pour la prochaine commande
			}
		}
		else {
			// Remplissage sécurisé du tampon pour éviter un crash de la mémoire ("Buffer Overflow")
			if (index_buffer < 9) {
				rx_buffer[index_buffer] = (char)rx_char;
				index_buffer++;
			}
		}

		// On réarme l'interruption matérielle pour autoriser la réception du caractère suivant
		HAL_UART_Receive_IT(&huart2, &rx_char, 1);
	}
}
// =========================================================================
// MISE À JOUR DE L'HEURE ET DE L'ÉPHÉMÉRIDE
// =========================================================================

// Ancien design
/*void rafraichir_ephemeride(void) {
	// 1. Lecture sécurisée de l'Heure depuis le composant matériel (RTC)
	HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);

	// 2. Lecture sécurisée de la Date (Obligatoire juste après GetTime pour déverrouiller les registres RTC)
	HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

	// 3. Conversion du format propriétaire STM32 vers le format Standard C (struct tm)
	tm_temps.tm_mday = sDate.Date;
	tm_temps.tm_mon  = sDate.Month - 1;  // les mois vont de 0 (Janvier) à 11 (Décembre)
	tm_temps.tm_year = sDate.Year + 100; // Le STM32 compte depuis 2000, 'struct tm' compte depuis 1900.

	// 4. Lancement de l'algorithme astronomique avec les nouvelles données
	calculerEphemeride();
}
*/
// =========================================================================
// ANTI-PLANTAGE LIAISON SÉRIE (Gestion de l'Overrun Error)
// =========================================================================
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        // Si le port série plante (erreur de surcharge ORE),
        // on force la réactivation de l'interruption !
        HAL_UART_Receive_IT(&huart2, &rx_char, 1);
    }
}
