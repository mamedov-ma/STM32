/*
 * This example demonstrates using timers with encoder
 */

#include "stm32f0xx_ll_rcc.h"
#include "stm32f0xx_ll_system.h"
#include "stm32f0xx_ll_bus.h"
#include "stm32f0xx_ll_gpio.h"
#include "stm32f0xx_ll_tim.h"
#include "xprintf.h"
#include "oled_driver.h"
#include "stm32f0xx_ll_exti.h"
#include "stm32f0xx_ll_utils.h"
#include "stm32f0xx_ll_cortex.h"





/**
  * System Clock Configuration
  * The system Clock is configured as follow :
  *    System Clock source            = PLL (HSI/2)
  *    SYSCLK(Hz)                     = 48000000
  *    HCLK(Hz)                       = 48000000
  *    AHB Prescaler                  = 1
  *    APB1 Prescaler                 = 1
  *    HSI Frequency(Hz)              = 8000000
  *    PLLMUL                         = 12
  *    Flash Latency(WS)              = 1
  */
static void rcc_config()
{
    /* Set FLASH latency */
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);

    /* Enable HSI and wait for activation*/
    LL_RCC_HSI_Enable();
    while (LL_RCC_HSI_IsReady() != 1);

    /* Main PLL configuration and activation */
    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI_DIV_2,
                                LL_RCC_PLL_MUL_12);

    LL_RCC_PLL_Enable();
    while (LL_RCC_PLL_IsReady() != 1);

    /* Sysclk activation on the main PLL */
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL);

    /* Set APB1 prescaler */
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);

    /* Update CMSIS variable (which can be updated also
     * through SystemCoreClockUpdate function) */
    SystemCoreClock = 48000000;
}

/*
 * Clock on GPIOC and set two led pins
 */
static void gpio_config(void)
{
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC);
    LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_8, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_9, LL_GPIO_MODE_OUTPUT);
    return;
}

/*
 * Configure timer to encoder mode
 */
static void timers_config(void)
{
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_1, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetPinMode(GPIOA, LL_GPIO_PIN_5, LL_GPIO_MODE_ALTERNATE);
    LL_GPIO_SetAFPin_0_7(GPIOA, LL_GPIO_PIN_1, LL_GPIO_AF_2);
    LL_GPIO_SetAFPin_0_7(GPIOA, LL_GPIO_PIN_5, LL_GPIO_AF_2);
    LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_1, LL_GPIO_PULL_UP);
    LL_GPIO_SetPinPull(GPIOA, LL_GPIO_PIN_5, LL_GPIO_PULL_UP);

    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);
    /* (1) Configure TI1FP1 on TI1 (CC1S = 01)
         configure TI1FP2 on TI2 (CC2S = 01) */
    /* (2) Configure TI1FP1 and TI2FP2 non inverted (CC1P = CC2P = 0, reset value) */
    /* (3) Configure both inputs are active on both rising and falling edges
        (SMS = 011) */ 
    LL_TIM_IC_SetActiveInput(TIM2, LL_TIM_CHANNEL_CH1, LL_TIM_ACTIVEINPUT_DIRECTTI); // 1
    LL_TIM_IC_SetActiveInput(TIM2, LL_TIM_CHANNEL_CH2, LL_TIM_ACTIVEINPUT_DIRECTTI); // 1
    LL_TIM_IC_SetPolarity(TIM2, LL_TIM_CHANNEL_CH1, LL_TIM_ETR_POLARITY_NONINVERTED);  // 2
    LL_TIM_IC_SetPolarity(TIM2, LL_TIM_CHANNEL_CH1, LL_TIM_ETR_POLARITY_NONINVERTED);  // 2
    LL_TIM_SetEncoderMode(TIM2, LL_TIM_ENCODERMODE_X4_TI12); // 3
    //LL_TIM_IC_SetFilter(TIM2, LL_TIM_CHANNEL_CH1, LL_TIM_IC_FILTER_FDIV16_N5);
    //LL_TIM_IC_SetFilter(TIM2, LL_TIM_CHANNEL_CH2, LL_TIM_IC_FILTER_FDIV16_N5);
    LL_TIM_SetAutoReload(TIM2, 0xFFFF);
    LL_TIM_EnableCounter(TIM2);
    //
    return;
}

/*---------------------------------------------*/
/*
 * Configure system timer to time 1 ms
 */
/*static void systick_config(void)
{
    LL_InitTick(48000000, 1000);
    LL_SYSTICK_EnableIT();
    NVIC_SetPriority(SysTick_IRQn, 1);
    return;
}*/
/*
 * Handler for system timer
 */
/*void SysTick_Handler(void)
{
    
    if (LL_TIM_GetCounterMode(TIM2) == LL_TIM_COUNTERMODE_UP) 
    {
            LL_GPIO_SetOutputPin(GPIOC, LL_GPIO_PIN_8);
            LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_9);
    }
    if (LL_TIM_GetCounterMode(TIM2) == LL_TIM_COUNTERMODE_DOWN) 
    {
            LL_GPIO_SetOutputPin(GPIOC, LL_GPIO_PIN_9);
            LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_8);
    }
        
    oled_clr(clBlack);
    oled_set_cursor(0,0);
    xprintf("%d", (uint32_t)(LL_TIM_GetCounter(TIM2) / 95 * 256));
    oled_update(); 
    return;
}*/









/*
 * Just set of commands to waste CPU power for a second
 * (basically it is a simple cycle with a predefined number
 * of loops)
 */
static void delay(void)
{
    for (int i = 0; i < 10000; i++);
}

/*
 * Turn on Green led when turn encoder to the right
 * Turn on Blue led when turn encoder to the left
 */
 
 
 static void printf_config(void)
{
    xdev_out(oled_putc);
    return;X
}


int main(void)
{
    rcc_config();
    gpio_config();
    oled_config();
    printf_config();
    timers_config();
    //systick_config();
    


    int i = 0;
    
    while (1) 
    {
        /*if (LL_TIM_GetCounterMode(TIM2) == LL_TIM_COUNTERMODE_UP) {
            LL_GPIO_SetOutputPin(GPIOC, LL_GPIO_PIN_8);
            LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_9);
        }
        if (LL_TIM_GetCounterMode(TIM2) == LL_TIM_COUNTERMODE_DOWN) {
            LL_GPIO_SetOutputPin(GPIOC, LL_GPIO_PIN_9);
            LL_GPIO_ResetOutputPin(GPIOC, LL_GPIO_PIN_8);
        }
        */
        
        oled_clr(clBlack);
        oled_set_cursor(0,0);
        xprintf("%d", LL_TIM_GetCounterMode(TIM2));
        oled_update();
        
        
        for(int i = 0; i < 10; i++)
            delay();
            
        i++;
        if (i == 10)
            break; 
    }   
    return 0;
}
