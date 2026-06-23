/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Concurrent Multi-Frequency Waveform Generation via
  * Dynamic Timer Output Compare & AFIO Remapping
  ******************************************************************************
  * @attention
  *
  * This firmware demonstrates advanced hardware instrumentation principles on the
  * STM32F103 microcontroller, utilizing a single 16-bit timer (TIM2) to generate
  * up to four completely independent, concurrent square wave frequencies.
  *
  * ============================================================================
  * CLOCK & TIMEBASE CONFIGURATION
  * ============================================================================
  * - System Clock (SYSCLK)      = 48 MHz (Via PLL, sourcing HSE)
  * - AHB Bus Prescaler          = 1 (48 MHz)
  * - APB1 Peripheral Prescaler  = 4 (12 MHz Clock / 24 MHz Timer Base Clock)
  *
  * *Note:* Due to hardware design rules on the APB1 bus, when the prescaler is not 1,
  * a hardware x2 multiplier is applied to the timer clocks. Consequently, TIM2
  * runs on a dedicated **24 MHz internal clock timebase**.
  *
  * ============================================================================
  * HARDWARE PIN SCHEDULING & AFIO REMAPPING (PARTIAL REMAP 2)
  * ============================================================================
  * To prevent pin contention with USART1 (TX/RX pins on PA2/PA3), an Alternate
  * Function I/O (AFIO) partial remapping strategy is explicitly applied:
  *
  * - TIM2_CH1  ->  Pin PA0 (Output Compare Toggle Mode)
  * - TIM2_CH2  ->  Pin PA1 (Output Compare Toggle Mode)
  * - TIM2_CH3  ->  Pin PB10 (Output Compare Toggle Mode)
  * - TIM2_CH4  ->  Pin PB11 (Output Compare Toggle Mode)
  *
  * ============================================================================
  * SOFTWARE TIMING MECHANISM (DYNAMIC PULSE SHIFTING)
  * ============================================================================
  * The master counter register (`TIM2->CNT`) is configured to run continuously from
  * 0 to 0xFFFF without resetting (`htimer2.Init.Period = 0xFFFF`).
  * * When a channel's comparison register (`TIM2->CCRx`) matches the counter:
  * 1. The micro's internal hardware automatically toggles the physical GPIO pin.
  * 2. An interrupt fires (`HAL_TIM_OC_DelayElapsedCallback`).
  * 3. The ISR calculates the target for the *next* edge by reading the current match
  * point and sliding it forward using the low-level `__HAL_TIM_SET_COMPARE` macro.
  * 4. 16-bit unsigned overflow is handled transparently by the CPU architecture.
  *
  ******************************************************************************
  */

#include "stm32f1xx_hal.h"
#include "main_app.h"
#include "msp.h"
#include "it.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>

// RCC OSC struct. declaration
RCC_OscInitTypeDef osc_init;

// CLK Config struct. declaration
RCC_ClkInitTypeDef clk_init;


TIM_HandleTypeDef htimer2;


uint32_t pulse1_value = 24000;//26000; // to produce 500Hz

uint32_t pulse2_value = 12000;//13000; // to produce 1KHz

uint32_t pulse3_value = 6000; // to produce 2KHz

uint32_t pulse4_value = 3000; // to produce 4KHz

uint32_t ccr_content;


int main(void){

	// HAL library inits.
	HAL_Init();

	// SYSCLK configuration
	SYSCLK_Config();


	// Timer 2 inits.

	TIMER2_Init() ;


	// start timer 2 in interrupt mode
	if ( HAL_TIM_OC_Start_IT(&htimer2,TIM_CHANNEL_1) != HAL_OK) {

		Error_handler();
	}

	if ( HAL_TIM_OC_Start_IT(&htimer2,TIM_CHANNEL_2) != HAL_OK) {

		Error_handler();
	}

	if ( HAL_TIM_OC_Start_IT(&htimer2,TIM_CHANNEL_3) != HAL_OK) {

		Error_handler();
	}

	if ( HAL_TIM_OC_Start_IT(&htimer2,TIM_CHANNEL_4) != HAL_OK) {

		Error_handler();
	}


	while(1) ;


    return 0 ;

}


void SYSCLK_Config(void) {

	// 1. Enable HSI SYSCLK and configure it as source clock

		memset(&osc_init, 0, sizeof(osc_init));

		osc_init.OscillatorType = RCC_OSCILLATORTYPE_HSE;

		osc_init.HSEState =  RCC_HSE_BYPASS;

		osc_init.PLL.PLLSource = RCC_PLLSOURCE_HSE ;

		osc_init.PLL.PLLState = RCC_PLL_ON;


		// 2 . Configure AHB , APB1 AND APB2 Prescalers

		memset(&clk_init, 0, sizeof(clk_init));

		clk_init.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK \
							| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 ;


		clk_init.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK ;

		osc_init.PLL.PLLMUL =  RCC_PLL_MUL9;


		osc_init.PLL.PLLMUL =  RCC_PLL_MUL6;

		clk_init.AHBCLKDivider = RCC_SYSCLK_DIV1 ;

		clk_init.APB1CLKDivider = RCC_HCLK_DIV4 ;

		clk_init.APB2CLKDivider = RCC_HCLK_DIV4 ;


		if (HAL_RCC_OscConfig(&osc_init) != HAL_OK) {

			 // there is a problem
					 Error_handler();

		}

		if(HAL_RCC_ClockConfig(&clk_init, FLASH_ACR_LATENCY_1) != HAL_OK) {

			Error_handler();
		};

		__HAL_RCC_HSI_DISABLE();

		// Sysclk configuration

		HAL_SYSTICK_Config( HAL_RCC_GetSysClockFreq()/1000);

		HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

}

// Timer 2 parameter inits.

void TIMER2_Init(void) {

	TIM_OC_InitTypeDef tim2OC_init = {0};

	// initialize the output compare time base unit

	htimer2.Instance = TIM2;
	htimer2.Init.Prescaler = 0;
	htimer2.Init.Period = 0xFFFF;
	if (HAL_TIM_OC_Init(&htimer2)!= HAL_OK )
	{
		Error_handler();
	}

	// --- Shared Channel Hardware Configuration Parameters ---

	tim2OC_init.OCMode = TIM_OCMODE_TOGGLE;
	tim2OC_init.OCPolarity =  TIM_OCPOLARITY_HIGH;
	tim2OC_init.Pulse = pulse1_value ;

	// configure output compare channel 1

	if(HAL_TIM_OC_ConfigChannel(&htimer2, &tim2OC_init,TIM_CHANNEL_1) != HAL_OK) {

		 Error_handler();
	}

	tim2OC_init.Pulse = pulse2_value ;

	// configure output compare channel 2

	if(HAL_TIM_OC_ConfigChannel(&htimer2, &tim2OC_init,TIM_CHANNEL_2) != HAL_OK) {

			 Error_handler();
		}


	tim2OC_init.Pulse = pulse3_value ;
	// configure output compare channel 3

	if(HAL_TIM_OC_ConfigChannel(&htimer2, &tim2OC_init,TIM_CHANNEL_3) != HAL_OK) {

			 Error_handler();
		}


	tim2OC_init.Pulse = pulse4_value ;
	// configure output compare channel 4

	if(HAL_TIM_OC_ConfigChannel(&htimer2, &tim2OC_init,TIM_CHANNEL_4) != HAL_OK) {

			 Error_handler();
		}


}



void Error_handler(void){

	// Infinite loop if error occurs, blinking a led can be used too here instead

	while(1);

}


void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim){

	/* TIM2_CH1 toggling with frequency = 500Hz (Toggle every 24,000 ticks)*/
	if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {

		ccr_content = __HAL_TIM_GET_COMPARE(htim, TIM_CHANNEL_1);
		__HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_1 ,(ccr_content + pulse1_value) & 0xFFFF);

	}

	/* TIM2_CH2 toggling with frequency = 1KHz (Toggle every 12,000 ticks)*/

	if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) {
		ccr_content = __HAL_TIM_GET_COMPARE(htim, TIM_CHANNEL_2);
		__HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_2 ,(ccr_content + pulse2_value) & 0xFFFF);
	}

	/* TIM2_CH3 toggling with frequency = 2KHz (Toggle every 6,000 ticks) */

	if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3) {
		ccr_content = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_3);
		__HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_3 ,(ccr_content + pulse3_value) & 0xFFFF);
	}

	/* TIM2_CH4 toggling with frequency = 4KHz (Toggle every 3,000 ticks) */

	if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_4) {
		ccr_content = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_4);
		__HAL_TIM_SET_COMPARE(htim, TIM_CHANNEL_4 ,(ccr_content + pulse4_value) & 0xFFFF);
	}

}
