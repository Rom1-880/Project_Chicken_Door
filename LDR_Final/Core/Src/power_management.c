/* functions.c — Fonctions utilisateur extraites de main.c */

#include "power_management.h"

/* Déclarations forward (définies dans main.c) */
extern void SystemClock_Config(void);
extern void Public_MX_GPIO_Init(void);
extern void Public_MX_USART2_UART_Init(void);

  //==================================================================//
 //             MISE EN VEILLE PROCESSEUR                            //
//==================================================================//

void Aller_Au_Dodo(void)
{
 //	Stop1
	// ── 1. Désactiver l'UART ──
    HAL_UART_DeInit(&huart2);
    __HAL_RCC_USART2_CLK_DISABLE();

    // ── 2. Arrêter l'ADC et le DMA ──
    HAL_ADC_Stop_DMA(&hadc1);
    adc_ready = 0;

    // ── 3. Éteindre le pont diviseur (PA4) ──
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

    // ── 4. Toutes les GPIO en analogique (zéro fuite) ──
    GPIO_InitTypeDef GPIO_Blank = {0};
    GPIO_Blank.Mode = GPIO_MODE_ANALOG;
    GPIO_Blank.Pull = GPIO_NOPULL;
    GPIO_Blank.Pin  = GPIO_PIN_ALL;
    HAL_GPIO_Init(GPIOA, &GPIO_Blank);
    HAL_GPIO_Init(GPIOB, &GPIO_Blank);
    HAL_GPIO_Init(GPIOC, &GPIO_Blank);

    // ── 5. Couper le SysTick ──
    CLEAR_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);

    // ── 6. STOP MODE 1 — ~10 µA ──
    //HAL_PWREx_EnterSTOP1Mode(PWR_STOPENTRY_WFI);
    HAL_PWREx_EnterSTOP2Mode(PWR_STOPENTRY_WFI);

    //HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);
    //
    //  >>> RÉVEIL PAR LA RTC <<<

    // ── 7. Reconfigurer l'horloge (obligatoire après Stop Mode) ──
    SystemClock_Config();

    // ── 8. Réactiver le SysTick ──
    SET_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);
    HAL_ResumeTick();

    // ── 9. Restaurer les GPIO ──
    Public_MX_GPIO_Init();


    // ── 10. Rallumer l'UART ──
    __HAL_RCC_USART2_CLK_ENABLE();
    Public_MX_USART2_UART_Init();

    // ── 11. Rallumer le pont diviseur et relancer l'ADC ──
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
    HAL_Delay(10);
    HAL_ADCEx_Calibration_Start(&hadc1);
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, 2);


/*			Sleep normal
	// ── 1. Arrêter ce qui consomme en tâche de fond ──
	    HAL_ADC_Stop_DMA(&hadc1); // On coupe l'ADC et le DMA pour éviter le réveil immédiat
	    HAL_UART_DeInit(&huart2);  // On coupe l'UART
	    __HAL_RCC_USART2_CLK_DISABLE();

	    // ── 2. Suspendre le SysTick ──
	    HAL_SuspendTick();
	    CLEAR_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);

	    // ── 3. Entrée en Sleep Mode ──
	    HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);

	    // ══ RÉVEIL PAR LA RTC ICI ══

	    // ── 4. Relancer les périphériques au réveil ──
	    SET_BIT(SysTick->CTRL, SysTick_CTRL_TICKINT_Msk);
	    HAL_ResumeTick();

	    __HAL_RCC_USART2_CLK_ENABLE();
	    Public_MX_USART2_UART_Init();

	    HAL_ADCEx_Calibration_Start(&hadc1);
	    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buffer, 2);
*/


}

  //==================================================================//
 //                       CALLBACKS HAL                              //
//==================================================================//

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
        adc_ready = 1;
}

void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc_local)
{
    __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(hrtc_local, RTC_FLAG_WUTF);
}
