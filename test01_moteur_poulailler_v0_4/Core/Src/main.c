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
ADC_HandleTypeDef hadc1;
LPTIM_HandleTypeDef hlptim1;
TIM_HandleTypeDef htim1;
UART_HandleTypeDef huart2;
/* USER CODE BEGIN PV */
uint16_t current_speed = 900; // On fixe une vitesse par défaut pour les tests
uint8_t rx_data;              // Variable pour stocker le caractère reçu
char msg[100];
// lecture courant moteur
uint32_t adc_value = 0;
float courant_moteur = 0.0;
uint32_t last_tick = 0; // Pour l'envoi périodique
// moyenne courant moteur
float lecture[5] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
float somme_courant = 0.0f;
float moyenne_courant = 0.0f;
// sécurité courant
float courant_fonctionnement_morteur = 0.0f; // courant de fonctionnement normal du moteur
float threshold = 0.9f; // limite définie
int compteur_securite = 0;       // Arret du moteur après 5 relevé > threshold
uint32_t motor_start_time = 0;
typedef enum {
	MOTEUR_OFF,
	DEMARRAGE_MOTEUR,
	CALIBRATION_MOTEUR,
   MOTEUR_MARCHE,
} MotorState_t;
MotorState_t motor_state = MOTEUR_OFF;
uint32_t elapsed = 0; // On la met ici pour qu'elle soit accessible partout
/* USER CODE END PV */
/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM1_Init(void);
static void MX_LPTIM1_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
//test
/* USER CODE END PFP */
/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void Motor_Forward(void)
{
// ARRÊT des PWM
  HAL_LPTIM_PWM_Stop(&hlptim1, LPTIM_CHANNEL_1);
  HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);
// PINS A ZERO
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
  HAL_Delay(5);
// Lancement de TIM1 + set de la vitesse
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, current_speed);
  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);
// Timer démarage moteur +  security_counter
  motor_start_time = HAL_GetTick();
  motor_state = DEMARRAGE_MOTEUR;
  compteur_securite = 0;
  elapsed = 0;
}
void Motor_Reverse(void)
{
// ARRÊT des PWM
  HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);
  HAL_LPTIM_PWM_Stop(&hlptim1, LPTIM_CHANNEL_1);
// PINS A ZERO
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
  HAL_Delay(5);
// Lancement de LPTIM1 + set de la vitesse
  HAL_LPTIM_PWM_Start(&hlptim1, LPTIM_CHANNEL_1);
  __HAL_LPTIM_COMPARE_SET(&hlptim1, LPTIM_CHANNEL_1, current_speed);
// Timer démarage moteur +  security_counter
  motor_start_time = HAL_GetTick();
  motor_state = DEMARRAGE_MOTEUR;
  compteur_securite = 0;
}
void Motor_Stop(void)
{
// ARRÊT des PWM
  HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_4);
  HAL_LPTIM_PWM_Stop(&hlptim1, LPTIM_CHANNEL_1);
// PINS A ZERO
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_11, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
  motor_state = MOTEUR_OFF;
  threshold = 1.0f; // Reset du seuil
}
void Motor_SetSpeed(uint16_t speed)
{
  if (speed > 999) speed = 999;
  if (speed > 0 && speed < 700) speed = 700;
  current_speed = speed;
  // Mise à jour immédiate des registres pour les deux timers
  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_4, current_speed);
  __HAL_LPTIM_COMPARE_SET(&hlptim1, LPTIM_CHANNEL_1, current_speed);
}
// Lecture courant moteur
float Get_Motor_Current(void)
{
  uint32_t raw_value = 0;
  float res_ohm = 1.3f;
  float offset = 0.090f;
   HAL_ADC_Start(&hadc1);
  if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
      adc_value = HAL_ADC_GetValue(&hadc1);
      // Formule : Courant = Tension / Résistance
      // Tension = (Valeur_ADC / 4095) * 3.3V
      courant_moteur = (((adc_value * 3.3f) / 4095.0f) / res_ohm)*1.103;
      courant_moteur -= offset;
       // On évite d'afficher des valeurs négatives
       if (courant_moteur < 0) courant_moteur = 0;
  }
  HAL_ADC_Stop(&hadc1);
  return courant_moteur;
}
float Update_Moving_Average(float new_sample)
{
   // 1. Décalage des valeurs (Shift)
	lecture[0] = lecture[1];
	lecture[1] = lecture[2];
	lecture[2] = lecture[3];
	lecture[3] = lecture[4];
	lecture[4] = new_sample;
   // 2. Calcul de la somme
   somme_courant = 0;
   for(int i = 0; i < 5; i++) {
   	somme_courant += lecture[i];
   }
   // 3. Retourne la moyenne
   return somme_courant / 5.0f;
}
/* USER CODE END 0 */
/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
 /* USER CODE BEGIN 1 */
	HAL_Delay(2000);
 /* USER CODE END 1 */
 /* MCU Configuration--------------------------------------------------------*/
 /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
 HAL_Init();
 /* USER CODE BEGIN Init */
 /* USER CODE END Init */
 /* Configure the system clock */
 SystemClock_Config();
 /* USER CODE BEGIN SysInit */
 /* USER CODE END SysInit */
 /* Initialize all configured peripherals */
 MX_GPIO_Init();
 MX_TIM1_Init();
 MX_LPTIM1_Init();
 MX_ADC1_Init();
 MX_USART2_UART_Init();
 /* USER CODE BEGIN 2 */
HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4); // Changé de CHANNEL_1 à CHANNEL_4
__HAL_TIM_MOE_ENABLE(&htim1);
if (HAL_LPTIM_PWM_Start(&hlptim1, LPTIM_CHANNEL_1) != HAL_OK)
{
    Error_Handler();
}
 /* USER CODE END 2 */
 /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
 BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);
 HAL_UART_Receive_IT(&huart2, &rx_data, 1);
 /* Infinite loop */
 /* USER CODE BEGIN WHILE */
 while (1)
   {
       if (motor_state == MOTEUR_OFF)
       {
           // --- MODE SOMMEIL ---
           HAL_UART_Transmit(&huart2, (uint8_t*)"Zzz...\r\n", 8, 100);
           // On nettoie les erreurs UART avant de dormir
           __HAL_UART_CLEAR_IT(&huart2, UART_CLEAR_OREF | UART_CLEAR_NEF | UART_CLEAR_FEF);
           HAL_UART_Receive_IT(&huart2, &rx_data, 1);
           HAL_SuspendTick();
           HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);
           // --- RÉVEIL ---
           SystemClock_Config();
           HAL_ResumeTick();
           HAL_UART_Transmit(&huart2, (uint8_t*)"WAKEUP!\r\n", 9, 100);
           HAL_Delay(100);
       }
       else
       {
           // --- MOTEUR EN MARCHE ---
           uint32_t current_time = HAL_GetTick();
           if (current_time - last_tick >= 500)
           {
               last_tick = current_time;
               elapsed = current_time - motor_start_time;
               float instant_current = Get_Motor_Current();
               moyenne_courant = Update_Moving_Average(instant_current);
               // Machine à états
               if (motor_state == DEMARRAGE_MOTEUR && elapsed > 2000) {
                   motor_state = CALIBRATION_MOTEUR;
               }
               else if (motor_state == CALIBRATION_MOTEUR && elapsed > 3000) {
                   courant_fonctionnement_morteur = moyenne_courant;
                   threshold = (courant_fonctionnement_morteur < 0.05f) ? 0.1f : courant_fonctionnement_morteur * 1.5f;
                   motor_state = MOTEUR_MARCHE;
               }
               else if (motor_state == MOTEUR_MARCHE) {
                   if (moyenne_courant > threshold) {
                       compteur_securite++;
                       if (compteur_securite >= 5) Motor_Stop();
                   } else {
                       compteur_securite = 0;
                   }
                   if (elapsed > 20000) Motor_Stop();
               }
               // Affichage unique et propre
               int len = sprintf(msg, "I:%.2fA | Thr:%.2fA | Stat:%d | T:%lu s\r\n",
                                moyenne_courant, threshold, (int)motor_state, elapsed/1000);
               HAL_UART_Transmit(&huart2, (uint8_t*)msg, len, 100);
           }
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
 /** Configure the main internal regulator output voltage
 */
 HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
 /** Initializes the RCC Oscillators according to the specified parameters
 * in the RCC_OscInitTypeDef structure.
 */
 RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
 RCC_OscInitStruct.MSIState = RCC_MSI_ON;
 RCC_OscInitStruct.MSICalibrationValue = RCC_MSICALIBRATION_DEFAULT;
 RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_11;
 RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
 if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
 {
   Error_Handler();
 }
 /** Initializes the CPU, AHB and APB buses clocks
 */
 RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                             |RCC_CLOCKTYPE_PCLK1;
 RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
 RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
 RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
 if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
 {
   Error_Handler();
 }
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
 /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
 */
 hadc1.Instance = ADC1;
 hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV1;
 hadc1.Init.Resolution = ADC_RESOLUTION_12B;
 hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
 hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
 hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
 hadc1.Init.LowPowerAutoWait = DISABLE;
 hadc1.Init.LowPowerAutoPowerOff = DISABLE;
 hadc1.Init.ContinuousConvMode = DISABLE;
 hadc1.Init.NbrOfConversion = 1;
 hadc1.Init.DiscontinuousConvMode = DISABLE;
 hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
 hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
 hadc1.Init.DMAContinuousRequests = DISABLE;
 hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
 hadc1.Init.SamplingTimeCommon1 = ADC_SAMPLETIME_160CYCLES_5;
 hadc1.Init.SamplingTimeCommon2 = ADC_SAMPLETIME_160CYCLES_5;
 hadc1.Init.OversamplingMode = DISABLE;
 hadc1.Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_HIGH;
 if (HAL_ADC_Init(&hadc1) != HAL_OK)
 {
   Error_Handler();
 }
 /** Configure Regular Channel
 */
 sConfig.Channel = ADC_CHANNEL_14;
 sConfig.Rank = ADC_REGULAR_RANK_1;
 sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
 if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
 {
   Error_Handler();
 }
 /* USER CODE BEGIN ADC1_Init 2 */
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
 hlptim1.Instance = LPTIM1;
 hlptim1.Init.Clock.Source = LPTIM_CLOCKSOURCE_APBCLOCK_LPOSC;
 hlptim1.Init.Clock.Prescaler = LPTIM_PRESCALER_DIV64;
 hlptim1.Init.Trigger.Source = LPTIM_TRIGSOURCE_SOFTWARE;
 hlptim1.Init.Period = 999;
 hlptim1.Init.UpdateMode = LPTIM_UPDATE_IMMEDIATE;
 hlptim1.Init.CounterSource = LPTIM_COUNTERSOURCE_INTERNAL;
 hlptim1.Init.Input1Source = LPTIM_INPUT1SOURCE_GPIO;
 hlptim1.Init.Input2Source = LPTIM_INPUT2SOURCE_GPIO;
 hlptim1.Init.RepetitionCounter = 0;
 if (HAL_LPTIM_Init(&hlptim1) != HAL_OK)
 {
   Error_Handler();
 }
 sConfig1.Pulse = 0;
 sConfig1.OCPolarity = LPTIM_OCPOLARITY_LOW;
 if (HAL_LPTIM_OC_ConfigChannel(&hlptim1, &sConfig1, LPTIM_CHANNEL_1) != HAL_OK)
 {
   Error_Handler();
 }
 /* USER CODE BEGIN LPTIM1_Init 2 */
 /* USER CODE END LPTIM1_Init 2 */
 HAL_LPTIM_MspPostInit(&hlptim1);
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
 TIM_MasterConfigTypeDef sMasterConfig = {0};
 TIM_OC_InitTypeDef sConfigOC = {0};
 TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};
 /* USER CODE BEGIN TIM1_Init 1 */
 /* USER CODE END TIM1_Init 1 */
 htim1.Instance = TIM1;
 htim1.Init.Prescaler = 63;
 htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
 htim1.Init.Period = 999;
 htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
 htim1.Init.RepetitionCounter = 0;
 htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
 if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
 {
   Error_Handler();
 }
 sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
 sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
 sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
 if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
 {
   Error_Handler();
 }
 sConfigOC.OCMode = TIM_OCMODE_PWM1;
 sConfigOC.Pulse = 0;
 sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
 sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
 sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
 sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
 if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
 {
   Error_Handler();
 }
 sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
 sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
 sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
 sBreakDeadTimeConfig.DeadTime = 0;
 sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
 sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
 sBreakDeadTimeConfig.BreakFilter = 0;
 sBreakDeadTimeConfig.BreakAFMode = TIM_BREAK_AFMODE_INPUT;
 sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
 sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
 sBreakDeadTimeConfig.Break2Filter = 0;
 sBreakDeadTimeConfig.Break2AFMode = TIM_BREAK_AFMODE_INPUT;
 sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
 if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
 {
   Error_Handler();
 }
 /* USER CODE BEGIN TIM1_Init 2 */
 /* USER CODE END TIM1_Init 2 */
 HAL_TIM_MspPostInit(&htim1);
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
 huart2.Instance = USART2;
 huart2.Init.BaudRate = 115200;
 huart2.Init.WordLength = UART_WORDLENGTH_8B;
 huart2.Init.StopBits = UART_STOPBITS_1;
 huart2.Init.Parity = UART_PARITY_NONE;
 huart2.Init.Mode = UART_MODE_TX_RX;
 huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
 huart2.Init.OverSampling = UART_OVERSAMPLING_16;
 huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
 huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
 huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
 if (HAL_UART_Init(&huart2) != HAL_OK)
 {
   Error_Handler();
 }
 if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
 {
   Error_Handler();
 }
 if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
 {
   Error_Handler();
 }
 if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK)
 {
   Error_Handler();
 }
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
 /* GPIO Ports Clock Enable */
 __HAL_RCC_GPIOC_CLK_ENABLE();
 __HAL_RCC_GPIOF_CLK_ENABLE();
 __HAL_RCC_GPIOA_CLK_ENABLE();
 __HAL_RCC_GPIOB_CLK_ENABLE();
 /*Configure GPIO pins : I2C1_SCL_Pin I2C1_SDA_Pin */
 GPIO_InitStruct.Pin = I2C1_SCL_Pin|I2C1_SDA_Pin;
 GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
 GPIO_InitStruct.Pull = GPIO_NOPULL;
 GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
 GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
 HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}
/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
   if (huart->Instance == USART2)
   {
       switch(rx_data)
       {
         case 'D': Motor_Forward(); break;
         case 'A': Motor_Reverse(); break;
         case 'S': Motor_Stop();    break;
         case '+': Motor_SetSpeed(current_speed + 100); break;
         case '-': Motor_SetSpeed(current_speed - 100); break;
       }
       HAL_UART_Receive_IT(&huart2, &rx_data, 1);
   }
}
/* USER CODE END 4 */
/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
 /* USER CODE BEGIN Error_Handler_Debug */
/* User can add his own implementation to report the HAL error return state */
__disable_irq();
while (1)
{
}
 /* USER CODE END Error_Handler_Debug */
}
#ifdef  USE_FULL_ASSERT
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
/* User can add his own implementation to report the file name and line number,
   ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
 /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

