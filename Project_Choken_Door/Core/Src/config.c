/*
 * config.c
 *
 *  Created on: Apr 20, 2026
 *      Author: ALLAM Tarek
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

/*void writeCommand(uint8_t cmd) {
    // 1. Mode Commande : on met la broche A0 à 0
    HAL_GPIO_WritePin(A0_GPIO_Port, A0_Pin, GPIO_PIN_RESET);

    // 2. Sélection de l'écran : on met CS à 0 (Actif à l'état bas)
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);

    // 3. Envoi de l'octet via SPI (timeout de 100ms)
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);

    // 4. Désélection de l'écran : on remet CS à 1
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
}
*/

void writeCommand(uint8_t cmd) {
    // 1. Mode Commande : on met la broche A0 à 0
    // L'utilisation de la partie haute du registre BSRR (Bit Set/Reset Register)
    // permet une mise à 0 instantanée en 1 seul cycle d'horloge.
    A0_GPIO_Port->BRR = (uint32_t)A0_Pin;

    // 2. Sélection de l'écran : on met CS à 0
    CS_GPIO_Port->BRR = (uint32_t)CS_Pin;

    SPI1->CR1 |= SPI_CR1_SPE;

    // 3. Envoi de l'octet via SPI (Sans timeout)
    // A. On attend que le buffer d'émission (TXE = Transmit buffer Empty) soit prêt
 //   while (!(SPI1->SR & SPI_SR_TXE));

    // B. On écrit directement dans le Data Register (DR).
    // Le cast en pointeur 8 bits est crucial pour forcer le STM32
    // à n'envoyer que 8 bits et non 16 bits.
    *((__IO uint8_t *)&SPI1->DR) = cmd;

    // C. ATTENTION PIÈGE : On attend que le bus SPI ait physiquement fini d'envoyer
    // les données (BSY = Busy flag doit retomber à 0).
    while (SPI1->SR & SPI_SR_BSY);

    // 4. Désélection de l'écran : on remet CS à 1
    // L'utilisation de la partie basse du registre BSRR met la broche à 1 instantanément.
    CS_GPIO_Port->BSRR = CS_Pin;
}

// Fonction pour envoyer une DONNÉE à l'écran
/*void writeData(uint8_t data) {
    // 1. Mode Donnée : on met la broche A0 à 1
    HAL_GPIO_WritePin(A0_GPIO_Port, A0_Pin, GPIO_PIN_SET);

    // 2. Sélection de l'écran : on met CS à 0
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET);

    // 3. Envoi de l'octet via SPI
    HAL_SPI_Transmit(&hspi1, &data, 1, 100);

    // 4. Désélection de l'écran : on remet CS à 1
    HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
}
*/

void writeData(uint8_t data) {
    // 1. Mode Donnée : on met la broche A0 à 1
    // L'écriture dans la partie basse (les 16 premiers bits) du registre BSRR
    // met la broche à l'état HAUT instantanément.
    A0_GPIO_Port->BSRR = A0_Pin;

    // 2. Sélection de l'écran : on met CS à 0 (Partie haute du BSRR = état BAS)
    CS_GPIO_Port->BRR = (uint32_t)CS_Pin;

    SPI1->CR1 |= SPI_CR1_SPE;

    // 3. Envoi de l'octet via SPI (Sans timeout)
    // A. On attend que le buffer d'émission soit vide
//    while (!(SPI1->SR & SPI_SR_TXE));

    // B. On écrit la donnée directement dans le registre
    *((__IO uint8_t *)&SPI1->DR) = data;

    // C. On attend impérativement que le bus physique ait terminé de transmettre
    while (SPI1->SR & SPI_SR_BSY);

    // 4. Désélection de l'écran : on remet CS à 1
    CS_GPIO_Port->BSRR = CS_Pin;
}


// Fonction de délai millisecondes compatible STM32
void delay(uint32_t t) {
    HAL_Delay(t);
}

