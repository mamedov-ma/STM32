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


#define x_max 3
#define y_max 3


static void rcc_config();
static void gpio_config(void);
static void timers_config(void);
static void printf_config(void);
uint8_t sum_x (uint8_t j);
uint8_t sum_y (uint8_t i);
void fill_arr ();
void find_empty ();
void draw ();
void print_krest (enum color_t color);
void print_krug (enum color_t color);
void print_grid ();
void move_cursor_x (uint8_t dir);
void print_win (char winner);
void print_no_winner ();
char check_win ();
int check_full ();
void put_curs ();
int Button_Status();



/*
 * Настраиваем тактирование 
 */
static void rcc_config()
{
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_1);

    LL_RCC_HSI_Enable();
    while (LL_RCC_HSI_IsReady() != 1);

    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSI_DIV_2,
                                LL_RCC_PLL_MUL_12);

    LL_RCC_PLL_Enable();
    while (LL_RCC_PLL_IsReady() != 1);

    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL);

    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);

    SystemCoreClock = 48000000;
}


/*
 * Настраиваем диоды
 */
static void gpio_config(void)
{
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC);
    LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_8, LL_GPIO_MODE_OUTPUT);
    LL_GPIO_SetPinMode(GPIOC, LL_GPIO_PIN_9, LL_GPIO_MODE_OUTPUT);
    return;
}


/*
 * Настраиваем таймер в режиме энкодера
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
    
    LL_TIM_IC_SetActiveInput(TIM2, LL_TIM_CHANNEL_CH1, LL_TIM_ACTIVEINPUT_DIRECTTI); // 1
    LL_TIM_IC_SetActiveInput(TIM2, LL_TIM_CHANNEL_CH2, LL_TIM_ACTIVEINPUT_DIRECTTI); // 1
    LL_TIM_IC_SetPolarity(TIM2, LL_TIM_CHANNEL_CH1, LL_TIM_ETR_POLARITY_NONINVERTED);  // 2
    LL_TIM_IC_SetPolarity(TIM2, LL_TIM_CHANNEL_CH1, LL_TIM_ETR_POLARITY_NONINVERTED);  // 2
    LL_TIM_SetEncoderMode(TIM2, LL_TIM_ENCODERMODE_X4_TI12); // 3
    
    LL_TIM_SetAutoReload(TIM2, 0xFFFF);
    LL_TIM_EnableCounter(TIM2);
    
    return;
}


static void printf_config(void)
{
    xdev_out(oled_putc);
    return;
}

char arr[y_max][x_max] = {'\0'};

int8_t x, y = 0;
uint8_t player_turn = 0;

uint8_t sum_x (uint8_t j)
{
    return ((x + 1) * 2 + x * 16 + 34 + j);
}

uint8_t sum_y (uint8_t i)
{
    return ((y + 1) * 2 + y * 16 + 2 + i);
}


void fill_arr ()
{
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            arr[i][j] = '\0';
}


void find_empty ()
{
    for (y = 0; y < 3; y++)
        for (x = 0; x < 3; x++)
            if (arr[y][x] == '\0')
		return;
}


/*
 * функция рисует поле для игры в каждый момент времени
 */
void draw ()
{
    oled_clr (clBlack);
    print_grid ();

    for (y = 0; y < 3; y++)
        for (x = 0; x < 3; x++)
        {
            if (arr[y][x] == 'x')
                print_krest (clWhite);
            else if (arr[y][x] == 'o')
                print_krug (clWhite);
        }

    oled_update ();
}


/*
 * Данная функция принимает на вход цвет, и рисует им крест
 */
void print_krest (enum color_t color)
{
    for (uint8_t i = 2; i < 16; i++)
        for (uint8_t j = 2; j < 16; j++)
	    {
	        if ( i == j || i == 17 - j)
		    {
			oled_set_pix (sum_x (j), sum_y (i), color);
		    }
	    }
}


/*
 * Данная функция принимает на вход цвет, и рисует им круг 
 */
void print_krug (enum color_t color)
{
    for (uint8_t i = 2; i < 16; i++)
    {
	if (i == 2 || 17 - i == 2)
	    for (uint8_t j = 7; j < 11; j++)
		oled_set_pix (sum_x (j), sum_y (i), color);
	if (i == 3 || 17 - i == 3)
	{
	    for (uint8_t j = 5; j < 7; j++)
		oled_set_pix (sum_x (j), sum_y (i), color);
	    for (uint8_t j = 11; j < 13; j++)
                oled_set_pix (sum_x (j), sum_y (i), color);
        }
	if (i == 4 || 17 - i == 4)
	{
	    uint8_t j = i;
                oled_set_pix (sum_x (j), sum_y (i), color);
	    j = 17 - i;
	        oled_set_pix (sum_x (j), sum_y (i), color);
	}
    }

    for (uint8_t j = 2; j < 16; j++)
    {
        if (j == 2 || 17 - j == 2)
            for (uint8_t i = 7; i < 11; i++)
                oled_set_pix (sum_x (j), sum_y (i), color);
        if (j == 3 || 17 - j == 3)
        {
            for (uint8_t i = 5; i < 7; i++)
                oled_set_pix (sum_x (j), sum_y (i), color);
            for (uint8_t i = 11; i < 13; i++)
                oled_set_pix (sum_x (j), sum_y (i), color);
        }
    }

}


/*
 * Данная функция отрисовывает решетку на экране
 */
void print_grid ()
{
    for (uint8_t i = 3; i < 59; i++)
        for (uint8_t j = 35; j < 91; j++)
            {
                if ((((i - 3) % 18) == 0) || (((i - 4) % 18) == 0 ))
                    oled_set_pix (j, i, clWhite);
                if ((((j - 35) % 18) == 0) || (((j - 36) % 18) == 0 ))
                    oled_set_pix (j, i, clWhite);
            }
}


/*
 * Данная функция сдвигает курсор на dir клеток
 */
void move_cursor_x (uint8_t dir)
{
    while (1)
    {
	x += dir;

	if (x > 2)
	{
            y += 1;
	    x = 0;

	    if (y > 2)
	        y = 0;
	}	

	if (x < 0)
	{
            x = 2;
	    y -= 1;
	    if (y < 0)
		y = 2;
	}

        if (arr[y][x] == '\0')
	{
	    return;
	}
    }
}


/*
 * Данная функция выводит на экран поздравления победившему игроку
 */
void print_win (char winner)
{
    oled_clr (clBlack);
    oled_set_cursor (0, 0);
    xprintf ("     Player %c won\n", winner);
    xprintf ("       the game!\n");
    xprintf ("     If you want to\n");
    xprintf ("    play again press\n");
    xprintf ("         Reset.\n");
    oled_update ();
}


/*
 * Данная функция выводит на экран сообщение о том, что у игроков ничья
 */
void print_no_winner ()
{
    oled_clr (clBlack);
    oled_set_cursor (0, 0);
    xprintf ("         Draw!\n");
    xprintf ("     If you want to\n");
    xprintf ("    play again press\n");
    xprintf ("         Reset.\n");
    oled_update ();
}


/*
 * Проверка на то, что крайний ход являлся победным
 */
char check_win ()
{
    uint8_t j = 0;
    uint8_t i = 0;

    char symb = '\0';

    for (i = 0; i < 3; i++)
    {
        if ((symb = arr[i][j]) == arr[i][j + 1] && arr[i][j] == arr[i][j + 2] && (symb == 'x' || symb == 'o'))
	{
            print_win (arr[i][j]);
	    return symb;
	}
    }

    i = 0;
    for (j = 0; j < 3; j++)
    {
        if ((symb = arr[i][j]) == arr[i + 1][j] && arr[i][j] == arr[i + 2][j] && (symb == 'x' || symb == 'o'))
	{
            print_win (arr[i][j]);
	    return symb;
	}
    }

    if ((symb = arr[0][0]) == arr[1][1] && arr[0][0] == arr[2][2] && (symb == 'x' || symb == 'o'))
    {
        print_win (arr[0][0]);
	return symb;
    }

    if ((symb = arr[0][2]) == arr[1][1] && arr[1][1] == arr[2][0] && (symb == 'x' || symb == 'o'))
    {
        print_win (arr[1][1]);
	return symb;
    }

    return '\0';
}


/*
 * Проверка на ничью
 */
int check_full ()
{
    for (uint8_t i = 0; i < 3; i++)
        for (uint8_t j = 0; j < 3; j++)
            {
                if (arr[i][j] == '\0')
                    return 0;
            }
   
    return 1;
}


/*
 * Отображает курсор
 */
void put_curs ()
{
    for (uint8_t i = 0; i < 18; i++)
	for (uint8_t j = 0; j < 18; j++)
            oled_set_pix (sum_x (j), sum_y (i), clWhite);

    if (player_turn)
        print_krest (clBlack);
    else
	print_krug (clBlack);

    oled_update ();

    return;
}


/*
 * Возвращает 1 - если кнопка нажата, 0 - если нет
 */
int button_status, debouncer_clk = 0;
int Button_Status()
{ 
    if (LL_GPIO_IsInputPinSet(GPIOA, LL_GPIO_PIN_0)) 
    {
        button_status = 1;
        debouncer_clk = 0;
    }
    
    if (button_status)
    {
        debouncer_clk++;
    }
    
    if (debouncer_clk >= 5)
    {
        button_status = 0;
        debouncer_clk = 0;
    }    

    return button_status;
}




int main(void)
{
    rcc_config();
    gpio_config();
    oled_config();
    timers_config();
    printf_config();

    fill_arr ();

    int check = 0;
    char check_end = 0;

    x = 0;                                         // 
    y = 0;                                         //
    int X,Y = 0;                                   //
    int ch = 0;                                    //
    int old_status = 0;                            //
    int new_counter = 0;                           //
    int old_counter = LL_TIM_GetCounter(TIM2);     //  
    uint8_t x_save = 0;                            //
    uint8_t y_save = 0;                            // 
    int check_no_winner = 0;                       //
    
    while (1) 
    {
	draw ();
	
	x = x_save;
	y = y_save;

	if (check == 0)
            find_empty ();          

        put_curs ();
 
        new_counter = LL_TIM_GetCounter(TIM2);
 
        if (new_counter - old_counter >= 10)
        {
	    move_cursor_x (1);
	    check = 1;
	    old_counter = LL_TIM_GetCounter(TIM2);
        }
	else if (new_counter - old_counter <= -10) 
	{
	    move_cursor_x (-1);
	    check = 1;
	    old_counter = LL_TIM_GetCounter(TIM2);
        }	
       

	//if button pressed, then put 'x', or 'o' to the xurrent place, and change turn	
	
	if(Button_Status() == 0)
	{
	    X = x;
	    Y = y;	    
	}
	
	
	if(old_status == 0 && Button_Status() == 1)  //
	{                                            // 
	    if(player_turn == 0)                     //
	    {                                        //
	    arr[Y][X]   = 'o';                       //
	    player_turn = 1;                         //
	    }                                        //
	    else if(player_turn == 1)                //  Обработка хода
	    {                                        // 
	    arr[Y][X]   = 'x';                       //
	    player_turn = 0;                         //
	    }                                        //
	                                             //
	    check = 0;                               //
	}                                            
        
        old_status = Button_Status();
        
        x_save = x;
        y_save = y;
	
	check_end = check_win ();

	if (check_end != '\0')                       //
	                                             //  
	    break;                                   //
	                                             //  Проверка на то, был ли ход финальным
        check_no_winner = check_full ();             //
        if (check_no_winner != 0)                    //
            break;                                   //
    }                                                

    
    //Окно окончания игры
    while (1)
    {
        if (check_end != '\0')
            print_win (check_end);
        if (check_no_winner != 0)
            print_no_winner ();
  
        LL_GPIO_TogglePin(GPIOC, LL_GPIO_PIN_8);
        LL_GPIO_TogglePin(GPIOC, LL_GPIO_PIN_9);
    }

    return 0;
}
