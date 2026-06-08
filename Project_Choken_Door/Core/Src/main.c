/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  *
  * Ce fichier est le point d'entrée du programme fusionné.
  * Toute la logique moteur (FSM, courant, LEDs, UART) est déléguée à moteur.c.
  * Ce main.c ne contient que :
  *   - L'initialisation des périphériques (MX_*)
  *   - La configuration de l'horloge
  *   - La boucle principale while(1)
  *   - Les fonctions Clock_SwitchToSleep / Clock_SwitchToFullSpeed
  *     (appelées par Enter_Low_Power_Mode dans moteur.c)
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include "moteur.h"
#include <stdio.h>
#include <string.h>
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
ADC_HandleTypeDef   hadc1;
LPTIM_HandleTypeDef hlptim1;
RTC_HandleTypeDef   hrtc;
TIM_HandleTypeDef   htim1;
UART_HandleTypeDef  huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_LPTIM1_Init(void);
static void MX_TIM1_Init(void);
static void MX_RTC_Init(void);
static void MX_USART2_UART_Init(void);

/* USER CODE BEGIN PFP */
void Clock_SwitchToSleep(void);
void Clock_SwitchToFullSpeed(void);
static void UART_Send_Welcome_Msg(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* ---------------------------------------------------------------------------
 * UART_Send_Welcome_Msg — Logo ASCII d'accueil envoyé une seule fois au boot
 * --------------------------------------------------------------------------- */
static void UART_Send_Welcome_Msg(void)
{
    HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n", 2, 10);
    HAL_Delay(20);

    static const char logo[] =
            "                              ######\r\n"
            "                            ########     ##\r\n"
            "                            #########   ######\r\n"
            "                             #########  ########\r\n"
            "                   ########  ##########  ########\r\n"
            "                  ################################\r\n"
            "                  ################################\r\n"
            "                   ###############################\r\n"
            "                    ##########         ##########\r\n"
            "                     #######             #######\r\n"
            "                      #####         @@@   ######\r\n"
            "     ###########        #         @@@@@@@  #########\r\n"
            "   #######    #### #####           @@@@@    #########   #####\r\n"
            "  ######      ##########                    ########### ######\r\n"
            " ######      #### ######                    #################\r\n"
            " ######           ##### #####      ######     #######   #####    #####    #######   ###### ###### #####\r\n"
            "######           #############    #####    #####      #####   ####     ###### ###  ############\r\n"
            "######           ######  ######   ######  ######       ##### ####      ######  ### ######   ######\r\n"
            "######          ######   ######   ###### ######       ###########   ######   ### ######   ######\r\n"
            "######          ######   #####   ######  #####        ######  #####  #####  ###   #####    #####\r\n"
            "######          #####   ######   ######  #####       ######    ##### #########    ######   ######\r\n"
            "######         ######   ######   #####   #####       ######   ###### ######      #######   #####\r\n"
            "#######     #########   ######  ####### #######   ########    ###### ######    ########    #####\r\n"
            "  ##################    ################# ################    ################# ######    #######\r\n"
            "     #####    #####        ##       ##      ####    #####        ##      ###     #####       ###\r\n"
            "\r\n\0";

    HAL_UART_Transmit(&huart2, (uint8_t*)logo, sizeof(logo) - 1, 1500);
    HAL_Delay(100);

    static const char bloc_info[] =
        "\r\n+------------------------------------------------+\r\n"
        "|     SYSTEME DE CONTROLE MOTEUR INITIALISE      |\r\n"
        "|            PRET A RECEVOIR LES ORDRES          |\r\n"
        "+------------------------------------------------+\r\n\r\n\0";

    HAL_UART_Transmit(&huart2, (uint8_t*)bloc_info, sizeof(bloc_info) - 1, 200);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
    /* MCU Configuration -------------------------------------------------------*/
    HAL_Init();

    /* USER CODE BEGIN Init */
    /* USER CODE END Init */

    SystemClock_Config();

    /* USER CODE BEGIN SysInit */
    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_ADC1_Init();
    MX_LPTIM1_Init();
    MX_TIM1_Init();
    MX_RTC_Init();
    MX_USART2_UART_Init();

    /* USER CODE BEGIN 2 */

    // Pré-démarre TIM1 (requis avant Motor_Forward)
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
    __HAL_TIM_MOE_ENABLE(&htim1); // Main Output Enable : active la sortie TIM1

    // Pré-démarre LPTIM1 (requis avant Motor_Reverse)
    if (HAL_LPTIM_PWM_Start(&hlptim1, LPTIM_CHANNEL_1) != HAL_OK) Error_Handler();

    // Message d'accueil
    UART_Send_Welcome_Msg();

    // Arme l'interruption UART dès le démarrage : sans ça, les commandes ne
    // fonctionnent qu'après le premier passage en veille (qui l'arme lui-même).
    HAL_UART_Receive_IT(&huart2, &rx_data, 1);

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */

        // 1. Traitement des commandes UART (IT + polling de secours)
        Process_UART_Command();

        // 2. Timeout LEDs vitesse (seulement hors état d'erreur)
        if (motor_state != MOTEUR_ERREUR_ABSENCE && motor_state != MOTEUR_ERREUR_BLOCAGE)
        {
            Update_LED_Timeout();
        }

        // 3. Clignotement non-bloquant des LEDs en cas d'erreur
        Gerer_Erreur_Moteur();

        // 4. Moteur en marche : surveillance courant + FSM sécurité
        if (motor_state != MOTEUR_OFF
         && motor_state != MOTEUR_ERREUR_ABSENCE
         && motor_state != MOTEUR_ERREUR_BLOCAGE)
        {
            Motor_Periodic_Update();
        }

        // 5. Moteur à l'arrêt + LEDs éteintes → entrée en veille basse consommation
        else if (motor_state == MOTEUR_OFF && led_active == 0)
        {
          // Enter_Low_Power_Mode();
        }
    }
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
    HAL_PWR_EnableBkUpAccess();
    __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_LSE | RCC_OSCILLATORTYPE_MSI;
    RCC_OscInitStruct.LSEState            = RCC_LSE_ON;
    RCC_OscInitStruct.MSIState            = RCC_MSI_ON;
    RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.MSIClockRange       = RCC_MSIRANGE_11; // 48 MHz
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
    hadc1.Init.LowPowerAutoPowerOff  = ENABLE;  // Éteint l'ADC entre deux conversions (~1 mA économisé)
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.Overrun               = ADC_OVR_DATA_PRESERVED;
    hadc1.Init.SamplingTimeCommon1   = ADC_SAMPLETIME_160CYCLES_5; // Temps d'échantillonnage long : meilleure précision courant
    hadc1.Init.SamplingTimeCommon2   = ADC_SAMPLETIME_160CYCLES_5;
    hadc1.Init.OversamplingMode      = DISABLE;
    hadc1.Init.TriggerFrequencyMode  = ADC_TRIGGER_FREQ_HIGH;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) Error_Handler();

    sConfig.Channel      = ADC_CHANNEL_14; // PA7 : shunt de mesure de courant
    sConfig.Rank         = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) Error_Handler();

    /* USER CODE BEGIN ADC1_Init 2 */
    /* USER CODE END ADC1_Init 2 */
}

/**
  * @brief LPTIM1 Initialization Function
  * @retval None
  */
static void MX_LPTIM1_Init(void)
{
    /* USER CODE BEGIN LPTIM1_Init 0 */
    /* USER CODE END LPTIM1_Init 0 */

    LPTIM_OC_ConfigTypeDef sConfig1 = {0};

    /* USER CODE BEGIN LPTIM1_Init 1 */
    /* USER CODE END LPTIM1_Init 1 */

    hlptim1.Instance               = LPTIM1;
    hlptim1.Init.Clock.Source      = LPTIM_CLOCKSOURCE_APBCLOCK_LPOSC;
    hlptim1.Init.Clock.Prescaler   = LPTIM_PRESCALER_DIV64; // Fréquence PWM cohérente avec TIM1
    hlptim1.Init.Trigger.Source    = LPTIM_TRIGSOURCE_SOFTWARE;
    hlptim1.Init.Period            = 999; // Même période que TIM1
    hlptim1.Init.UpdateMode        = LPTIM_UPDATE_IMMEDIATE;
    hlptim1.Init.CounterSource     = LPTIM_COUNTERSOURCE_INTERNAL;
    hlptim1.Init.Input1Source      = LPTIM_INPUT1SOURCE_GPIO;
    hlptim1.Init.Input2Source      = LPTIM_INPUT2SOURCE_GPIO;
    hlptim1.Init.RepetitionCounter = 0;
    if (HAL_LPTIM_Init(&hlptim1) != HAL_OK) Error_Handler();

    sConfig1.Pulse      = 0;
    sConfig1.OCPolarity = LPTIM_OCPOLARITY_LOW; // ⚠ Polarité inversée vs TIM1 (câblage pont en H)
    if (HAL_LPTIM_OC_ConfigChannel(&hlptim1, &sConfig1, LPTIM_CHANNEL_1) != HAL_OK) Error_Handler();

    /* USER CODE BEGIN LPTIM1_Init 2 */
    /* USER CODE END LPTIM1_Init 2 */

    HAL_LPTIM_MspPostInit(&hlptim1); // Reconnecte la broche GPIO à la sortie LPTIM1
}

/**
  * @brief RTC Initialization Function
  * @retval None
  */
static void MX_RTC_Init(void)
{
    /* USER CODE BEGIN RTC_Init 0 */
    /* USER CODE END RTC_Init 0 */

    /* USER CODE BEGIN RTC_Init 1 */
    /* USER CODE END RTC_Init 1 */

    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    hrtc.Instance            = RTC;
    hrtc.Init.HourFormat     = RTC_HOURFORMAT_24;
    hrtc.Init.AsynchPrediv   = 127;  // (127+1) × (255+1) = 32768 Hz (LSE)
    hrtc.Init.SynchPrediv    = 255;  // → 1 Hz pour le compteur RTC
    hrtc.Init.OutPut         = RTC_OUTPUT_DISABLE;
    hrtc.Init.OutPutRemap    = RTC_OUTPUT_REMAP_NONE;
    hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    hrtc.Init.OutPutType     = RTC_OUTPUT_TYPE_OPENDRAIN;
    hrtc.Init.OutPutPullUp   = RTC_OUTPUT_PULLUP_NONE;
    hrtc.Init.BinMode        = RTC_BINARY_NONE;
    if (HAL_RTC_Init(&hrtc) != HAL_OK) Error_Handler();

    sTime.Hours          = 0x0;
    sTime.Minutes        = 0x0;
    sTime.Seconds        = 0x0;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK) Error_Handler();

    sDate.WeekDay = RTC_WEEKDAY_MONDAY;
    sDate.Month   = RTC_MONTH_JANUARY;
    sDate.Date    = 0x1;
    sDate.Year    = 0x0;
    if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK) Error_Handler();

    /* USER CODE BEGIN RTC_Init 2 */
    /* USER CODE END RTC_Init 2 */
}

/**
  * @brief TIM1 Initialization Function
  * @retval None
  */
static void MX_TIM1_Init(void)
{
    /* USER CODE BEGIN TIM1_Init 0 */
    /* USER CODE END TIM1_Init 0 */

    TIM_MasterConfigTypeDef        sMasterConfig        = {0};
    TIM_OC_InitTypeDef             sConfigOC            = {0};
    TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

    /* USER CODE BEGIN TIM1_Init 1 */
    /* USER CODE END TIM1_Init 1 */

    htim1.Instance               = TIM1;
    htim1.Init.Prescaler         = 63;   // Divise l'horloge APB pour la fréquence PWM cible
    htim1.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim1.Init.Period            = 999;  // ⚠ Période = 1000 pas (rapport cyclique [0,999])
    htim1.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&htim1) != HAL_OK) Error_Handler();

    sMasterConfig.MasterOutputTrigger  = TIM_TRGO_RESET;
    sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
    sMasterConfig.MasterSlaveMode      = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK) Error_Handler();

    sConfigOC.OCMode       = TIM_OCMODE_PWM1; // Actif tant que CNT < CCR
    sConfigOC.Pulse        = 0;               // Rapport cyclique initial 0 %
    sConfigOC.OCPolarity   = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode   = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState  = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_4) != HAL_OK) Error_Handler();

    // Break/Dead-time désactivé : le pont en H est géré par GPIO, pas par break hardware
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

    HAL_TIM_MspPostInit(&htim1); // Reconnecte PA11 à la sortie TIM1 CH4
}

/**
  * @brief USART2 Initialization Function
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
  * @retval None
  */
static void MX_GPIO_Init(void)
{
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Activation obligatoire des horloges GPIO avant tout accès registre
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

/* USER CODE BEGIN MX_GPIO_Init_2 */

    // I2C1 : open-drain alternate function (PB8 = SCL, PB9 = SDA)
    // ⚠ Ces broches sont partagées avec LED_D_3 et LED_D_4 — voir moteur.h
    //GPIO_InitStruct.Pin       = I2C1_SCL_Pin | I2C1_SDA_Pin;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // Pins de commande moteur : sortie push-pull
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    GPIO_InitStruct.Pin = MOTEUR_NSLEEP_PIN | MOTEUR_ARRIERE_PIN; // PA9, PA10
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = MOTEUR_AVANT_PIN; // PB1
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // Pins LED : même mode sortie PP (PB7, PB3, PB8, PB9)
    GPIO_InitStruct.Pin = LED_G_1_PIN | LED_G_2_PIN | LED_D_3_PIN | LED_D_4_PIN;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // État initial : driver moteur en veille, LEDs éteintes
    HAL_GPIO_WritePin(GPIOA, MOTEUR_NSLEEP_PIN | MOTEUR_ARRIERE_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, MOTEUR_AVANT_PIN | LED_G_1_PIN | LED_G_2_PIN
                            | LED_D_3_PIN | LED_D_4_PIN, GPIO_PIN_RESET);

/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void Peripherals_ReInit(void)
{
    // Cette fonction sert de passerelle pour moteur.c après le réveil
    MX_GPIO_Init();
    MX_TIM1_Init();
    MX_LPTIM1_Init();
    MX_ADC1_Init();
    MX_USART2_UART_Init();
}

/* ---------------------------------------------------------------------------
 * Clock_SwitchToSleep — réduit MSI 48 MHz → 4 MHz + SCALE2 avant veille
 *
 * ⚠ Appelée par Enter_Low_Power_Mode() dans moteur.c.
 *    Déclarée ici car elle utilise huart2 et les init HAL locaux.
 *    Séquence obligatoire : flash latency → fréquence → tension → BRR UART
 *    DOIT être appelée AVANT HAL_UART_Receive_IT (sinon le HAL_UART_Init annule l'IT).
 * --------------------------------------------------------------------------- */
void Clock_SwitchToSleep(void)
{
    __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_0); // 0 wait state suffisant à 4 MHz

    RCC_OscInitTypeDef osc      = {0};
    osc.OscillatorType          = RCC_OSCILLATORTYPE_MSI;
    osc.MSIState                = RCC_MSI_ON;
    osc.MSICalibrationValue     = RCC_MSICALIBRATION_DEFAULT;
    osc.MSIClockRange           = RCC_MSIRANGE_6; // 4 MHz
    osc.PLL.PLLState            = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&osc);

    // SCALE2 (Vcore 1.0 V) seulement APRÈS la réduction de fréquence
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE2);

    HAL_UART_Init(&huart2); // Recalcule le BRR pour 115200 baud @ 4 MHz
}

/* ---------------------------------------------------------------------------
 * Clock_SwitchToFullSpeed — restaure MSI 4 MHz → 48 MHz + SCALE1 au réveil
 *
 * ⚠ Appelée par Enter_Low_Power_Mode() dans moteur.c après le WFI.
 *    Séquence obligatoire : tension → fréquence → flash latency → BRR UART
 * --------------------------------------------------------------------------- */
void Clock_SwitchToFullSpeed(void)
{
    // SCALE1 (Vcore 1.2 V) obligatoire AVANT de repasser au-dessus de 26 MHz
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitTypeDef osc  = {0};
    osc.OscillatorType      = RCC_OSCILLATORTYPE_MSI;
    osc.MSIState            = RCC_MSI_ON;
    osc.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
    osc.MSIClockRange       = RCC_MSIRANGE_11; // 48 MHz
    osc.PLL.PLLState        = RCC_PLL_NONE;
    HAL_RCC_OscConfig(&osc);

    __HAL_FLASH_SET_LATENCY(FLASH_LATENCY_1); // Restaure latence pour 48 MHz

    HAL_UART_Init(&huart2); // Recalcule le BRR pour 115200 baud @ 48 MHz
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    __disable_irq(); // Coupe toutes les interruptions pour figer le système
    while (1) {}
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
