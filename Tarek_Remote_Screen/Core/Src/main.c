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
#include "lcd.h"
#include "graphics.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "variables_globales.h" // Bibliotheque de des variables utlisés
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

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
//------------NOUVEAU-----------------
// ---- Instanciation des variables globales ----
time_t temps = 0;
struct tm tm_temps = {0};
signed char UTC = 1;        // Par défaut (ex: hiver France)
signed char absUTC = 1;

long tensionpile = 645;     // Simulons une pile pleine au démarrage
char langue = 2;            // Par défaut : 2 (Français)

char modeO = 1;             // Heure fixe par défaut
char modeF = 1;             // Heure fixe par défaut
signed char heureO = 8;     // Ouvre à 8h00
signed char minO = 0;
signed char heureF = 20;    // Ferme à 20h00
signed char minF = 0;
char signed minretard = 15; // 15 min de retard par défaut

signed char latitude = 48;  // Ex: Paris
int longitude = 2;          // Ex: Paris
char choixi = 1;

// VARIABLES DE NAVIGATION (Globales pour être vues par menu.c)
uint8_t ON = 0;
uint8_t DWN = 0;
uint8_t HUP = 0;
uint8_t RTN = 0;
uint8_t ecran = 0; // état de la machine d'état (0 = Accueil)
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// Variables globales de tes boutons
uint8_t touche_ON = 0;
uint8_t touche_DWN = 0;
uint8_t touche_HUP = 0;
uint8_t touche_RTN = 0;

void lire_commandes_Docklight(void) {
    static char rx_buffer[10]; // Stocke le mot en cours de réception
    static uint8_t index = 0;
    uint8_t rx_char;

    // Remise à zéro des boutons à chaque passage
    touche_ON = 0;
    touche_DWN = 0;
    touche_HUP = 0;
    touche_RTN = 0;

    // Lecture d'un caractère (non bloquant)
    if (HAL_UART_Receive(&huart2, &rx_char, 1, 0) == HAL_OK) {
        // Si on reçoit 'Entrée' (\r ou \n), le mot est complet
        if (rx_char == '\r' || rx_char == '\n') {
            rx_buffer[index] = '\0'; // On termine la chaîne de caractères

            // On compare le mot reçu
                        if (strcmp(rx_buffer, "ON") == 0) {
                            touche_ON = 1;
                            char msg[] = "--> Action: ON\r\n";
                            HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
                        }
                        else if (strcmp(rx_buffer, "DWN") == 0) {
                            touche_DWN = 1;
                            char msg[] = "--> Action: DWN\r\n";
                            HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
                        }
                        else if (strcmp(rx_buffer, "HUP") == 0) {
                            touche_HUP = 1;
                            char msg[] = "--> Action: HUP\r\n";
                            HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
                        }
                        else if (strcmp(rx_buffer, "RTN") == 0) {
                            touche_RTN = 1;
                            char msg[] = "--> Action: RTN\r\n";
                            HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), 100);
                        }
        		}
        else {
            // Sinon, on ajoute la lettre au mot (si on ne dépasse pas la taille du buffer)
            if (index < 9) {
                rx_buffer[index] = (char)rx_char;
                index++;
            }
        }
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
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

    // 1. Initialisation de l'écran
    initialise_LCD();
    setOrientation(4);
    // 2. Nettoyer l'écran (fond noir)
    // blackWhite = 1 selon ta fonction clearScreen mettra le fond en 0x0000 (Noir)
    clearScreen(1);

    // 3. Configuration des couleurs pour le texte
    // F800 = Rouge, 0xFFFF = Blanc, 0x07E0 = Vert

    // "1 2 3" en Blanc
    setColor(0xFFFF);
    setBackgroundColor(0x0000);
    drawString(20, 30, FONT_LG, "1 2 3");

    // "VIVA" en Vert
    setColor(0x07E0);
    drawString(20, 60, FONT_LG, "VIVA");

    // "L'ALGERIE !" en Rouge
    setColor(0xF800);
    drawString(20, 90, FONT_LG, "L'ALGERIE !");

    char message_init[] = "\r\n=== LIAISON SERIE INITIALISEE ===\r\nEnvoyez ON, DWN, HUP ou RTN pour naviguer.\r\n";
     HAL_UART_Transmit(&huart2, (uint8_t*)message_init, strlen(message_init), HAL_MAX_DELAY);

    /* USER CODE END 2 */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  // 1. Lire les mots venant de Docklight
	        lire_commandes_Docklight();

	        // 2. Transférer aux variables de ta machine à états
	        ON = touche_ON;
	        DWN = touche_DWN;
	        HUP = touche_HUP;
	        RTN = touche_RTN;

	        // 3. Ta machine à états .
	        menu(); // Appel de ta machine à états
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
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
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
  HAL_GPIO_WritePin(GPIOA, A0_Pin|ALIM_AFF_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, CS_Pin|RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : TOUCH_DOWN_Pin TOUCH_MENU_Pin TOUCH_OK_Pin TOUCH_UP_Pin */
  GPIO_InitStruct.Pin = TOUCH_DOWN_Pin|TOUCH_MENU_Pin|TOUCH_OK_Pin|TOUCH_UP_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : A0_Pin ALIM_AFF_Pin */
  GPIO_InitStruct.Pin = A0_Pin|ALIM_AFF_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

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
