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
#include <string.h>
// ---------------------modif
#include "lcd.h"
#include "graphics.h"
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

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint8_t rx_byte[1];       // La boîte pour recevoir 1 seule lettre à la fois
char rx_buffer[20];       // Le carnet (buffer) pour écrire le mot en entier (max 20 lettres)
uint8_t rx_index = 0;     // Le stylo (l'index) qui retient à quelle case on est rendu
// NOUVELLE VARIABLE : Chronomètre du dernier ordre
uint32_t temps_dernier_ordre = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI1_Init(void);
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
  MX_USART2_UART_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */

  // ---> NOUVEAU : On allume la LED verte (Simulation de l'écran ALLUMÉ)
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);

  // 1. Envoi du message de démarrage UNE SEULE FOIS
  uint8_t message[] = "Test de la carte OK ! En attente d'ordres...\r\n";
  HAL_UART_Transmit(&huart2, message, sizeof(message)-1, 1000);

  // 2. Armement de l'interruption pour écouter le premier caractère
  HAL_UART_Receive_IT(&huart2, rx_byte, 1);

  // ==========================================
  // 3. INITIALISATION ET TEST DE L'ÉCRAN
  // ==========================================
  initialise_LCD(); // Réveille et configure l'écran

  // Test d'affichage (utilise les fonctions de graphics.h)
  setColor(0xF800); // Choisir le Rouge (Code couleur RGB565)
  fillRect(0, 0, getScreenWidth(), getScreenHeight()); // Remplir l'écran en rouge pour tester !

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  // 1. On calcule combien de temps s'est écoulé depuis le  dernier ordre
	        uint32_t temps_actuel = HAL_GetTick();
	        uint32_t temps_ecoule = temps_actuel - temps_dernier_ordre;

	        // 2. Si ça fait plus de 10 000 ms (10 secondes)
	        if (temps_ecoule > 10000)
	        {
	            // On prépare un message pour avertir le PC
	            uint8_t msg_veille[] = "Zzz... 10s d'inactivite, je passe en VEILLE !\r\n";
	            HAL_UART_Transmit(&huart2, msg_veille, sizeof(msg_veille)-1, 10);

	            // ---> NOUVEAU : On éteint la LED verte (Simulation de l'écran ÉTEINT)
	            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

	            // NOTE POUR PLUS TARD : C'est ici que tu mettras la VRAIE fonction
	            // pour couper l'alimentation de l'écran TFT et endormir le STM32.

	            // Pour éviter que le message s'affiche en boucle toutes les millisecondes,
	            // on triche un peu pour le test : on remet le chrono à zéro.
	            temps_dernier_ordre = HAL_GetTick();
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
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_4BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5|A0_Pin|ALIM_AFF_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, CS_Pin|RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA5 A0_Pin ALIM_AFF_Pin */
  GPIO_InitStruct.Pin = GPIO_PIN_5|A0_Pin|ALIM_AFF_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : TOUCH_DOWN_Pin TOUCH_MENU_Pin TOUCH_OK_Pin TOUCH_UP_Pin */
  GPIO_InitStruct.Pin = TOUCH_DOWN_Pin|TOUCH_MENU_Pin|TOUCH_OK_Pin|TOUCH_UP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : CS_Pin RST_Pin */
  GPIO_InitStruct.Pin = CS_Pin|RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        // 1. Est-ce que la lettre qu'on vient de recevoir est un "Retour à la ligne" ?
        // (Sur Hercules, quand tu envoies un mot, ça ajoute souvent \r ou \n à la fin)
    	if (rx_byte[0] == '\r' || rx_byte[0] == '\n')
    	        {
    		// C'est la fin du mot. On ferme le mot proprement avec '\0'
    		rx_buffer[rx_index] = '\0';

    	    // ---> IMPORTANT : On remet à zéro le chrono de veille et on rallume l'écran (LED)
    		temps_dernier_ordre = HAL_GetTick();
    		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    		// <---

    		// ====================================================//.
    		// NOUVELLE GESTION DES 4 TOUCHES (MENU, UP, DOWN, OK) //
    		// ====================================================//
    		if (strcmp(rx_buffer, "MENU") == 0)
    			{
    			uint8_t msg_menu[] = "--> Action : Affichage MENU PRINCIPAL\r\n";
    			HAL_UART_Transmit(&huart2, msg_menu, sizeof(msg_menu)-1, 10);
    			}
    		else if (strcmp(rx_buffer, "UP") == 0)
    	        {
    			uint8_t msg_up[] = "--> Action : Navigation HAUT\r\n";
    	        HAL_UART_Transmit(&huart2, msg_up, sizeof(msg_up)-1, 10);
    	        }
    	            else if (strcmp(rx_buffer, "DOWN") == 0)
    	            {
    	                uint8_t msg_down[] = "--> Action : Navigation BAS\r\n";
    	                HAL_UART_Transmit(&huart2, msg_down, sizeof(msg_down)-1, 10);
    	            }
    	            else if (strcmp(rx_buffer, "OK") == 0)
    	            {
    	                uint8_t msg_ok[] = "--> Action : VALIDATION (OK)\r\n";
    	                HAL_UART_Transmit(&huart2, msg_ok, sizeof(msg_ok)-1, 10);
    	            }
    	            // Si on a tapé un mot inconnu (et que ce n'est pas juste un mot vide)
    	            else if (rx_index > 0)
    	            {
    	                uint8_t msg_err[] = "--> Erreur : Touche non reconnue...\r\n";
    	                HAL_UART_Transmit(&huart2, msg_err, sizeof(msg_err)-1, 10);
    	            }
    	            // ==========================================

    	            // On a fini de lire, on remet le "stylo" à zéro
    	            rx_index = 0;
        }
        else
        {
            // NON, ce n'est pas la fin du mot. C'est une lettre normale (ex: 'O' ou 'N')
            // On l'ajoute dans notre carnet, tant qu'il y a de la place
            if (rx_index < 19)
            {
                rx_buffer[rx_index] = rx_byte[0]; // On écrit la lettre
                rx_index++;                       // On avance le stylo d'une case
            }
        }

        // 6. On réarme l'interruption pour écouter la PROCHAINE lettre
        HAL_UART_Receive_IT(&huart2, rx_byte, 1);
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
