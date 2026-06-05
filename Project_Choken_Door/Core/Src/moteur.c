/* ===========================================================================
 * Fichier : moteur.c
 * Description : Gestion du moteur (PWM, sécurité courant, UART, Mode veille)
 * =========================================================================== */

#include "moteur.h"
#include <stdio.h>
#include <string.h>

/* ===========================================================================
 * LIAISONS EXTERNES (Périphériques et fonctions définis dans main.c)
 * =========================================================================== */
extern TIM_HandleTypeDef htim1;
extern LPTIM_HandleTypeDef hlptim1;
extern ADC_HandleTypeDef hadc1;
extern UART_HandleTypeDef huart2;

/* Prototypes des fonctions CubeMX et de gestion d'horloge du main.c */
extern void MX_GPIO_Init(void);
extern void MX_TIM1_Init(void);
extern void MX_LPTIM1_Init(void);
extern void MX_ADC1_Init(void);
extern void Clock_SwitchToSleep(void);
extern void Clock_SwitchToFullSpeed(void);

/* ===========================================================================
 * VARIABLES GLOBALES
 * =========================================================================== */
MotorState_t motor_state = MOTEUR_OFF;
uint32_t motor_start_time = 0;
uint32_t last_tick = 0;
uint32_t elapsed = 0;
uint32_t disconnect_start_time = 0;
uint16_t compteur_securite = 0;
uint16_t current_speed = 700; // Vitesse minimale par défaut
uint16_t speed_level = 1;     // Niveau par défaut
uint32_t adc_value = 0;

float courant_moteur = 0.0f;
float moyenne_courant = 0.0f;
float courant_fonctionnement_morteur = 0.0f; // Conservé tel quel pour compatibilité
float threshold = 1.0f;
float lecture[5] = {0};
float somme_courant = 0.0f;

volatile uint8_t rx_data = 0;
volatile uint8_t rx_pending = 0;

char msg_status[300];
char msg_cmd[100];

// Tableau des vitesses correspondantes aux niveaux 1 à 4
uint16_t speed_table[4] = {700, 800, 900, 999};
uint8_t led_active = 0;
uint32_t led_timer = 0;


/* ===========================================================================
 * FONCTIONS PRIVÉES (Uniquement visibles dans moteur.c)
 * =========================================================================== */

/**
 * @brief Éteint l'ensemble des 4 LEDs de visualisation.
 */
static void LED_AllOff(void)
{
    HAL_GPIO_WritePin(LED_G_1_PORT, LED_G_1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_G_2_PORT, LED_G_2_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_D_3_PORT, LED_D_3_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_D_4_PORT, LED_D_4_PIN, GPIO_PIN_RESET);
}

/**
 * @brief Inverse l'état (Toggle) des 4 LEDs simultanément.
 */
static void LED_ToggleAll(void)
{
    HAL_GPIO_TogglePin(LED_G_1_PORT, LED_G_1_PIN);
    HAL_GPIO_TogglePin(LED_G_2_PORT, LED_G_2_PIN);
    HAL_GPIO_TogglePin(LED_D_3_PORT, LED_D_3_PIN);
    HAL_GPIO_TogglePin(LED_D_4_PORT, LED_D_4_PIN);
}

/**
 * @brief Arrêt d'urgence matériel direct du pont en H et des PWM.
 */
static void Motor_HardStop(void)
{
    HAL_LPTIM_PWM_Stop(&hlptim1, LPTIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2,  GPIO_PIN_RESET);
}

/**
 * @brief Arrête uniquement les signaux PWM (TIM1 et LPTIM1).
 */
static void PWM_StopAll(void)
{
    HAL_LPTIM_PWM_Stop(&hlptim1, LPTIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);
}

/**
 * @brief Initialise les compteurs de temps lors d'un démarrage moteur.
 */
static void Motor_StartTimer(void)
{
    motor_start_time  = HAL_GetTick();
    motor_state       = DEMARRAGE_MOTEUR;
    compteur_securite = 0;
    elapsed           = 0;
}

/**
 * @brief Envoie un rapport textuel complet de l'état du système via l'UART.
 */
static void UART_Send_Status(void)
{
    int len = sprintf(msg_status,
        "\r\n+--------------------------------------+\r\n"
        "|            ETAT DU SYSTEME           |\r\n"
        "+--------------------------------------+\r\n"
        "| Courant Moy.   : %6.3f A            |\r\n"
        "| Courant Fct.   : %6.3f A            |\r\n"
        "| Seuil (Thresh) : %6.3f A            |\r\n"
        "| Compteur Secu. : %-6d              |\r\n"
        "| Etat Moteur    : %-6d              |\r\n"
        "| Temps Ecoule   : %-8lu ms         |\r\n"
        "| Niveau Vitesse : %-6d              |\r\n"
        "+--------------------------------------+\r\n",
        moyenne_courant, courant_fonctionnement_morteur,
        threshold, compteur_securite, (int)motor_state, elapsed, speed_level);

    HAL_UART_Transmit(&huart2, (uint8_t*)msg_status, len, 200);
}

/**
 * @brief Machine à États Finis (FSM) gérant la sécurité et le cycle du moteur.
 */
static void Motor_Security_FSM(uint32_t current_time)
{
    elapsed = current_time - motor_start_time;

    if (motor_state == DEMARRAGE_MOTEUR && elapsed > 2000)
    {
        motor_state = CALIBRATION_MOTEUR;
        HAL_UART_Transmit(&huart2, (uint8_t*)"Calibrage...\r\n", 14, 10);
    }
    else if (motor_state == CALIBRATION_MOTEUR && elapsed > 3000)
    {
        courant_fonctionnement_morteur = moyenne_courant;
        if (courant_fonctionnement_morteur < 0.05f) courant_fonctionnement_morteur = 0.05f;

        threshold   = courant_fonctionnement_morteur * 1.25f;
        motor_state = MOTEUR_MARCHE;

        int len = sprintf(msg_status, "Seuil fixé à: %.2f A\r\n", threshold);
        HAL_UART_Transmit(&huart2, (uint8_t*)msg_status, len, 50);
    }
    else if (motor_state == MOTEUR_MARCHE && elapsed > 20000)
    {
        Motor_Stop();
        HAL_UART_Transmit(&huart2, (uint8_t*)"FIN DE COURSE (TEMPS)\r\n", 23, 50);
        elapsed = 0;
    }
    else if (motor_state == MOTEUR_MARCHE)
    {
        /* Détection Absence Moteur (Courant nul) */
        if (moyenne_courant < 0.04f)
        {
            if (disconnect_start_time == 0)
            {
                disconnect_start_time = current_time;
                HAL_UART_Transmit(&huart2, (uint8_t*)"[!] Courant nul ! Verification en cours...\r\n", 44, 100);
            }
            else
            {
                uint32_t temps_suspect = current_time - disconnect_start_time;

                if (temps_suspect >= 5000)
                {
                    Motor_HardStop();
                    motor_state = MOTEUR_ERREUR_ABSENCE;
                    HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n!!! DEFAUT: MOTEUR DECONNECTE (TIMEOUT 5S) !!!\r\n", 51, 100);
                    elapsed = 0;
                    disconnect_start_time = 0;
                }
                else
                {
                    char msg_countdown[60];
                    int len_count = sprintf(msg_countdown, "[!] Test absence... en cours depuis %lu ms / 5000 ms\r\n", temps_suspect);
                    HAL_UART_Transmit(&huart2, (uint8_t*)msg_countdown, len_count, 50);
                }
            }
        }
        /* Détection Blocage Moteur (Surintensité répétée) */
        else if (moyenne_courant > threshold)
        {
            disconnect_start_time = 0;
            compteur_securite++;

            if (compteur_securite >= 5)
            {
                Motor_HardStop();
                motor_state = MOTEUR_ERREUR_BLOCAGE;
                HAL_UART_Transmit(&huart2, (uint8_t*)"!!! DEFAUT: BLOCAGE MOTEUR  !!!\r\n", 35, 100);
                elapsed = 0;
            }
        }
        else
        {
            disconnect_start_time = 0;
            compteur_securite = 0;
        }
    }
}


/* ===========================================================================
 * FONCTIONS PUBLIQUES (Appelables depuis l'extérieur / main.c)
 * =========================================================================== */

/**
 * @brief Active la marche avant du moteur via TIM1.
 */
void Motor_Forward(void)
{
    PWM_StopAll();
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
    HAL_Delay(5);

    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, current_speed);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
    Motor_StartTimer();
}

/**
 * @brief Active la marche arrière du moteur via LPTIM1.
 */
void Motor_Reverse(void)
{
    PWM_StopAll();
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2,  GPIO_PIN_RESET);
    HAL_Delay(5);

    HAL_LPTIM_PWM_Start(&hlptim1, LPTIM_CHANNEL_1);
    __HAL_LPTIM_COMPARE_SET(&hlptim1, LPTIM_CHANNEL_1, current_speed);
    Motor_StartTimer();
}

/**
 * @brief Arrête proprement le moteur et réinitialise les sécurités de base.
 */
void Motor_Stop(void)
{
    PWM_StopAll();
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2,  GPIO_PIN_RESET);

    motor_state = MOTEUR_OFF;
    threshold   = 1.0f;

    LED_AllOff();
    led_active = 0;
}

/**
 * @brief Modifie la vitesse actuelle du moteur et met à jour l'affichage LED.
 */
void Motor_SetSpeed(uint16_t speed)
{
    if (speed > 999)             speed = 999;
    if (speed > 0 && speed < 700) speed = 700;

    current_speed = speed;

    if (motor_state != MOTEUR_OFF)
    {
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, current_speed);
        __HAL_LPTIM_COMPARE_SET(&hlptim1, LPTIM_CHANNEL_1, current_speed);
    }

    HAL_GPIO_WritePin(LED_G_1_PORT, LED_G_1_PIN, (speed_level >= 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_G_2_PORT, LED_G_2_PIN, (speed_level >= 2) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_D_3_PORT, LED_D_3_PIN, (speed_level >= 3) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_D_4_PORT, LED_D_4_PIN, (speed_level >= 4) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    led_timer  = HAL_GetTick();
    led_active = 1;
}

/**
 * @brief Effectue une mesure instantanée du courant consommé par le moteur via l'ADC.
 */
float Get_Motor_Current(void)
{
    float res_ohm = 1.3f;
    float offset  = 0.062f;

    HAL_ADC_Start(&hadc1);

    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
    {
        adc_value = HAL_ADC_GetValue(&hadc1);
        courant_moteur = (((adc_value * 3.3f) / 4095.0f) / res_ohm) * 1.103f;
        courant_moteur -= offset;

        if (courant_moteur < 0) courant_moteur = 0;
    }

    HAL_ADC_Stop(&hadc1);
    return courant_moteur;
}

/**
 * @brief Calcule la moyenne glissante du courant sur les 5 derniers échantillons.
 */
float Update_Moving_Average(float new_sample)
{
    lecture[0] = lecture[1];
    lecture[1] = lecture[2];
    lecture[2] = lecture[3];
    lecture[3] = lecture[4];
    lecture[4] = new_sample;

    somme_courant = 0;
    for (int i = 0; i < 5; i++) somme_courant += lecture[i];

    return somme_courant / 5.0f;
}

/**
 * @brief Gestion périodique (cadencée à 1s) des mesures de courant et de la FSM.
 */
void Motor_Periodic_Update(void)
{
    uint32_t current_time = HAL_GetTick();

    if (current_time - last_tick >= 1000)
    {
        last_tick = current_time;

        float instant   = Get_Motor_Current();
        moyenne_courant = Update_Moving_Average(instant);

        Motor_Security_FSM(current_time);
        UART_Send_Status();
    }
}

/**
 * @brief Analyse et exécute les commandes reçues depuis le terminal UART.
 */
void Process_UART_Command(void)
{
    // Si l'interruption n'a encore rien reçu, on quitte immédiatement
    if (rx_pending == 0) return;

    // Un caractère est arrivé ! On baisse le drapeau
    rx_pending = 0;

    // ÉCHO : On renvoie le caractère reçu au PC (cast pour éviter le warning volatile)
    HAL_UART_Transmit(&huart2, (uint8_t*)&rx_data, 1, 10);

    // Analyse du caractère reçu
    switch (rx_data)
    {
        case 'D': Motor_Forward(); break;
        case 'A': Motor_Reverse(); break;
        case 'S': Motor_Stop();    break;

        case '+':
            if (speed_level < 4)
            {
                speed_level++;
                current_speed = speed_table[speed_level - 1];
                Motor_SetSpeed(current_speed);
            }
            break;

        case '-':
            if (speed_level > 1)
            {
                speed_level--;
                current_speed = speed_table[speed_level - 1];
                Motor_SetSpeed(current_speed);
            }
            break;
    }

}
/**
 * @brief Éteint automatiquement la barre de LEDs de vitesse après 1.5 seconde.
 */
void Update_LED_Timeout(void)
{
    if (led_active && (HAL_GetTick() - led_timer >= 1500))
    {
        LED_AllOff();
        led_active = 0;
    }
}

/**
 * @brief Gère le clignotement asynchrone (non-bloquant) des LEDs en cas d'erreur.
 */
void Gerer_Erreur_Moteur(void)
{
    static uint32_t last_blink_tick = 0;
    uint32_t intervalle = 0;

    if      (motor_state == MOTEUR_ERREUR_ABSENCE) intervalle = 3000; // Lent : déconnexion
    else if (motor_state == MOTEUR_ERREUR_BLOCAGE) intervalle = 500;  // Rapide : blocage

    if (intervalle > 0 && (HAL_GetTick() - last_blink_tick >= intervalle))
    {
        last_blink_tick = HAL_GetTick();
        LED_ToggleAll();
    }
}

/**
 * @brief Fait passer le microcontrôleur en mode SLEEP basse consommation avec réveil UART.
 */
void Enter_Low_Power_Mode(void)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)"Entree en veille...\r\n", 21, 50);
    HAL_Delay(10); // Laisse l'UART vider son buffer TX

    /* 1. Purge UART */
    __HAL_UART_FLUSH_DRREGISTER(&huart2);
    __HAL_UART_CLEAR_FLAG(&huart2, UART_CLEAR_OREF | UART_CLEAR_NEF | UART_CLEAR_FEF | UART_CLEAR_PEF);
    volatile uint32_t tmpreg = huart2.Instance->RDR;
    (void)tmpreg;

    /* 2. GPIO en analogique pour supprimer les courants de fuite */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;

    // Conserver PA2/PA3 (UART) et PA13/PA14 (SWD de débug)
    GPIO_InitStruct.Pin = GPIO_PIN_ALL & ~(GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_13 | GPIO_PIN_14);
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_ALL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* 3. Clock gating (désactivation des horloges inutiles) */
    __HAL_RCC_TIM1_CLK_DISABLE();
    __HAL_RCC_LPTIM1_CLK_DISABLE();
    __HAL_RCC_ADC_CLK_DISABLE();

    /* 4. Réduction de l'horloge système (ex: bascule sur MSI ou division) */
    Clock_SwitchToSleep();

    /* 5. Configuration de l'interruption de réveil sur l'horloge réduite */
    HAL_UART_Receive_IT(&huart2, &rx_data, 1);

    /* 6. Désactivation du SysTick pour éviter un réveil toutes les 1 ms */
    HAL_SuspendTick();

    /* Entrée effective en mode SLEEP (Attente d'interruption UART - WFI) */
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);

    /* =================== RÉVEIL ICI =================== */

    HAL_ResumeTick(); // Relance le SysTick immédiatement

    /* 7. Restauration de l'horloge système pleine vitesse (ex: 48 MHz) */
    Clock_SwitchToFullSpeed();

    /* 8. Réactivation des horloges périphériques */
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_LPTIM1_CLK_ENABLE();
    __HAL_RCC_ADC_CLK_ENABLE();

    /* 9. Restauration complète des configurations matérielles */
    MX_GPIO_Init();
    MX_TIM1_Init();
    MX_LPTIM1_Init();
    MX_ADC1_Init();

    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
    __HAL_TIM_MOE_ENABLE(&htim1);

    HAL_UART_Transmit(&huart2, (uint8_t*)"Reveil OK !\r\n", 13, 50);
}

/**
 * @brief Callback HAL automatique lors de la réception complète d'un caractère UART.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        rx_pending = 1;
        HAL_UART_Receive_IT(&huart2, &rx_data, 1);
    }
}

/* ---------------------------------------------------------------------------
 * HAL_UART_ErrorCallback — Sécurité anti-blocage de l'interruption RX
 * Si une erreur de frame, de bruit ou d'overrun (ORE) arrive (très fréquent
 * lors des gros envois de texte), la HAL coupe l'IT. Cette fonction la relance.
 * --------------------------------------------------------------------------- */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        // Relance immédiatement l'interruption de réception pour ne pas perdre la main
        HAL_UART_Receive_IT(&huart2, (uint8_t*)&rx_data, 1);
    }
}
