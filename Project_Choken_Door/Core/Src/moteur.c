/* moteur.c */
#include "moteur.h"
#include <stdio.h>
#include <string.h>

// Rappel des périphériques configurés dans le main.c (liaisons externes)
extern TIM_HandleTypeDef htim1;
extern LPTIM_HandleTypeDef hlptim1;
extern ADC_HandleTypeDef hadc1;
extern UART_HandleTypeDef huart2;

// Déclaration et initialisation des variables globales de ta partie
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
float courant_fonctionnement_morteur = 0.0f;
float threshold = 1.0f;
float lecture[5] = {0};
float somme_courant = 0.0f;

uint8_t rx_data = 0;
uint8_t rx_pending = 0;

// ⚠ Taille calculée : trame UART_Send_Status ≈ 464 octets → 512 avec marge de sécurité
// Ancien [300] causait un débordement de 164 octets → corruption mémoire → crash UART
char msg_status[512];
char msg_cmd[100];

// Tableau des vitesses correspondantes aux niveaux 1 à 4
uint16_t speed_table[4] = {700, 800, 900, 999};
uint8_t led_active = 0;
uint32_t led_timer = 0;


/* ===========================================================================
 * FONCTIONS PRIVÉES (uniquement visibles dans moteur.c)
 * =========================================================================== */

static void LED_AllOff(void)
{
    HAL_GPIO_WritePin(LED_G_1_PORT, LED_G_1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_G_2_PORT, LED_G_2_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_D_3_PORT, LED_D_3_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_D_4_PORT, LED_D_4_PIN, GPIO_PIN_RESET);
}

static void LED_ToggleAll(void)
{
    HAL_GPIO_TogglePin(LED_G_1_PORT, LED_G_1_PIN);
    HAL_GPIO_TogglePin(LED_G_2_PORT, LED_G_2_PIN);
    HAL_GPIO_TogglePin(LED_D_3_PORT, LED_D_3_PIN);
    HAL_GPIO_TogglePin(LED_D_4_PORT, LED_D_4_PIN);
}

static void Motor_HardStop(void)
{
    HAL_LPTIM_PWM_Stop(&hlptim1, LPTIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2,  GPIO_PIN_RESET);
}

static void PWM_StopAll(void)
{
    HAL_LPTIM_PWM_Stop(&hlptim1, LPTIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);
}

static void Motor_StartTimer(void)
{
    motor_start_time  = HAL_GetTick();
    motor_state       = DEMARRAGE_MOTEUR;
    compteur_securite = 0;
    elapsed           = 0;
}

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
 * FONCTIONS PUBLIQUES (Appelables depuis l'extérieur)
 * =========================================================================== */

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

void Motor_SetSpeed(uint16_t speed)
{
    if (speed > 999)      speed = 999;
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

void Motor_Periodic_Update(void)
{
    uint32_t current_time = HAL_GetTick();

    if (current_time - last_tick >= 1000) // Cadence fixe de 1000 ms (ajuste à 500 si besoin)
    {
        last_tick = current_time;

        float instant   = Get_Motor_Current();
        moyenne_courant = Update_Moving_Average(instant);

        Motor_Security_FSM(current_time);
        UART_Send_Status();
    }
}

void Process_UART_Command(void)
{
    // Si l'interruption n'a rien reçu, on ne fait rien
    if (rx_pending == 0) return;

    // Un caractère a été reçu par l'interruption ! On baisse le drapeau
    rx_pending = 0;

    // Écho local : renvoie le caractère au PC pour voir ce qu'on tape
    HAL_UART_Transmit(&huart2, &rx_data, 1, 10);

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

    // IMPORTANT : On relance la réception par interruption pour le prochain caractère
    HAL_UART_Receive_IT(&huart2, &rx_data, 1);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) // Ou huart->Instance == &huart2 selon ton code
    {
        rx_pending = 1; // Lève le drapeau pour Process_UART_Command
    }
}


/* ---------------------------------------------------------------------------
 * Update_LED_Timeout — extinction automatique des LEDs vitesse après 1.5 s
 * Évite de laisser la barre de vitesse allumée indéfiniment.
 * Non appelée en état d'erreur : le clignotement d'erreur ne doit pas être coupé.
 * --------------------------------------------------------------------------- */
void Update_LED_Timeout(void)
{
    if (led_active && (HAL_GetTick() - led_timer >= 1500))
    {
        LED_AllOff();  // Éteint les 4 LEDs
        led_active = 0; // Lève le verrou de mise en veille
    }
}


/* ---------------------------------------------------------------------------
 * Gerer_Erreur_Moteur — clignotement non-bloquant selon le type d'erreur
 *
 * ERREUR_ABSENCE  → toutes les LEDs clignotent toutes les 3 s (défaut discret)
 * ERREUR_BLOCAGE  → toutes les LEDs clignotent toutes les 0.5 s (alarme rapide)
 *
 * Non-bloquant : utilise une variable statique + HAL_GetTick (pas de HAL_Delay).
 * L'état est verrouillé : seule une commande 'S' permet de sortir de l'erreur.
 * --------------------------------------------------------------------------- */
void Gerer_Erreur_Moteur(void)
{
    static uint32_t last_blink_tick = 0; // Statique : persiste entre deux appels
    uint32_t intervalle = 0;

    // Sélection de la période de clignotement selon l'erreur détectée
    if      (motor_state == MOTEUR_ERREUR_ABSENCE) intervalle = 3000; // Lent : déconnexion
    else if (motor_state == MOTEUR_ERREUR_BLOCAGE) intervalle = 500;  // Rapide : blocage

    if (intervalle > 0 && (HAL_GetTick() - last_blink_tick >= intervalle))
    {
        last_blink_tick = HAL_GetTick(); // Mémorise pour la prochaine période
        LED_ToggleAll(); // Inverse l'état des 4 LEDs simultanément
    }
}


/* ---------------------------------------------------------------------------
 * Enter_Low_Power_Mode — veille SLEEP avec réveil par UART
 *
 * Séquence :
 *   1. Purge UART → évite réveil immédiat sur flag résiduel
 *   2. GPIO → analogique → supprime courants de fuite
 *   3. Clock gating TIM1/LPTIM1/ADC
 *   4. Clock_SwitchToSleep() EN PREMIER → HAL_UART_Init recalcule le BRR
 *      ⚠ Doit précéder HAL_UART_Receive_IT : sinon l'Init annule l'IT
 *   5. HAL_UART_Receive_IT → capture l'octet de réveil dans rx_data
 *   6. SysTick off → WFI
 *   ---- RÉVEIL ----
 *   7. Clock_SwitchToFullSpeed() → BRR restauré pour 48 MHz
 *   8. Périphériques restaurés
 * --------------------------------------------------------------------------- */
void Enter_Low_Power_Mode(void)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)"Entree en veille...\r\n", 21, 50);
    HAL_Delay(10); // Laisse l'UART vider son buffer TX avant la coupure

    /* 1. Purge UART : supprime les flags résiduels pour éviter un faux réveil */
    __HAL_UART_FLUSH_DRREGISTER(&huart2);
    __HAL_UART_CLEAR_FLAG(&huart2, UART_CLEAR_OREF | UART_CLEAR_NEF | UART_CLEAR_FEF | UART_CLEAR_PEF);
    volatile uint32_t tmpreg = huart2.Instance->RDR; // Vide le registre de données
    (void)tmpreg;                                     // Évite le warning "unused variable"

    /* 2. GPIO en analogique : supprime les courants de fuite (économie de courant) */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;

    // PA2 (TX) et PA3 (RX) conservés → UART physiquement connecté
    // PA13/PA14 (SWD) conservés → débogage toujours possible
    GPIO_InitStruct.Pin = GPIO_PIN_ALL & ~(GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_13 | GPIO_PIN_14);
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_ALL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct); // Tout GPIOB en analogique (moteur + LEDs inutiles)
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* 3. Clock gating : coupe les périphériques inutiles en veille */
    __HAL_RCC_TIM1_CLK_DISABLE();
    __HAL_RCC_LPTIM1_CLK_DISABLE();
    __HAL_RCC_ADC_CLK_DISABLE();

    /* 4. Réduction clock EN PREMIER : HAL_UART_Init() recalcule le BRR @ 4 MHz
     *    Règle absolue : Clock_SwitchToSleep() AVANT HAL_UART_Receive_IT
     *    Sinon HAL_UART_Init() à l'intérieur effacerait RXNEIE → pas de réveil */
    Clock_SwitchToSleep();

    /* 5. Setup IT de réveil APRÈS le switch d'horloge
     *    HAL stocke l'octet dans rx_data → HAL_UART_RxCpltCallback → rx_pending = 1 */
    HAL_UART_Receive_IT(&huart2, &rx_data, 1);

    /* 6. SysTick off : sinon réveil toutes les 1 ms (SysTick continue en SLEEP) */
    HAL_SuspendTick();

    // CPU s'arrête ici — USART2 surveille le bus à 115200 baud @ 4 MHz
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);

    /* ======= RÉVEIL ICI ======= */

        HAL_ResumeTick(); // Relance le SysTick (HAL_GetTick() redevient fiable)

        /* 7. Restaure 48 MHz AVANT de relancer TIM1/LPTIM1 */
        Clock_SwitchToFullSpeed();

        /* 8. Réactive les horloges périphériques */
        __HAL_RCC_TIM1_CLK_ENABLE();
        __HAL_RCC_LPTIM1_CLK_ENABLE();
        __HAL_RCC_ADC_CLK_ENABLE();

        /* 9. Restaure GPIO + périphériques (perdus pendant la veille) */
        Peripherals_ReInit();

        /* ===================================================================
         * /!\ CORRECTION INTERNE REVEIL GPIO /!\
         * On reconfigure les broches du moteur qui ont été passées en ANALOGIQUE
         * =================================================================== */
        GPIO_InitTypeDef GPIO_ReInitStruct = {0};

        // 1. Reconfiguration des broches de direction/contrôle (Sorties classiques)
        GPIO_ReInitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
        GPIO_ReInitStruct.Pull  = GPIO_NOPULL;
        GPIO_ReInitStruct.Speed = GPIO_SPEED_FREQ_LOW;

        GPIO_ReInitStruct.Pin   = GPIO_PIN_11; // Broche Marche Arrière (ou Avant selon ton câblage)
        HAL_GPIO_Init(GPIOA, &GPIO_ReInitStruct);

        GPIO_ReInitStruct.Pin   = GPIO_PIN_2;  // Broche Marche Avant (ou Arrière)
        HAL_GPIO_Init(GPIOB, &GPIO_ReInitStruct);

        // Si tu as des broches NSLEEP ou RESET pour le driver, réinitialise-les aussi ici !
        // Exemple si ton NSLEEP est sur PA9 :
        // GPIO_ReInitStruct.Pin = GPIO_PIN_9;
        // HAL_GPIO_Init(GPIOA, &GPIO_ReInitStruct);

        // 2. Reconfiguration des broches PWM (Fonctions alternatives)
        // TIM1_CH4 (généralement sur PA11 ou autre selon ton .ioc, adapte si nécessaire)
        GPIO_ReInitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_ReInitStruct.Pull      = GPIO_NOPULL;
        GPIO_ReInitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
        GPIO_ReInitStruct.Alternate = GPIO_AF2_TIM1; // Vérifie le numéro d'AF dans ton .ioc si besoin
        GPIO_ReInitStruct.Pin       = GPIO_PIN_11;   // À ajuster selon la vraie broche PWM de ton TIM1_CH4
        HAL_GPIO_Init(GPIOA, &GPIO_ReInitStruct);

        /* =================================================================== */

        HAL_UART_AbortReceive(&huart2);
        HAL_UART_Receive_IT(&huart2, &rx_data, 1);

        HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4); // Relance TIM1
        __HAL_TIM_MOE_ENABLE(&htim1);             // Réactive la sortie principale de TIM1

        HAL_UART_Transmit(&huart2, (uint8_t*)"Reveil OK !\r\n", 13, 50);
    }

/**
  * @brief  Cette fonction est appelée automatiquement par le HAL
  * si l'UART plante (Overrun, Bruit, etc.)
  */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        // 1. On annule la réception bloquée
        HAL_UART_AbortReceive(huart);

        // 2. On efface les drapeaux d'erreur matériels (Overrun, Framing, Noise)
        // Note : sur STM32U0, effacer les flags se fait via le registre ICR
        __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF | UART_CLEAR_NEF | UART_CLEAR_FEF);

        // 3. On relance l'interruption pour ne pas rester sourd
        HAL_UART_Receive_IT(huart, &rx_data, 1);
    }
}
