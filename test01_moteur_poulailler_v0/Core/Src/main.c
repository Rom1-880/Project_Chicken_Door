/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

ADC_HandleTypeDef  hadc1;
LPTIM_HandleTypeDef hlptim1;
TIM_HandleTypeDef   htim1;
UART_HandleTypeDef  huart2;

/* USER CODE BEGIN PV */

/* --- Vitesse et commandes ------------------------------------------------- */
uint16_t current_speed   = 900;  // Valeur par défaut pour les tests (plage 700–999)
uint8_t  rx_data;                // Octet reçu sur l'UART (polling ou IT)
uint8_t  rx_pending      = 0;    // Flag : 1 = rx_data contient l'octet de réveil (capturé par IT)
char     msg[100];               // Buffer partagé pour sprintf + envoi UART

/* --- Mesure du courant moteur --------------------------------------------- */
uint32_t adc_value      = 0;    // Valeur brute 12 bits retournée par l'ADC
float    courant_moteur = 0.0f; // Courant instantané calculé (en Ampères)
uint32_t last_tick      = 0;    // Référence temporelle pour l'envoi périodique

/* --- Moyenne glissante sur 5 échantillons --------------------------------- */
float lecture[5]       = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}; // Fenêtre FIFO
float somme_courant    = 0.0f; // Somme recalculée à chaque échantillon
float moyenne_courant  = 0.0f; // Résultat exposé au reste du programme

/* --- Sécurité : détection de blocage ------------------------------------- */
float    courant_fonctionnement_morteur = 0.0f; // Référence établie en phase de calibration
float    threshold                      = 0.9f; // Seuil dynamique : recalculé après calibration
int      compteur_securite              = 0;    // Nombre de dépassements consécutifs du seuil
uint32_t motor_start_time               = 0;    // Horodatage du dernier démarrage moteur

/* --- Machine à états du moteur ------------------------------------------- */
typedef enum {
    MOTEUR_OFF,         // Moteur à l'arrêt, MCU peut entrer en veille
    DEMARRAGE_MOTEUR,   // Ignore les surcourants (transitoire de démarrage)
    CALIBRATION_MOTEUR, // Mesure le courant nominal pour définir le seuil
    MOTEUR_MARCHE,      // Surveillance active : détection de blocage
} MotorState_t;

MotorState_t motor_state = MOTEUR_OFF; // État initial : moteur éteint
uint32_t     elapsed     = 0;          // Durée écoulée depuis le démarrage (ms), globale car lue partout

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
static void MX_LPTIM1_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART2_UART_Init(void);

/* USER CODE BEGIN PFP */
void Enter_Low_Power_Mode(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* ---------------------------------------------------------------------------
 * PWM_StopAll
 * Arrête les deux PWM (TIM1 et LPTIM1) avant tout changement de direction.
 * Appelée systématiquement pour éviter un court-circuit du pont en H.
 * --------------------------------------------------------------------------- */
static void PWM_StopAll(void)
{
    HAL_LPTIM_PWM_Stop(&hlptim1, LPTIM_CHANNEL_1); // Coupe la PWM marche arrière
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);        // Coupe la PWM marche avant
}

/* ---------------------------------------------------------------------------
 * Motor_StartTimer
 * Réinitialise le compteur de temps et le compteur de sécurité au démarrage.
 * Mutualisé entre Motor_Forward et Motor_Reverse.
 * --------------------------------------------------------------------------- */
static void Motor_StartTimer(void)
{
    motor_start_time   = HAL_GetTick(); // Horodatage du démarrage pour la machine à états
    motor_state        = DEMARRAGE_MOTEUR; // Phase initiale : on ignore les surcourants
    compteur_securite  = 0;            // Repart de zéro pour la détection de blocage
    elapsed            = 0;            // Remise à zéro du timer affiché en UART
}

/* ---------------------------------------------------------------------------
 * Motor_Forward
 * Lance le moteur en marche avant via TIM1 CH4.
 * Séquence : arrêt propre → pins à 0 → délai sécurité → démarrage TIM1.
 * --------------------------------------------------------------------------- */
void Motor_Forward(void)
{
    PWM_StopAll(); // On coupe les deux PWM avant de changer de sens

    // Remise à zéro des deux broches de direction du pont en H
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
    HAL_Delay(5); // Pause anti-shoot-through : attend l'extinction des transistors

    // Application du rapport cyclique puis lancement de la PWM marche avant
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, current_speed);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);

    Motor_StartTimer(); // Démarre le compteur de temps et initialise la machine à états
}

/* ---------------------------------------------------------------------------
 * Motor_Reverse
 * Lance le moteur en marche arrière via LPTIM1 CH1.
 * Séquence : arrêt propre → pins à 0 → délai sécurité → démarrage LPTIM1.
 * --------------------------------------------------------------------------- */
void Motor_Reverse(void)
{
    PWM_StopAll(); // On coupe les deux PWM avant de changer de sens

    // Remise à zéro des broches de direction (ordre inverse de Forward pour le pont en H)
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2,  GPIO_PIN_RESET);
    HAL_Delay(5); // Pause anti-shoot-through

    // Lancement de LPTIM1 puis application du rapport cyclique
    HAL_LPTIM_PWM_Start(&hlptim1, LPTIM_CHANNEL_1);
    __HAL_LPTIM_COMPARE_SET(&hlptim1, LPTIM_CHANNEL_1, current_speed);

    Motor_StartTimer(); // Démarre le compteur de temps et initialise la machine à états
}

/* ---------------------------------------------------------------------------
 * Motor_Stop
 * Arrêt immédiat : coupe les PWM, décharge les broches, réinitialise l'état.
 * --------------------------------------------------------------------------- */
void Motor_Stop(void)
{
    PWM_StopAll(); // Coupe TIM1 et LPTIM1

    // Pins de direction à 0 pour laisser le pont en H en roue libre
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2,  GPIO_PIN_RESET);

    motor_state = MOTEUR_OFF; // Permet au while(1) de basculer en veille
    threshold   = 1.0f;       // Remet le seuil à la valeur par défaut pour le prochain démarrage
}

/* ---------------------------------------------------------------------------
 * Motor_SetSpeed
 * Applique une nouvelle vitesse avec clamp : [700, 999].
 * Met à jour les deux timers simultanément pour un changement sans à-coup.
 * --------------------------------------------------------------------------- */
void Motor_SetSpeed(uint16_t speed)
{
    if (speed > 999)              speed = 999; // Limite haute : période du timer = 999
    if (speed > 0 && speed < 700) speed = 700; // Limite basse : en dessous le moteur ne tourne pas

    current_speed = speed;

    // Mise à jour immédiate des deux timers (l'un est peut-être inactif, mais sans danger)
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, current_speed);
    __HAL_LPTIM_COMPARE_SET(&hlptim1, LPTIM_CHANNEL_1, current_speed);
}

/* ---------------------------------------------------------------------------
 * Get_Motor_Current
 * Lit l'ADC, convertit en Ampères avec la formule : I = (Vadc / R) * correction.
 * Retire l'offset résiduel et clamp à 0 pour éviter les valeurs négatives.
 * --------------------------------------------------------------------------- */
float Get_Motor_Current(void)
{
    float res_ohm = 1.3f;  // Résistance shunt mesurée (Ω)
    float offset  = 0.062f; // Offset de tension résiduel à l'état de repos (A)

    HAL_ADC_Start(&hadc1); // Déclenche une conversion unique (mode logiciel)

    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
    {
        adc_value = HAL_ADC_GetValue(&hadc1); // Lecture de la valeur 12 bits (0–4095)

        // Formule : I = (ADC/4095 * Vref) / R * facteur_correction
        courant_moteur = (((adc_value * 3.3f) / 4095.0f) / res_ohm) * 1.103f;
        courant_moteur -= offset; // Soustrait le zéro de l'ampèremètre à vide

        if (courant_moteur < 0) courant_moteur = 0; // Pas de courant négatif physiquement
    }

    HAL_ADC_Stop(&hadc1); // Libère l'ADC
    return courant_moteur;
}

/* ---------------------------------------------------------------------------
 * Update_Moving_Average
 * Fenêtre glissante FIFO de 5 échantillons : lisse le courant pour éviter
 * les fausses détections de blocage dues aux pics transitoires.
 * --------------------------------------------------------------------------- */
float Update_Moving_Average(float new_sample)
{
    // Décalage FIFO : l'indice 0 est le plus ancien, 4 le plus récent
    lecture[0] = lecture[1];
    lecture[1] = lecture[2];
    lecture[2] = lecture[3];
    lecture[3] = lecture[4];
    lecture[4] = new_sample; // Insère le nouvel échantillon en queue

    // Recalcul de la somme (plus simple qu'un calcul incrémental sur 5 valeurs)
    somme_courant = 0;
    for (int i = 0; i < 5; i++) {
        somme_courant += lecture[i];
    }

    return somme_courant / 5.0f; // Moyenne arithmétique des 5 derniers échantillons
}

/* ---------------------------------------------------------------------------
 * UART_Send_Status
 * Formate et envoie la trame de debug périodique sur la liaison série.
 * Centralisé ici pour ne pas surcharger la machine à états.
 * --------------------------------------------------------------------------- */
static void UART_Send_Status(void)
{
    int len = sprintf(msg,
        "I:%.3fA | courant_fonctionnement:%.3fA \r\n"
        "|threshold:%.3fA|counter :%d |Etat :%d \r\n"
        " TIMER:%lu ms\r\n",
        moyenne_courant,
        courant_fonctionnement_morteur,
        threshold,
        compteur_securite,
        (int)motor_state,
        elapsed);

    HAL_UART_Transmit(&huart2, (uint8_t*)msg, len, 50);
}

/* ---------------------------------------------------------------------------
 * Motor_Security_FSM
 * Machine à états de surveillance du courant moteur :
 *   DEMARRAGE    → attend 2 s (transitoire ignoré)
 *   CALIBRATION  → mesure le courant nominal et fixe le seuil à +20 %
 *   MARCHE       → détecte un blocage (5 dépassements consécutifs) ou fin de course (20 s)
 * --------------------------------------------------------------------------- */
static void Motor_Security_FSM(uint32_t current_time)
{
    elapsed = current_time - motor_start_time; // Temps écoulé depuis le dernier démarrage

    if (motor_state == DEMARRAGE_MOTEUR && elapsed > 2000)
    {
        // 2 s écoulées : fin du transitoire, on passe en calibration
        motor_state = CALIBRATION_MOTEUR;
        HAL_UART_Transmit(&huart2, (uint8_t*)"Calibrage...\r\n", 14, 10);
    }
    else if (motor_state == CALIBRATION_MOTEUR && elapsed > 3000)
    {
        // 3 s écoulées : le courant est stable, on le prend comme référence
        courant_fonctionnement_morteur = moyenne_courant;

        // Plancher à 0.05 A pour éviter un seuil nul si le moteur tourne à vide
        if (courant_fonctionnement_morteur < 0.05f) courant_fonctionnement_morteur = 0.05f;

        threshold   = courant_fonctionnement_morteur * 1.2f; // Marge de +20 % sur le courant nominal
        motor_state = MOTEUR_MARCHE;                         // Surveillance active

        int len = sprintf(msg, "Seuil fixé à: %.2f A\r\n", threshold);
        HAL_UART_Transmit(&huart2, (uint8_t*)msg, len, 50);
    }
    else if (motor_state == MOTEUR_MARCHE && elapsed > 20000)
    {
        // 20 s : temps maximum atteint → fin de course temporelle
        Motor_Stop();
        HAL_UART_Transmit(&huart2, (uint8_t*)"FIN DE COURSE (TEMPS)\r\n", 23, 50);
        elapsed = 0; // Remise à zéro pour le prochain cycle
    }
    else if (motor_state == MOTEUR_MARCHE)
    {
        if (moyenne_courant > threshold)
        {
            compteur_securite++; // Dépassement du seuil : incrémente le compteur de blocage

            if (compteur_securite >= 5)
            {
                // 5 dépassements consécutifs = blocage confirmé → arrêt d'urgence
                Motor_Stop();
                HAL_UART_Transmit(&huart2, (uint8_t*)"!!! BLOCAGE DETECTE - ARRET !!!\r\n", 33, 100);
                elapsed = 0;
            }
        }
        else
        {
            compteur_securite = 0; // Retour sous le seuil : on remet le compteur à zéro
        }
    }
}

/* ---------------------------------------------------------------------------
 * Motor_Periodic_Update
 * Bloc exécuté toutes les 500 ms quand le moteur tourne :
 *   1. Lit et lisse le courant
 *   2. Lance la machine à états de sécurité
 *   3. Envoie la trame de debug UART
 * Extrait du while(1) pour en alléger la lecture.
 * --------------------------------------------------------------------------- */
static void Motor_Periodic_Update(void)
{
    uint32_t current_time = HAL_GetTick();

    if (current_time - last_tick >= 500) // Cadence de 500 ms
    {
        last_tick = current_time; // Mise à jour de la référence temporelle

        float instant_current = Get_Motor_Current();          // Mesure ADC → Ampères
        moyenne_courant       = Update_Moving_Average(instant_current); // Lissage FIFO

        Motor_Security_FSM(current_time); // Machine à états : calibration + détection blocage

        UART_Send_Status(); // Envoi de la trame de debug sur la liaison série
    }
}

/* ---------------------------------------------------------------------------
 * Process_UART_Command
 * Réception non-bloquante d'un caractère UART.
 * Vérifie d'abord rx_pending : si le réveil IT a déjà capturé un octet,
 * on le traite directement sans relire l'UART (sinon il serait perdu).
 * Echo immédiat + dispatch de la commande + confirmation texte.
 * Extrait du while(1) pour en alléger la lecture.
 * --------------------------------------------------------------------------- */
static void Process_UART_Command(void)
{
    // Priorité à l'octet de réveil capturé par IT (sinon polling normal, timeout=0)
    if (!rx_pending && HAL_UART_Receive(&huart2, &rx_data, 1, 0) != HAL_OK) return;
    rx_pending = 0; // Consomme le flag IT (sans effet si on vient du polling)

    HAL_UART_Transmit(&huart2, &rx_data, 1, 10); // Echo : renvoie le caractère pour confirmation PC

    switch (rx_data)
    {
        case 'D': Motor_Forward();                       break;
        case 'A': Motor_Reverse();                       break;
        case 'S': Motor_Stop();                          break;
        case '+': Motor_SetSpeed(current_speed + 100);  break; // Incrément de vitesse
        case '-': Motor_SetSpeed(current_speed - 100);  break; // Décrément de vitesse
    }

    // Confirmation de la commande reçue avec la vitesse courante
    sprintf(msg, "\r\nCommande: %c | Vitesse: %d\r\n", rx_data, current_speed);
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
}

/* ---------------------------------------------------------------------------
 * HAL_UART_RxCpltCallback
 * Appelé par le HAL quand HAL_UART_Receive_IT a capturé son octet.
 * On lève juste un flag : le traitement se fait dans Process_UART_Command
 * pour ne pas exécuter de logique moteur depuis une interruption.
 * --------------------------------------------------------------------------- */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart2)
    {
        rx_pending = 1; // rx_data contient l'octet de réveil → Process_UART_Command le consommera
    }
}

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
    /* USER CODE BEGIN 1 */
    /* USER CODE END 1 */

    /* MCU Configuration -------------------------------------------------------*/
    HAL_Init();            // Reset périphériques, init Flash et SysTick
    /* USER CODE BEGIN Init */
    /* USER CODE END Init */

    SystemClock_Config();  // Configure l'horloge système (MSI 48 MHz)
    /* USER CODE BEGIN SysInit */
    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_TIM1_Init();
    MX_LPTIM1_Init();
    MX_ADC1_Init();
    MX_USART2_UART_Init();

    /* USER CODE BEGIN 2 */
    // Note : l'IT UART est activée dynamiquement dans Enter_Low_Power_Mode via HAL_UART_Receive_IT,
    // pas besoin de l'activer ici — on est en mode polling tant que le moteur peut tourner.

    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4); // Pré-démarre TIM1 (nécessaire avant Motor_Forward)
    __HAL_TIM_MOE_ENABLE(&htim1);             // Autorise la sortie du timer (Main Output Enable)

    if (HAL_LPTIM_PWM_Start(&hlptim1, LPTIM_CHANNEL_1) != HAL_OK) // Pré-démarre LPTIM1
    {
        Error_Handler();
    }
    /* USER CODE END 2 */

    BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI); // Bouton user en interruption externe

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        // 1. Réception et traitement des commandes UART (non bloquant)
        Process_UART_Command();

        // 2. Si le moteur tourne : lecture courant + sécurité + debug série (toutes les 500 ms)
        if (motor_state != MOTEUR_OFF)
        {
            Motor_Periodic_Update();
        }
        // 3. Si le moteur est à l'arrêt : mise en veille pour économiser l'énergie
        else
        {
            Enter_Low_Power_Mode(); // Restaure les périphériques au réveil (UART, TIM, LPTIM, ADC)
        }
    }
    /* USER CODE END WHILE */
    /* USER CODE BEGIN 3 */
    /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.MSIState            = RCC_MSI_ON;
    RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.MSIClockRange       = RCC_MSIRANGE_11;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_MSI;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) Error_Handler();
}

/**
 * @brief ADC1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_ADC1_Init(void)
{
    /* USER CODE BEGIN ADC1_Init 0 */
    /* USER CODE END ADC1_Init 0 */

    ADC_ChannelConfTypeDef sConfig = {0};

    /* USER CODE BEGIN ADC1_Init 1 */
    /* USER CODE END ADC1_Init 1 */

    hadc1.Instance                   = ADC1;
    hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV1;
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode          = ADC_SCAN_DISABLE;
    hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait      = DISABLE;
    hadc1.Init.LowPowerAutoPowerOff  = ENABLE;   // Éteint l'ADC automatiquement après chaque conversion (~1 mA économisé)
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.Overrun               = ADC_OVR_DATA_PRESERVED;
    hadc1.Init.SamplingTimeCommon1   = ADC_SAMPLETIME_160CYCLES_5;
    hadc1.Init.SamplingTimeCommon2   = ADC_SAMPLETIME_160CYCLES_5;
    hadc1.Init.OversamplingMode      = DISABLE;
    hadc1.Init.TriggerFrequencyMode  = ADC_TRIGGER_FREQ_HIGH;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) Error_Handler();

    sConfig.Channel      = ADC_CHANNEL_14;      // Broche PA7 (shunt courant moteur)
    sConfig.Rank         = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) Error_Handler();

    /* USER CODE BEGIN ADC1_Init 2 */
    // Note : HAL_ADCEx_Calibration_Run n'existe pas sur STM32G0 (calibration gérée en hardware au reset)
    /* USER CODE END ADC1_Init 2 */
}

/**
 * @brief LPTIM1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_LPTIM1_Init(void)
{
    /* USER CODE BEGIN LPTIM1_Init 0 */
    /* USER CODE END LPTIM1_Init 0 */

    LPTIM_OC_ConfigTypeDef sConfig1 = {0};

    /* USER CODE BEGIN LPTIM1_Init 1 */
    /* USER CODE END LPTIM1_Init 1 */

    hlptim1.Instance                = LPTIM1;
    hlptim1.Init.Clock.Source       = LPTIM_CLOCKSOURCE_APBCLOCK_LPOSC;
    hlptim1.Init.Clock.Prescaler    = LPTIM_PRESCALER_DIV64;  // Divise pour obtenir la fréquence PWM souhaitée
    hlptim1.Init.Trigger.Source     = LPTIM_TRIGSOURCE_SOFTWARE;
    hlptim1.Init.Period             = 999; // Période identique à TIM1 pour cohérence du rapport cyclique
    hlptim1.Init.UpdateMode         = LPTIM_UPDATE_IMMEDIATE;
    hlptim1.Init.CounterSource      = LPTIM_COUNTERSOURCE_INTERNAL;
    hlptim1.Init.Input1Source       = LPTIM_INPUT1SOURCE_GPIO;
    hlptim1.Init.Input2Source       = LPTIM_INPUT2SOURCE_GPIO;
    hlptim1.Init.RepetitionCounter  = 0;
    if (HAL_LPTIM_Init(&hlptim1) != HAL_OK) Error_Handler();

    sConfig1.Pulse      = 0;                     // Rapport cyclique initial à 0 % (moteur à l'arrêt)
    sConfig1.OCPolarity = LPTIM_OCPOLARITY_LOW;  // Polarité inversée par rapport à TIM1
    if (HAL_LPTIM_OC_ConfigChannel(&hlptim1, &sConfig1, LPTIM_CHANNEL_1) != HAL_OK) Error_Handler();

    /* USER CODE BEGIN LPTIM1_Init 2 */
    /* USER CODE END LPTIM1_Init 2 */

    HAL_LPTIM_MspPostInit(&hlptim1); // Reconnecte la broche GPIO à la sortie LPTIM1
}

/**
 * @brief TIM1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM1_Init(void)
{
    /* USER CODE BEGIN TIM1_Init 0 */
    /* USER CODE END TIM1_Init 0 */

    TIM_MasterConfigTypeDef       sMasterConfig    = {0};
    TIM_OC_InitTypeDef            sConfigOC        = {0};
    TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

    /* USER CODE BEGIN TIM1_Init 1 */
    /* USER CODE END TIM1_Init 1 */

    htim1.Instance               = TIM1;
    htim1.Init.Prescaler         = 63;    // Divise l'horloge APB pour obtenir la fréquence PWM cible
    htim1.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim1.Init.Period            = 999;   // Période = 1000 pas → rapport cyclique sur [0, 999]
    htim1.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&htim1) != HAL_OK) Error_Handler();

    sMasterConfig.MasterOutputTrigger  = TIM_TRGO_RESET;
    sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
    sMasterConfig.MasterSlaveMode      = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK) Error_Handler();

    sConfigOC.OCMode       = TIM_OCMODE_PWM1;         // Mode PWM standard (actif tant que CNT < CCR)
    sConfigOC.Pulse        = 0;                        // Rapport cyclique initial à 0 %
    sConfigOC.OCPolarity   = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode   = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState  = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_4) != HAL_OK) Error_Handler();

    // Break/Dead-time : tout désactivé (pont en H géré par les GPIOs, pas par le break hardware)
    sBreakDeadTimeConfig.OffStateRunMode  = TIM_OSSR_DISABLE;
    sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
    sBreakDeadTimeConfig.LockLevel        = TIM_LOCKLEVEL_OFF;
    sBreakDeadTimeConfig.DeadTime         = 0;
    sBreakDeadTimeConfig.BreakState       = TIM_BREAK_DISABLE;
    sBreakDeadTimeConfig.BreakPolarity    = TIM_BREAKPOLARITY_HIGH;
    sBreakDeadTimeConfig.BreakFilter      = 0;
    sBreakDeadTimeConfig.BreakAFMode      = TIM_BREAK_AFMODE_INPUT;
    sBreakDeadTimeConfig.Break2State      = TIM_BREAK2_DISABLE;
    sBreakDeadTimeConfig.Break2Polarity   = TIM_BREAK2POLARITY_HIGH;
    sBreakDeadTimeConfig.Break2Filter     = 0;
    sBreakDeadTimeConfig.Break2AFMode     = TIM_BREAK_AFMODE_INPUT;
    sBreakDeadTimeConfig.AutomaticOutput  = TIM_AUTOMATICOUTPUT_DISABLE;
    if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK) Error_Handler();

    /* USER CODE BEGIN TIM1_Init 2 */
    /* USER CODE END TIM1_Init 2 */

    HAL_TIM_MspPostInit(&htim1); // Reconnecte la broche GPIO à la sortie TIM1 CH4
}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void)
{
    /* USER CODE BEGIN USART2_Init 0 */
    /* USER CODE END USART2_Init 0 */
    /* USER CODE BEGIN USART2_Init 1 */
    /* USER CODE END USART2_Init 1 */

    huart2.Instance                    = USART2;
    huart2.Init.BaudRate               = 115200;
    huart2.Init.WordLength             = UART_WORDLENGTH_8B;
    huart2.Init.StopBits               = UART_STOPBITS_1;
    huart2.Init.Parity                 = UART_PARITY_NONE;
    huart2.Init.Mode                   = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl              = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling           = UART_OVERSAMPLING_16;
    huart2.Init.OneBitSampling         = UART_ONE_BIT_SAMPLE_DISABLE;
    huart2.Init.ClockPrescaler         = UART_PRESCALER_DIV1;
    huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    if (HAL_UART_Init(&huart2) != HAL_OK) Error_Handler();

    if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK) Error_Handler();
    if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK) Error_Handler();
    if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK) Error_Handler();

    /* USER CODE BEGIN USART2_Init 2 */
    /* USER CODE END USART2_Init 2 */
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* USER CODE BEGIN MX_GPIO_Init_1 */
    /* USER CODE END MX_GPIO_Init_1 */

    // Activation des horloges GPIO (obligatoire avant tout accès aux registres)
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    // I2C1 en mode Open-Drain avec alternate function (non utilisé dans ce projet mais configuré par CubeMX)
    GPIO_InitStruct.Pin       = I2C1_SCL_Pin | I2C1_SDA_Pin;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* USER CODE BEGIN MX_GPIO_Init_2 */
    /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* ---------------------------------------------------------------------------
 * Clock_SwitchToSleep
 * Réduit l'horloge MSI de 48 MHz → 4 MHz avant la mise en veille.
 * Séquence obligatoire : fréquence d'abord, tension ensuite.
 *   - À 4 MHz, 0 wait state flash suffit (LATENCY_0)
 *   - SCALE2 (Vcore 1.0V) autorisé seulement sous 26 MHz
 *   - HAL_UART_Init relit la fréquence PCLK et recalcule le BRR → 115200 baud reste valide
 * --------------------------------------------------------------------------- */
static void Clock_SwitchToSleep(void)
{
    // 1. Flash en 0 wait state : valide jusqu'à ~16 MHz selon Vcore
    __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_0);

    // 2. Descente du MSI à 4 MHz (MSIRANGE_6)
    RCC_OscInitTypeDef osc = {0};
    osc.OscillatorType      = RCC_OSCILLATORTYPE_MSI;
    osc.MSIState            = RCC_MSI_ON;
    osc.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    osc.MSIClockRange       = RCC_MSIRANGE_6; // 4 MHz
    osc.PLL.PLLState        = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&osc);

    // 3. Passage en SCALE2 (Vcore 1.0V) : seulement APRÈS la réduction de fréquence
    //    Inverser l'ordre risque une instabilité CPU à haute fréquence sous-alimenté
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE2);

    // 4. Recalcule le BRR UART pour 115200 baud @ 4 MHz PCLK
    //    HAL_UART_Init lit HAL_RCC_GetPCLK1Freq() automatiquement → pas de magic number
    HAL_UART_Init(&huart2);
}

/* ---------------------------------------------------------------------------
 * Clock_SwitchToFullSpeed
 * Restaure MSI à 48 MHz et Vcore en SCALE1 après le réveil.
 * Séquence obligatoire : tension d'abord, fréquence ensuite.
 * --------------------------------------------------------------------------- */
static void Clock_SwitchToFullSpeed(void)
{
    // 1. Remontée en SCALE1 (Vcore 1.2V) obligatoire AVANT de dépasser 26 MHz
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

    // 2. Remontée du MSI à 48 MHz (MSIRANGE_11)
    RCC_OscInitTypeDef osc = {0};
    osc.OscillatorType      = RCC_OSCILLATORTYPE_MSI;
    osc.MSIState            = RCC_MSI_ON;
    osc.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    osc.MSIClockRange       = RCC_MSIRANGE_11; // 48 MHz
    osc.PLL.PLLState        = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&osc);

    // 3. Restore la latence flash pour 48 MHz (même valeur que SystemClock_Config)
    __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_1);

    // 4. Recalcule le BRR UART pour 115200 baud @ 48 MHz PCLK
    HAL_UART_Init(&huart2);
}

/* ---------------------------------------------------------------------------
 * Enter_Low_Power_Mode
 * Séquence complète de mise en veille et restauration :
 *   1. Purge UART → évite un réveil immédiat sur flag résiduel
 *   2. Pins inutiles en ANALOG → élimine les courants de fuite GPIO
 *   3. Horloges périphériques coupées (Clock Gating)
 *   4. SysTick suspendu → sinon réveil toutes les 1 ms
 *   5. WFI → CPU dort jusqu'à un caractère UART
 *   ---- RÉVEIL ICI ----
 *   6. Horloges et périphériques restaurés
 *   7. Retour en mode polling normal
 * --------------------------------------------------------------------------- */
void Enter_Low_Power_Mode(void)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)"Entree en veille...\r\n", 21, 50);
    HAL_Delay(10); // Laisse le temps à l'UART de finir d'envoyer avant de couper

    /* 1. Purge UART : évite un réveil immédiat sur un flag de réception résiduel */
    __HAL_UART_FLUSH_DRREGISTER(&huart2);
    __HAL_UART_CLEAR_FLAG(&huart2, UART_CLEAR_OREF | UART_CLEAR_NEF | UART_CLEAR_FEF | UART_CLEAR_PEF);
    volatile uint32_t tmpreg = huart2.Instance->RDR; // Vide le registre de données
    (void)tmpreg;                                     // Supprime le warning "variable non utilisée"

    /* 2. Pins inutiles en ANALOG → supprime les courants de fuite entre VDD et GND */
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;

    // PORT A : on préserve UART (PA2, PA3) et SWD (PA13, PA14) — tout le reste en analogique
    GPIO_InitStruct.Pin = GPIO_PIN_ALL & ~(GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_13 | GPIO_PIN_14);
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    // PORT B & C : tout en analogique (les pins moteur et autres sont inutiles en veille)
    GPIO_InitStruct.Pin = GPIO_PIN_ALL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* 3. Coupe les horloges périphériques non nécessaires en veille */
    __HAL_RCC_TIM1_CLK_DISABLE();
    __HAL_RCC_LPTIM1_CLK_DISABLE();
    __HAL_RCC_ADC_CLK_DISABLE();

    /* 4. Réduction clock + tension EN PREMIER : MSI 48 MHz → 4 MHz, SCALE1 → SCALE2
     *    HAL_UART_Init() à l'intérieur recalcule le BRR pour 115200 baud @ 4 MHz.
     *    DOIT être fait AVANT HAL_UART_Receive_IT, car HAL_UART_Init réinitialise
     *    complètement l'UART et efface RXNEIE — ce qui annulerait l'IT de réveil. */
    Clock_SwitchToSleep();

    /* 5. Setup de la réception IT : APRÈS Clock_SwitchToSleep pour ne pas être annulé.
     *    HAL stocke l'octet dans rx_data et appelle HAL_UART_RxCpltCallback → rx_pending = 1. */
    HAL_UART_Receive_IT(&huart2, &rx_data, 1);

    /* 6. Suspend le SysTick : sans ça, le CPU se réveillerait toutes les 1 ms */
    HAL_SuspendTick();

    // Le CPU s'arrête ICI jusqu'à réception d'un caractère sur UART2
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);

    /* =========================================================
     *  --- LE MICROCONTROLEUR SE REVEILLE ICI ---
     * ========================================================= */

    HAL_ResumeTick(); // Relance le SysTick (HAL_GetTick() redevient fiable)

    /* 7. Restaure clock + tension : 4 MHz → 48 MHz, SCALE2 → SCALE1
     *    Doit se faire AVANT de relancer TIM1/LPTIM1 qui ont été configurés pour 48 MHz */
    Clock_SwitchToFullSpeed();

    /* 8. Réactive les horloges périphériques */
    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_LPTIM1_CLK_ENABLE();
    __HAL_RCC_ADC_CLK_ENABLE();

    /* 9. Restaure les GPIOs et les périphériques dans leur état de fonctionnement normal */
    MX_GPIO_Init();    // Remet les GPIOs en mode AF/Analogique selon leur fonction
    MX_TIM1_Init();    // Reconfigure TIM1 (perdu pendant la veille)
    MX_LPTIM1_Init();  // Reconfigure LPTIM1
    MX_ADC1_Init();    // Reconfigure l'ADC

    // Relance les PWM de base (nécessaire avant qu'une commande moteur puisse fonctionner)
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
    __HAL_TIM_MOE_ENABLE(&htim1); // Réactive la sortie principale de TIM1

    /* 10. HAL_UART_Receive_IT désactive l'IT automatiquement après réception —
     *    rien à faire ici, on repasse en polling via Process_UART_Command. */

    HAL_UART_Transmit(&huart2, (uint8_t*)"Reveil OK !\r\n", 13, 50);
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    __disable_irq(); // Coupe toutes les interruptions pour figer le système en erreur
    while (1) {}
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
