/*
 * config.c
 *
 *  Created on: Apr 20, 2026
 *      Author: ALLAM Tarel
 */
/*
 * config.c
 *
 * Modifié pour portage STM32
 * Auteur : ALLAM Tarek
 */

#include "config.h"

// On importe le "handle" (le gestionnaire) du SPI1 généré automatiquement par CubeMX dans main.c
extern SPI_HandleTypeDef hspi1;

// Fonction pour envoyer une COMMANDE à l'écran
void writeCommand(uint8_t cmd) {
    // 1. Mode Commande : on met la broche A0 à 0
    HAL_GPIO_WritePin(A0_GPIO_Port, A0_Pin, GPIO_PIN_RESET);

    // 2. Sélection de l'écran : on met CS à 0 (Actif à l'état bas)
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);

    // 3. Envoi de l'octet via SPI (timeout de 100ms)
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);

    // 4. Désélection de l'écran : on remet CS à 1
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
}

// Fonction pour envoyer une DONNÉE à l'écran
void writeData(uint8_t data) {
    // 1. Mode Donnée : on met la broche A0 à 1
    HAL_GPIO_WritePin(A0_GPIO_Port, A0_Pin, GPIO_PIN_SET);

    // 2. Sélection de l'écran : on met CS à 0
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);

    // 3. Envoi de l'octet via SPI
    HAL_SPI_Transmit(&hspi1, &data, 1, 100);

    // 4. Désélection de l'écran : on remet CS à 1
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
}

// Fonction de délai millisecondes compatible STM32
void delay(uint32_t t) {
    HAL_Delay(t);
}

