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
#include "stdio.h"
#include "string.h"
#include "math.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

	//  Partie Energie  //
#define OFFSET_BAT -0.10f //retire 0,10V à la valeur finale

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

DAC_HandleTypeDef hdac1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

uint32_t adc_buffer[2]; //[0] = IN5 (Rank1, LDR),  [1] = IN4 (Rank2, batterie)
volatile uint8_t adc_ready = 0; // flag levé par le DMA quand les données sont prêtes


   //==================================================================//
  //                INITIALISATION PARTIE LUMINOSITÉ                  //
 //==================================================================//


// Variable pour la mesure de la luminosité
uint16_t adc_value = 0;
float voltage = 0.0f;
float R_ldr = 0.0f;
float lux = 0.0f;
char msg[100];

// Paramètres du montage
const float R_FIXED = 22000.0f; // résistance de 100k // Mesure Luminosité
const float VCC = 3.3f;


  //==================================================================//
 //               INITIALISATION PARTIE ÉNERGIE PILE                 //
//==================================================================//


// Variable pour l'estimation de l'énergie
uint32_t adc_32 = 0;   // Valeur brute lue par l'ADC (0-4095)
uint16_t adc_bat_value = 0;   // Valeur brute lue par l'ADC (0-4095)
float v_bat_measurer = 0.0f;  // Tension lue sur la pin PA0
float v_bat_reel = 0.0f;      // Tension réelle de la pile (après correction du pont)
int bat_pourcentage = 0;       // Résultat final en %

// Paramètres du montage
const float R10 = 560000.0f; //res 560k Ohm
const float R11 = 330000.0f; //res 330k Ohm

// Pont Diviseur Pile
const float PDP_Bat_Coef= (R11+R10)/ R11;

// Seuil des Piles (6V)
const float V_MAX = 6.0f; // 100%
const float V_MIN = 4.0f; // 0%

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);
static void MX_DAC1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

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
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_DAC1_Init();
  /* USER CODE BEGIN 2 */
  HAL_ADCEx_Calibration_Start(&hadc1);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET); // Alimente le pont diviseur
  HAL_Delay(10);
  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, 2); // Lance l'acquisition DMA en continu

  /* USER CODE END 2 */

  /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
    {

	  //==================================================================//
	 //                          LANCEMENT DES ADC                       //
	//==================================================================//

      // Lecture de la tension sur PA5 (LDR_INFO) // mesurer luminosité
      /**HAL_ADC_Start(&hadc1);
      if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
      {
          adc_value = HAL_ADC_GetValue(&hadc1);
      }

      // Lecture de la tension sur PA0 // Estimation de batterie (PILE)
      if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
           {
               adc_bat_value = HAL_ADC_GetValue(&hadc1);
           }

      HAL_ADC_Stop(&hadc1);
*/

	      if (adc_ready == 1)
	      {
	          adc_ready = 0;   // on remet le flag à 0

	  // Lecture DMA : les valeurs sont mises à jour automatiquement en fond de tâche
	  adc_32  = adc_buffer[0];
	  //adc_value     = adc_buffer[0];   // IN5 (PA5) — LDR luminosité
	  //adc_bat_value = adc_buffer[1];   // IN4 (PA4) — pont diviseur batterie
	  adc_value     = adc_32&0xFFFF;   // IN5 (PA5) — LDR luminosité
	  adc_bat_value = (adc_32&0xFFFF0000)>>16;   // IN4 (PA4) — pont diviseur batterie


      //==================================================================//
     //                          PARTIE LUMINOSITÉ                       //
    //==================================================================//

      //  Conversion : Chiffre ADC -> Tension (Volt)
      voltage = (float)adc_value * VCC / 4095.0;

      // Calcul de la partie entière et des deux premières décimales
      	  //int volt_entier = (int)voltage;
      	  //int volt_decimale = (int)((voltage - volt_entier) * 100);

      //  Calcul de la résistance de la LDR puis des Lux
      	  // On vérifie que voltage > 0 pour éviter de diviser par zéro
      if (voltage > 0.1f) {
          // Formule du pont diviseur inversée pour trouver R_ldr
          R_ldr = (VCC * R_FIXED/ voltage) - R_FIXED;

          // Formule d'approximation Lux (standard pour une LDR de 10k-100k)
          // Lux = 500 / (R_ldr en kOhm)
          //lux = 500.0 / (R_ldr / 1000.0);
          lux = pow(10,((log(R_ldr/1000.0f)-3)/-0.91));
      } else {
          lux = 0.0;
      }
      //Décomposition pour affichage sans %f
          int volt_entier = (int)voltage;
          int volt_dec    = (int)((voltage - volt_entier) * 100);
          int rldr_kohm   = (int)(R_ldr / 1000.0f);
          int lux_entier  = (int)lux;
          int lux_dec     = (int)((lux - lux_entier) * 10);

          //==================================================================//
         //                        PARTIE ÉNERGIE PILE                       //
        //==================================================================//

//Tension sur la PIN PA0 (entre 0 et 3.3V car elle ne prend que 3.3V)
          v_bat_measurer = ((float)adc_bat_value) * VCC / 4095.0;

// Tension Réel des piles (Application du Coef)
          //v_bat_reel = v_bat_measurer * PDP_Bat_Coef;
          v_bat_reel = v_bat_measurer * PDP_Bat_Coef ;

          // Ajout de l'Offset pour corriger la mesure
          v_bat_reel = v_bat_reel + OFFSET_BAT;

      //Calcul du pourcentage (Produit en croix entre V_MIN et V_MAX)
      if (v_bat_reel > V_MIN){
    	  bat_pourcentage = (int)(((v_bat_reel - V_MIN)/(V_MAX - V_MIN))*100.0f);
      } else {
    	  bat_pourcentage = 0;
      }

      // Sécurité pour ne pas afficher 102% ou -2%
          if (bat_pourcentage > 100) bat_pourcentage = 100;
          if (bat_pourcentage < 0)   bat_pourcentage = 0;

      //Décomposition pour affichage sans %f
          int v_entier = (int)v_bat_reel;
          int v_dec = (int)((v_bat_reel - v_entier) * 100);

          //==================================================================//
         //                       AFFICHAGE LIAISON SÉRIE                    //
        //==================================================================//

         //Affichage Luminosité
       // Affichage sur le PC (VCP / USART2)
      // On affiche l'entier (int)lux pour être sûr que ça s'affiche sans config spéciale
      //int len = sprintf(msg, "ADC: %lu |Tension : %d.%02dV | Lux estimat: %d\r\n", adc_value, voltage, (int)lux);
         int len = sprintf(msg, "ADC:%4lu | V:%d.%02dV | R_ldr:%d kohm | Lux:%d.%d\r\n", adc_value, volt_entier, volt_dec, rldr_kohm, lux_entier, lux_dec);
          HAL_UART_Transmit(&huart2, (uint8_t*)msg, len, HAL_MAX_DELAY);

          HAL_Delay(500); // On attend 0.5 secondes entre chaque mesure


          //Affichage Batterie Restante
          int len2 = sprintf(msg, "BAT: %4lu | Tension: %d.%02dV | Energie: %d%%\r\n\r\n",
                  adc_bat_value, v_entier, v_dec, bat_pourcentage);
              HAL_UART_Transmit(&huart2, (uint8_t*)msg, len2, 100);

              HAL_Delay(500); // On attend 0.5 secondes entre chaque mesures

          int len3 = sprintf(msg, "BRUT_LDR: %lu | BRUT_BAT: %lu\r\n", adc_value, adc_bat_value);
              HAL_UART_Transmit(&huart2, (uint8_t*)msg, len3, 100);

      // 5. COMMANDE DE LA LED (Seuil : 50 Lux)
      //if (lux < 50.0) {
      //    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);   // Allume (Nuit)
      //} else {
      //    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET); // Éteint (Jour)
      //}


	      }

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
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
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = ENABLE;
  hadc1.Init.LowPowerAutoPowerOff = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.NbrOfConversion = 2;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
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
  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */


  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief DAC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_DAC1_Init(void)
{

  /* USER CODE BEGIN DAC1_Init 0 */

  /* USER CODE END DAC1_Init 0 */

  DAC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN DAC1_Init 1 */

  /* USER CODE END DAC1_Init 1 */

  /** DAC Initialization
  */
  hdac1.Instance = DAC1;
  if (HAL_DAC_Init(&hdac1) != HAL_OK)
  {
    Error_Handler();
  }

  /** DAC channel OUT1 config
  */
  sConfig.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
  sConfig.DAC_Trigger = DAC_TRIGGER_NONE;
  sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_DISABLE;
  sConfig.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_INTERNAL;
  sConfig.DAC_UserTrimming = DAC_TRIMMING_FACTORY;
  if (HAL_DAC_ConfigChannel(&hdac1, &sConfig, DAC_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DAC1_Init 2 */

  /* USER CODE END DAC1_Init 2 */

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
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

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

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

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
// Callback appelé automatiquement par le DMA quand les 2 canaux sont convertis
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc->Instance == ADC1)
    {
        adc_ready = 1;   // lève le flag → le while(1) va traiter les données
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
