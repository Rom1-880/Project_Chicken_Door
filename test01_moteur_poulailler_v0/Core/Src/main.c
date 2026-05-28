/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 */
/* USER CODE END Header */

#include "main.h"

/* USER CODE BEGIN PD */
// --- Configuration des broches Moteur ---
#define MOTEUR_NSLEEP_PORT  GPIOA
#define MOTEUR_NSLEEP_PIN   GPIO_PIN_9
#define MOTEUR_AVANT_PORT   GPIOB
#define MOTEUR_AVANT_PIN    GPIO_PIN_1
#define MOTEUR_ARRIERE_PORT GPIOA
#define MOTEUR_ARRIERE_PIN  GPIO_PIN_10

// --- Configuration des broches LED ---
#define LED_G_1_PORT        GPIOB
#define LED_G_1_PIN         GPIO_PIN_7
#define LED_G_2_PORT        GPIOB
#define LED_G_2_PIN         GPIO_PIN_3
#define LED_D_3_PORT        GPIOB
#define LED_D_3_PIN         GPIO_PIN_8  // ⚠ Vérifier conflit avec I2C1_SCL si I2C utilisé
#define LED_D_4_PORT        GPIOB
#define LED_D_4_PIN         GPIO_PIN_9  // ⚠ Vérifier conflit avec I2C1_SDA si I2C utilisé
/* USER CODE END PD */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef  hadc1;
LPTIM_HandleTypeDef hlptim1;
RTC_HandleTypeDef  hrtc;
TIM_HandleTypeDef  htim1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

typedef enum {
    MOTEUR_ARRET = 0,
    MOTEUR_DROITE,
    MOTEUR_GAUCHE
} Motor_State_t;

/* --- Vitesse et commandes ------------------------------------------------- */
uint8_t  speed_level  = 3;                          // Niveau actif parmi 4 (1=lent, 4=rapide)
const uint16_t speed_table[4] = {700, 800, 900, 999}; // Correspondance niveau → valeur timer
uint16_t current_speed = 900;  // Valeur par défaut = niveau 3 (index 2 du tableau)
uint8_t  rx_data;              // Octet reçu sur l'UART (polling ou IT)
uint8_t  rx_pending   = 0;    // Flag : 1 = rx_data contient l'octet de réveil (capturé par IT)
char     msg[100];             // Buffer partagé pour sprintf + envoi UART

/* --- Mesure du courant moteur --------------------------------------------- */
uint32_t adc_value      = 0;    // Valeur brute 12 bits retournée par l'ADC
float    courant_moteur = 0.0f; // Courant instantané calculé (en Ampères)
uint32_t last_tick      = 0;    // Référence temporelle pour l'envoi périodique

/* --- Moyenne glissante sur 5 échantillons --------------------------------- */
float lecture[5]      = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}; // Fenêtre FIFO
float somme_courant   = 0.0f; // Somme recalculée à chaque échantillon
float moyenne_courant = 0.0f; // Résultat exposé au reste du programme

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
uint32_t     elapsed     = 0;          // Durée écoulée depuis le démarrage (ms)


uint32_t led_timer = 0;       // Sauvegarde le moment du changement de vitesse
uint8_t  led_active = 0;      // Flag : 1 = les LEDs sont actuellement allumées

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
static void MX_LPTIM1_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_RTC_Init(void);
/* USER CODE BEGIN PFP */
void Enter_Low_Power_Mode(void);
void Controler_Poulailler(Motor_State_t etat);
/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* ---------------------------------------------------------------------------
 * PWM_StopAll
 * Arrête les deux PWM (TIM1 et LPTIM1) avant tout changement de direction.
 * --------------------------------------------------------------------------- */
static void PWM_StopAll(void)
{
    HAL_LPTIM_PWM_Stop(&hlptim1, LPTIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);
}

/* ---------------------------------------------------------------------------
 * Motor_StartTimer
 * Réinitialise le compteur de temps et le compteur de sécurité au démarrage.
 * --------------------------------------------------------------------------- */
static void Motor_StartTimer(void)
{
    motor_start_time  = HAL_GetTick();
    motor_state       = DEMARRAGE_MOTEUR;
    compteur_securite = 0;
    elapsed           = 0;
}

/* ---------------------------------------------------------------------------
 * Motor_Forward — marche avant via TIM1 CH4
 * --------------------------------------------------------------------------- */
void Motor_Forward(void)
{
    PWM_StopAll();
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2,  GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
    HAL_Delay(5); // Anti-shoot-through

    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, current_speed);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
    Motor_StartTimer();

}

/* ---------------------------------------------------------------------------
 * Motor_Reverse — marche arrière via LPTIM1 CH1
 * --------------------------------------------------------------------------- */
void Motor_Reverse(void)
{
    PWM_StopAll();
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2,  GPIO_PIN_RESET);
    HAL_Delay(5); // Anti-shoot-through

    HAL_LPTIM_PWM_Start(&hlptim1, LPTIM_CHANNEL_1);
    __HAL_LPTIM_COMPARE_SET(&hlptim1, LPTIM_CHANNEL_1, current_speed);
    Motor_StartTimer();

}

/* ---------------------------------------------------------------------------
 * Motor_Stop — arrêt immédiat
 * --------------------------------------------------------------------------- */
void Motor_Stop(void)
{
    PWM_StopAll();
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2,  GPIO_PIN_RESET);
    motor_state = MOTEUR_OFF;
    threshold   = 1.0f; // Reset seuil pour le prochain démarrage

}

/* ---------------------------------------------------------------------------
 * Motor_SetSpeed — applique une vitesse avec clamp [700, 999]
 * --------------------------------------------------------------------------- */
void Motor_SetSpeed(uint16_t speed)
{
    if (speed > 999)              speed = 999;
    if (speed > 0 && speed < 700) speed = 700;
    current_speed = speed;

    // --- Sécurité : On applique la PWM UNIQUEMENT au périphérique actif ---
    if (motor_state != MOTEUR_OFF)
    {
        // Si on est en marche avant, on met à jour TIM1
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, current_speed);

        // Si on est en marche arrière, on met à jour LPTIM1
        __HAL_LPTIM_COMPARE_SET(&hlptim1, LPTIM_CHANNEL_1, current_speed);
    }

    // --- Allumage des LEDs selon le nouveau niveau de vitesse ---
    HAL_GPIO_WritePin(LED_G_1_PORT, LED_G_1_PIN, (speed_level >= 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_G_2_PORT, LED_G_2_PIN, (speed_level >= 2) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_D_3_PORT, LED_D_3_PIN, (speed_level >= 3) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_D_4_PORT, LED_D_4_PIN, (speed_level >= 4) ? GPIO_PIN_SET : GPIO_PIN_RESET);

    // --- Activation du timer (affichage pendant 2000 ms) ---
    led_timer = HAL_GetTick();
    led_active = 1;
}

/* ---------------------------------------------------------------------------
 * Get_Motor_Current — lecture ADC → Ampères
 * --------------------------------------------------------------------------- */
float Get_Motor_Current(void)
{
    float res_ohm = 1.3f;
    float offset  = 0.062f;

    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
    {
        adc_value      = HAL_ADC_GetValue(&hadc1);
        courant_moteur = (((adc_value * 3.3f) / 4095.0f) / res_ohm) * 1.103f;
        courant_moteur -= offset;
        if (courant_moteur < 0) courant_moteur = 0;
    }
    HAL_ADC_Stop(&hadc1);
    return courant_moteur;
}

/* ---------------------------------------------------------------------------
 * Update_Moving_Average — fenêtre FIFO de 5 échantillons
 * --------------------------------------------------------------------------- */
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

/* ---------------------------------------------------------------------------
 * UART_Send_Status — trame de debug périodique
 * --------------------------------------------------------------------------- */
static void UART_Send_Status(void)
{
    int len = sprintf(msg,
        "I:%.3fA | courant_fonctionnement:%.3fA \r\n"
        "|threshold:%.3fA|counter :%d |Etat :%d \r\n"
        " TIMER:%lu ms | Niveau Vitesse: %d\r\n",
        moyenne_courant, courant_fonctionnement_morteur,
        threshold, compteur_securite, (int)motor_state, elapsed, speed_level);
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, len, 50);
}

/* ---------------------------------------------------------------------------
 * Motor_Security_FSM — machine à états de surveillance courant
 * --------------------------------------------------------------------------- */
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
        threshold   = courant_fonctionnement_morteur * 1.2f;
        motor_state = MOTEUR_MARCHE;
        int len = sprintf(msg, "Seuil fixé à: %.2f A\r\n", threshold);
        HAL_UART_Transmit(&huart2, (uint8_t*)msg, len, 50);
    }
    else if (motor_state == MOTEUR_MARCHE && elapsed > 20000)
    {
        Motor_Stop();
        HAL_UART_Transmit(&huart2, (uint8_t*)"FIN DE COURSE (TEMPS)\r\n", 23, 50);
        elapsed = 0;
    }
    else if (motor_state == MOTEUR_MARCHE)
    {
        if (moyenne_courant > threshold)
        {
            compteur_securite++;
            if (compteur_securite >= 5)
            {
                Motor_Stop();
                HAL_UART_Transmit(&huart2, (uint8_t*)"!!! BLOCAGE DETECTE - ARRET !!!\r\n", 33, 100);
                elapsed = 0;
            }
        }
        else { compteur_securite = 0; }
    }
}

/* ---------------------------------------------------------------------------
 * Motor_Periodic_Update — exécuté toutes les 500 ms si moteur actif
 * --------------------------------------------------------------------------- */
static void Motor_Periodic_Update(void)
{
    uint32_t current_time = HAL_GetTick();
    if (current_time - last_tick >= 500)
    {
        last_tick       = current_time;
        float instant   = Get_Motor_Current();
        moyenne_courant = Update_Moving_Average(instant);
        Motor_Security_FSM(current_time);
        UART_Send_Status();
    }
}

/* ---------------------------------------------------------------------------
 * Process_UART_Command — réception non-bloquante + dispatch commandes
 * --------------------------------------------------------------------------- */
static void Process_UART_Command(void)
{
    if (!rx_pending && HAL_UART_Receive(&huart2, &rx_data, 1, 0) != HAL_OK) return;
    rx_pending = 0;

    HAL_UART_Transmit(&huart2, &rx_data, 1, 10); // Echo

    switch (rx_data)
    {
        case 'D': Motor_Forward();                      break;
        case 'A': Motor_Reverse();                      break;
        case 'S': Motor_Stop();                         break;
        case '+':
            if (speed_level < 4)
            {
                speed_level++;
                current_speed = speed_table[speed_level - 1]; // Conversion niveau (1-4) -> index (0-3)
                Motor_SetSpeed(current_speed);
            }
            break;
        case '-':
            if (speed_level > 1)
            {
                speed_level--;
                current_speed = speed_table[speed_level - 1]; // Conversion niveau (1-4) -> index (0-3)
                Motor_SetSpeed(current_speed);
            }
            break;
    }

    sprintf(msg, "\r\nCommande: %c | Niveau Vitesse: %d | Valeur PWM: %d\r\n", rx_data, speed_level, current_speed);
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
}

/* ---------------------------------------------------------------------------
 * HAL_UART_RxCpltCallback — capturé depuis l'IT, flag seulement
 * --------------------------------------------------------------------------- */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == &huart2) rx_pending = 1;
}

/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_TIM1_Init();
  MX_LPTIM1_Init();
  MX_ADC1_Init();
  MX_USART2_UART_Init();
  MX_RTC_Init();

  /* USER CODE BEGIN 2 */

  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
  __HAL_TIM_MOE_ENABLE(&htim1);

  if (HAL_LPTIM_PWM_Start(&hlptim1, LPTIM_CHANNEL_1) != HAL_OK) Error_Handler();
  /* USER CODE END 2 */

  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* USER CODE BEGIN WHILE */
  while (1)
    {
        // 1. Gestion des commandes de la liaison série
        Process_UART_Command();

        // 2. Extinction automatique des LEDs après 2 secondes (si actives)
        Update_LED_Timeout();

        // 3. Gestion de la veille et du moteur
        // On RESTE éveillé si le moteur tourne OU si les LEDs sont encore actives
        if ((motor_state != MOTEUR_OFF) || (led_active == 1))
        {
            // Si le moteur tourne, on met à jour le courant, sinon on attend juste la fin des LEDs
            if (motor_state != MOTEUR_OFF)
            {
                Motor_Periodic_Update();
            }
        }
        else
        {
            // On entre en veille UNIQUEMENT si le moteur est OFF et les LEDs éteintes
            Enter_Low_Power_Mode();
        }
    }
  /* USER CODE END WHILE */
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_LSE | RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState            = RCC_LSE_ON;
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

static void MX_ADC1_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};
  hadc1.Instance                   = ADC1;
  hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV1;
  hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode          = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait      = DISABLE;
  hadc1.Init.LowPowerAutoPowerOff  = ENABLE;
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

  sConfig.Channel      = ADC_CHANNEL_14;
  sConfig.Rank         = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) Error_Handler();
}

static void MX_LPTIM1_Init(void)
{
  LPTIM_OC_ConfigTypeDef sConfig1 = {0};
  hlptim1.Instance               = LPTIM1;
  hlptim1.Init.Clock.Source      = LPTIM_CLOCKSOURCE_APBCLOCK_LPOSC;
  hlptim1.Init.Clock.Prescaler   = LPTIM_PRESCALER_DIV64;
  hlptim1.Init.Trigger.Source    = LPTIM_TRIGSOURCE_SOFTWARE;
  hlptim1.Init.Period            = 999;
  hlptim1.Init.UpdateMode        = LPTIM_UPDATE_IMMEDIATE;
  hlptim1.Init.CounterSource     = LPTIM_COUNTERSOURCE_INTERNAL;
  hlptim1.Init.Input1Source      = LPTIM_INPUT1SOURCE_GPIO;
  hlptim1.Init.Input2Source      = LPTIM_INPUT2SOURCE_GPIO;
  hlptim1.Init.RepetitionCounter = 0;
  if (HAL_LPTIM_Init(&hlptim1) != HAL_OK) Error_Handler();

  sConfig1.Pulse      = 0;
  sConfig1.OCPolarity = LPTIM_OCPOLARITY_LOW;
  if (HAL_LPTIM_OC_ConfigChannel(&hlptim1, &sConfig1, LPTIM_CHANNEL_1) != HAL_OK) Error_Handler();
  HAL_LPTIM_MspPostInit(&hlptim1);
}

static void MX_RTC_Init(void)
{
  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  hrtc.Instance            = RTC;
  hrtc.Init.HourFormat     = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv   = 127;
  hrtc.Init.SynchPrediv    = 255;
  hrtc.Init.OutPut         = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutRemap    = RTC_OUTPUT_REMAP_NONE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType     = RTC_OUTPUT_TYPE_OPENDRAIN;
  hrtc.Init.OutPutPullUp   = RTC_OUTPUT_PULLUP_NONE;
  hrtc.Init.BinMode        = RTC_BINARY_NONE;
  if (HAL_RTC_Init(&hrtc) != HAL_OK) Error_Handler();

  sTime.Hours = 0x0; sTime.Minutes = 0x0; sTime.Seconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK) Error_Handler();

  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month   = RTC_MONTH_JANUARY;
  sDate.Date    = 0x1;
  sDate.Year    = 0x0;
  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK) Error_Handler();
}

static void MX_TIM1_Init(void)
{
  TIM_MasterConfigTypeDef        sMasterConfig    = {0};
  TIM_OC_InitTypeDef             sConfigOC        = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  htim1.Instance               = TIM1;
  htim1.Init.Prescaler         = 63;
  htim1.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim1.Init.Period            = 999;
  htim1.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK) Error_Handler();

  sMasterConfig.MasterOutputTrigger  = TIM_TRGO_RESET;
  sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
  sMasterConfig.MasterSlaveMode      = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK) Error_Handler();

  sConfigOC.OCMode       = TIM_OCMODE_PWM1;
  sConfigOC.Pulse        = 0;
  sConfigOC.OCPolarity   = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode   = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState  = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_4) != HAL_OK) Error_Handler();

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
  HAL_TIM_MspPostInit(&htim1);
}

static void MX_USART2_UART_Init(void)
{
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
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  GPIO_InitStruct.Pin       = I2C1_SCL_Pin | I2C1_SDA_Pin;
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull      = GPIO_NOPULL;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */

  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Pin   = MOTEUR_NSLEEP_PIN | MOTEUR_ARRIERE_PIN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = MOTEUR_AVANT_PIN;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = LED_G_1_PIN | LED_G_2_PIN | LED_D_3_PIN | LED_D_4_PIN;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  HAL_GPIO_WritePin(GPIOA, MOTEUR_NSLEEP_PIN | MOTEUR_ARRIERE_PIN, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, MOTEUR_AVANT_PIN | LED_G_1_PIN | LED_G_2_PIN
                          | LED_D_3_PIN | LED_D_4_PIN, GPIO_PIN_RESET);

/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* ---------------- -----------------------------------------------------------
 * Update_LED_Timeout — Éteint les LEDs après 2 secondes d'affichage
 * --------------------------------------------------------------------------- */
void Update_LED_Timeout(void)
{
    if (led_active && (HAL_GetTick() - led_timer >= 1500))
    {
        HAL_GPIO_WritePin(LED_G_1_PORT, LED_G_1_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_G_2_PORT, LED_G_2_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_D_3_PORT, LED_D_3_PIN, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED_D_4_PORT, LED_D_4_PIN, GPIO_PIN_RESET);
        led_active = 0; // On désactive le flag
    }
}


/* ---------------------------------------------------------------------------
 * Controler_Poulailler
 * Gère le nombre de LEDs allumées de gauche à droite selon speed_level
 * et pilote les pins de direction du pont en H.
 * --------------------------------------------------------------------------- */


static void Clock_SwitchToSleep(void)
{
    __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_0);

    RCC_OscInitTypeDef osc      = {0};
    osc.OscillatorType          = RCC_OSCILLATORTYPE_MSI;
    osc.MSIState                = RCC_MSI_ON;
    osc.MSICalibrationValue     = RCC_MSICALIBRATION_DEFAULT;
    osc.MSIClockRange           = RCC_MSIRANGE_6;
    osc.PLL.PLLState            = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&osc);

    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE2);

    HAL_UART_Init(&huart2);
}

static void Clock_SwitchToFullSpeed(void)
{
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitTypeDef osc  = {0};
    osc.OscillatorType      = RCC_OSCILLATORTYPE_MSI;
    osc.MSIState            = RCC_MSI_ON;
    osc.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    osc.MSIClockRange       = RCC_MSIRANGE_11;
    osc.PLL.PLLState        = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&osc);

    __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_1);

    HAL_UART_Init(&huart2);
}

void Enter_Low_Power_Mode(void)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)"Entree en veille...\r\n", 21, 50);
    HAL_Delay(10);

    __HAL_UART_FLUSH_DRREGISTER(&huart2);
    __HAL_UART_CLEAR_FLAG(&huart2, UART_CLEAR_OREF | UART_CLEAR_NEF | UART_CLEAR_FEF | UART_CLEAR_PEF);
    volatile uint32_t tmpreg = huart2.Instance->RDR;
    (void)tmpreg;

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;

    GPIO_InitStruct.Pin = GPIO_PIN_ALL & ~(GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_13 | GPIO_PIN_14);
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_ALL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    __HAL_RCC_TIM1_CLK_DISABLE();
    __HAL_RCC_LPTIM1_CLK_DISABLE();
    __HAL_RCC_ADC_CLK_DISABLE();

    Clock_SwitchToSleep();

    HAL_UART_Receive_IT(&huart2, &rx_data, 1);

    HAL_SuspendTick();

    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);

    /* ======= RÉVEIL ICI ======= */

    HAL_ResumeTick();

    Clock_SwitchToFullSpeed();

    __HAL_RCC_TIM1_CLK_ENABLE();
    __HAL_RCC_LPTIM1_CLK_ENABLE();
    __HAL_RCC_ADC_CLK_ENABLE();

    MX_GPIO_Init();
    MX_TIM1_Init();
    MX_LPTIM1_Init();
    MX_ADC1_Init();

    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
    __HAL_TIM_MOE_ENABLE(&htim1);

    HAL_UART_Transmit(&huart2, (uint8_t*)"Reveil OK !\r\n", 13, 50);
}

/* USER CODE END 4 */

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {}
#endif
