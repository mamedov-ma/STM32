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

#define A    LL_GPIO_PIN_0
#define B    LL_GPIO_PIN_1
#define C    LL_GPIO_PIN_2
#define D    LL_GPIO_PIN_3
#define E    LL_GPIO_PIN_4
#define F    LL_GPIO_PIN_5
#define G    LL_GPIO_PIN_6
#define PD   LL_GPIO_PIN_7
#define POS0 LL_GPIO_PIN_8
#define POS1 LL_GPIO_PIN_9
#define POS2 LL_GPIO_PIN_10
#define POS3 LL_GPIO_PIN_11

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
    LL_GPIO_SetPinMode(GPIOB, A,    LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOB, B,    LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOB, C,    LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOB, D,    LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOB, E,    LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOB, F,    LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOB, G,    LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOB, PD,   LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOB, POS0, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOB, POS1, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOB, POS2, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOB, POS3, LL_GPIO_MODE_OUTPUT);
    
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

    static uint32_t mask = A | B | C | D | E | F | G | PD | POS0 | POS1 | POS2 | POS3;


    uint32_t decoder[16] = {

    A | B | C | D | E | F, //0

    B | C, //1

    A | B | D | E | G, //2

    A | B | C | D | G, //3

    B | C | F | G, //4

    A | C | D | F | G, //5

    A | C | D | E | F | G, //6

    A | B | C,//7

    A | B | C | D | E | F | G, //8

    A | B | C | D | F | G, //9

    A | B | C | E | F | G, //A

    A | B | C | D | E | F | G, //B

    A | D | E | F, //C

    A | B | C | D | E | F, //D

    A | D | E | F | G, //E

    A | E | F | G, //F
    
    };

    uint32_t out = 0;

    uint32_t position[4] = {
    
           POS1 | POS2 | POS3,//first indicator
    POS0 |        POS2 | POS3,//second indicator
    POS0 | POS1 |        POS3,//third indicator
    POS0 | POS1 | POS2,       //fourth indicator 
      
    };
    
    uint32_t num[4] = {
      
    decoder[ number % 10],
    decoder[(number % 100)   / 10],
    decoder[(number % 1000)  / 100],
    decoder[(number % 10000) / 1000],
    };
    
    out = num[digit_num % 4] | position[digit_num % 4];

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

    /*while (1)
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
    } */



    int newnum = number;
    
    while(1)
    {
        dyn_display(newnum, digit_num);
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
                newnum = number + 1;
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
                number = newnum;
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
