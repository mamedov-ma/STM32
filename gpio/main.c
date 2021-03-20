#include<stdio.h>

#include "stm32f0xx_ll_adc.h"
#include "stm32f0xx_ll_rcc.h"
#include "stm32f0xx_ll_system.h"
#include "stm32f0xx_ll_bus.h"
#include "stm32f0xx_ll_gpio.h"

/*
* This example is a simple hexadecimal counter
* Push the USER button and the number on the indicator increases
* up to 0xF and then resets to 0x0
* If you want to see how it works w/o debouncing
* delete the following line:
* #define TURN_ON_CONTACT_DEBOUNCER
*/


#define TURN_ON_CONTACT_DEBOUNCER

/**
* System Clock Configuration
* The system Clock is configured as follow :
* System Clock source = PLL (HSI/2)
* SYSCLK(Hz) = 48000000
* HCLK(Hz) = 48000000
* AHB Prescaler = 1
* APB1 Prescaler = 1
* HSI Frequency(Hz) = 8000000
* PLLMUL = 12
* Flash Latency(WS) = 1
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
    //SystemCoreClock = 48000000;
}

/*
* Clock on GPIOC and set two led pins
*/

static void gpio_config(void)
{
    /*
    * Init two default LEDs
    */
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC);
    LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_8, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_9, LL_GPIO_MODE_OUTPUT);
    
    /*
    * Init port for indicator
    */
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_0, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_1, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_2, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_3, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_4, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_5, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_6, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_7, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_8, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_9, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_10, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOB, LL_GPIO_PIN_11, LL_GPIO_MODE_OUTPUT);
    
    /*
    * Init port for USER button
    */
    
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);
    LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_0, LL_GPIO_MODE_INPUT);
    
    return;
}


/*
* Just set of commands to waste CPU power for 10ms
* (basically it is a simple cycle with a predefined number
* of loops)
*/
__attribute__((naked)) static void delay_10ms(void)
{
    /*asm ("push {r7, lr}");
    asm ("ldr r6, [pc, #8]");
    asm ("sub r6, #1");
    asm ("cmp r6, #0");
    asm ("bne delay_10ms+0x4");
    asm ("pop {r7, pc}");
    asm (".word 0xea60"); //60000*/

    for (int i = 0; i < 5000; i++);
}


void dyn_display(uint16_t number, int digit_num)
{

    static uint32_t mask = LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_2 | \
    LL_GPIO_PIN_3 | LL_GPIO_PIN_4 | LL_GPIO_PIN_5 | LL_GPIO_PIN_6 | LL_GPIO_PIN_7 | LL_GPIO_PIN_8 | LL_GPIO_PIN_9 | \
    LL_GPIO_PIN_10 | LL_GPIO_PIN_11;


    uint32_t decoder[16] = {

    LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_2 | LL_GPIO_PIN_3 | \
    LL_GPIO_PIN_4 | LL_GPIO_PIN_5, //0

    LL_GPIO_PIN_1 | LL_GPIO_PIN_2, //1

    LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_6 | LL_GPIO_PIN_4 | \
    LL_GPIO_PIN_3, //2

    LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_6 | LL_GPIO_PIN_2 | \
    LL_GPIO_PIN_3, //3

    LL_GPIO_PIN_5 | LL_GPIO_PIN_6 | LL_GPIO_PIN_1 |
 
    LL_GPIO_PIN_2, //4

    LL_GPIO_PIN_0 | LL_GPIO_PIN_2 | LL_GPIO_PIN_3 | LL_GPIO_PIN_5 | \
    LL_GPIO_PIN_6, //5

    LL_GPIO_PIN_0 | LL_GPIO_PIN_2 | LL_GPIO_PIN_3 | LL_GPIO_PIN_4 | \
    LL_GPIO_PIN_5 | LL_GPIO_PIN_6, //6

    LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_2 | LL_GPIO_PIN_11,//7

    LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_2 | LL_GPIO_PIN_3 | \
    LL_GPIO_PIN_4 | LL_GPIO_PIN_5 | LL_GPIO_PIN_6, //8

    LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_2 | LL_GPIO_PIN_3 | \
    LL_GPIO_PIN_5 | LL_GPIO_PIN_6, //9

    LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_2 | LL_GPIO_PIN_4 | \
    LL_GPIO_PIN_5 | LL_GPIO_PIN_6, //10

    LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_2 | LL_GPIO_PIN_3 | \
    LL_GPIO_PIN_4 | LL_GPIO_PIN_5 | LL_GPIO_PIN_6, //11

    LL_GPIO_PIN_0 | LL_GPIO_PIN_3 | LL_GPIO_PIN_4 | LL_GPIO_PIN_5, //12

    LL_GPIO_PIN_0 | LL_GPIO_PIN_1 | LL_GPIO_PIN_2 | LL_GPIO_PIN_3 | \
    LL_GPIO_PIN_4 | LL_GPIO_PIN_5, //13

    LL_GPIO_PIN_0 | LL_GPIO_PIN_3 | LL_GPIO_PIN_4 | LL_GPIO_PIN_5 | \
    LL_GPIO_PIN_6, //14

    LL_GPIO_PIN_0 | LL_GPIO_PIN_4 | LL_GPIO_PIN_5 | LL_GPIO_PIN_6, //15

    //[16] =
    };


    //LL_GPIO_WriteOutputPort(GPIOC, ...);

    uint32_t out = 0;




    switch (digit_num % 4)
    {
    case 0:
        //LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_8);

        out = decoder[number % 10] | LL_GPIO_PIN_9 | \
        LL_GPIO_PIN_10 | LL_GPIO_PIN_11;
        break;

    case 1:
        //LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_9);

        out = decoder[(number % 100) / 10] | LL_GPIO_PIN_8 | \
        LL_GPIO_PIN_10 | LL_GPIO_PIN_11 | LL_GPIO_PIN_7;
        break;

    case 2:
        //LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_10);

        out = decoder[(number % 1000) / 100] | LL_GPIO_PIN_8 | \
        LL_GPIO_PIN_9 | LL_GPIO_PIN_11;
        break;

    case 3:
        //LL_GPIO_SetOutputPin(GPIOB, LL_GPIO_PIN_11);

        out = decoder[(number % 10000) / 1000] | LL_GPIO_PIN_8 | \
        LL_GPIO_PIN_9 | LL_GPIO_PIN_10 | LL_GPIO_PIN_7;
        break;

    default:
        break;
    }

    uint32_t port_state = 0;

    port_state = LL_GPIO_ReadOutputPort(GPIOB);

    port_state = (port_state & ~mask) | out;

    LL_GPIO_WriteOutputPort(GPIOB, port_state);

    digit_num = (digit_num + 1) % 4;

    return;
}







int main(void)
{
    rcc_config();
    gpio_config();

    uint16_t number = 0;
    int digit_num, time_running = 1;

    #if defined(TURN_ON_CONTACT_DEBOUNCER)
    /*
    * For contact debouncer algorithm 
    */
    uint32_t debouncer_clk = 0;
    uint32_t button_pressed = 0;
    #endif

    while (1)
    {
        for (int i = 0; i < 72; i++)
        {
            dyn_display((number / 600) * 1000 + number % 600, digit_num);
            digit_num++;
            delay_10ms();

            if (LL_GPIO_IsInputPinSet(GPIOA, LL_GPIO_PIN_0)) 
            {
                button_pressed = 1;
                debouncer_clk = 0;
            } 

            if (button_pressed)
            {
                debouncer_clk++;
                // delay_10ms();
            }

            if (debouncer_clk >= 5)
            {
                LL_GPIO_TogglePin(GPIOC, LL_GPIO_PIN_8);
                time_running = ~time_running;
                //set _indicator(number);
                button_pressed = 0;
                debouncer_clk = 0;
            }
        }

        if(time_running == 1)
        {
            ++number;
         }
    } 




while(1)
{
    dyn_display(number, digit_num);
    digit_num ++;
    delay_10ms();


    #if defined(TURN_ON_CONTACT_DEBOUNCER)

        /*
        * if button is pressed then set flag and reset counter
        */
        if (LL_GPIO_IsInputPinSet(GPIOA, LL_GPIO_PIN_0)) 
        {
            button_pressed = 1;
            debouncer_clk = 0;
        }
        
        /*
        * if flag is set increase counter
        */
        if (button_pressed)
        {
            debouncer_clk++;
            // delay_10ms();
        }
    
        /*
        * If counter manages to count up to 5 then button is not bouncing
        * any longer and we need to get action! (process it)
        * NOTE: 5 is just experimental value
        */
        if (debouncer_clk >= 5)
        {
            LL_GPIO_TogglePin(GPIOC, LL_GPIO_PIN_8);
            number++;
            //set_indicator(number);
            button_pressed = 0;
            debouncer_clk = 0;
        }
    
    #else
        if (LL_GPIO_IsInputPinSet(GPIOA, LL_GPIO_PIN_0))
        {
            LL_GPIO_TogglePin(GPIOC, LL_GPIO_PIN_8);
            number++;
            //set_indicator(number);
        }
   
    #endif
    }


    return 0;
}