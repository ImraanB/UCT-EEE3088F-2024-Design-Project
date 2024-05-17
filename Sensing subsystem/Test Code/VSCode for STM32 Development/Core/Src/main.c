//********************************************************************
//*              EEE3088F sensing subsytem test code                 *
//*==================================================================*
//* WRITTEN BY: Jesse Arendse   	                 		         *
//* DATE CREATED: 07/04/2023                                         *
//* MODIFIED BY: Imraan Banderker                                    *
//* DATE MODIFIED: 07/05/2024                                        *
//*==================================================================*
//* PROGRAMMED IN: Visual Studio Code                                *
//* TARGET:        STM32F0                                           *
//*==================================================================*
//* DESCRIPTION: Test sensing functionality of the sensing module    *
//*                                                                  *
//********************************************************************
// INCLUDE FILES
//====================================================================
#include <stdio.h>
#include <stdbool.h>
#define STM32F051
#include <stm32f0xx.h>
#include <stm32f051x8.h>
#include "lcd_stm32f0.c"
//====================================================================
// GLOBAL CONSTANTS
//====================================================================

//====================================================================
// GLOBAL VARIABLES
//====================================================================
uint16_t ADC_value_left;
uint16_t ADC_value_front;
uint16_t ADC_value_right;

bool holdOnFront = true;    // true: IR remains on if front wall is present, false: IR always pulses 
bool steadyLED = true;          
// true: LEDs change state only when IR is present 
// false: LEDs turn off when IR light turns off

uint8_t displayADC = 6;         // choose to disply ADC value for: left=7 front=6 right=5 none=0
uint16_t frontThreshold = 2400;
uint16_t sideThreshold = 2400;

//====================================================================
// FUNCTION DECLARATIONS
//====================================================================
void init_GPIOA(void);
void init_GPIOB(void); 
void init_ADC(void); 
void adcLCDdisplay(uint16_t number);
void init_TIM14(void);
void TIM14_IRQHandler (void);
void leftLED(void);
void frontLED(void);
void rightLED(void);

//====================================================================
// MAIN FUNCTION
//====================================================================
int main(void) {
    init_GPIOA();
    init_GPIOB();
    init_ADC();
    init_LCD();
    init_TIM14();


    while(1) {
        /*
            read ADC for left sensor
        */
        ADC1->CHSELR  |=  ADC_CHSELR_CHSEL0;  // select ADC channel 0(for ADC_IN0 = PA0)
        ADC1->CR  |=  ADC_CR_ADSTART;  // start conversion
        while( (ADC1->ISR & ADC_ISR_EOC) == 0 );  // wait until conversion is complete
        ADC_value_left = ADC1->DR;
        ADC1->CHSELR  &= ~ADC_CHSELR_CHSEL0;  // deselect ADC channel 0
        if(displayADC==7){
            adcLCDdisplay(ADC_value_left);      // display conversion result on the LCD
        }
        if(steadyLED){
            leftLED();
        }else{
            if (ADC_value_left<sideThreshold){
                GPIOB -> ODR |= GPIO_ODR_7;     //turn PB7 led on

            }else{
                GPIOB -> ODR &= ~GPIO_ODR_7;    //turn PB7 led off
            }
        }
        
        /*
            read ADC for front sensor
        */
        ADC1->CHSELR  |=  ADC_CHSELR_CHSEL1;  // select ADC channel 1(for ADC_IN1 = PA1)
        ADC1->CR  |=  ADC_CR_ADSTART;  // start conversion
        while( (ADC1->ISR & ADC_ISR_EOC) == 0 );  // wait until conversion is complete
        ADC_value_front = ADC1->DR;
        ADC1->CHSELR  &= ~ADC_CHSELR_CHSEL1; // deselect ADC channel 1

        if(displayADC==6){
            adcLCDdisplay(ADC_value_front);      // display conversion result on the LCD
        }

        if(steadyLED){
            frontLED();
        }else{
            if (ADC_value_front < frontThreshold){
                GPIOB -> ODR |= GPIO_ODR_6;     //turn PB6 led on
                if(holdOnFront){TIM14 -> CR1 &= ~TIM_CR1_CEN;}    // stop counter for TIM14

            }else{
                GPIOB -> ODR &= ~GPIO_ODR_6;    //turn PB6 led off
                if(holdOnFront){TIM14 -> CR1 |= TIM_CR1_CEN;}     //start counter for TIM14
            }
        }

        /*
            read ADC for right sensor
        */
        ADC1->CHSELR  |=  ADC_CHSELR_CHSEL2;  // select ADC channel 2(for ADC_IN2 = PA2)
        ADC1->CR  |=  ADC_CR_ADSTART;  // start conversion
        while( (ADC1->ISR & ADC_ISR_EOC) == 0 );  // wait until conversion is complete
        ADC_value_right = ADC1->DR;
        ADC1->CHSELR  &= ~ADC_CHSELR_CHSEL2; // deselect ADC channel 2

        if(displayADC==5){
            adcLCDdisplay(ADC_value_right);      // display conversion result on the LCD
        }

        if(steadyLED){
            rightLED();
        }else{
            if (ADC_value_right < sideThreshold){
                GPIOB -> ODR |= GPIO_ODR_5;     //turn PB5 led on

            }else{
                GPIOB -> ODR &= ~GPIO_ODR_5;    //turn PB5 led off
            }
        }
               
    }
} 

//====================================================================
// FUNCTION DEFINITIONS
//====================================================================
void init_GPIOA(void) { 
    RCC -> AHBENR |= RCC_AHBENR_GPIOAEN; // enable clock signal to Port A
    GPIOA -> MODER |= GPIO_MODER_MODER0;  // configure pin PA0 to analog input mode
    GPIOA -> MODER |= GPIO_MODER_MODER1;  // configure pin PA1 to analog input mode
    GPIOA -> MODER |= GPIO_MODER_MODER2;  // configure pin PA2 to analog input mode
    GPIOA -> MODER |= GPIO_MODER_MODER3_0;  // configure pin PA3 to output mode

} 

void init_GPIOB(void){
    RCC -> AHBENR |= RCC_AHBENR_GPIOBEN; // enable clock to GPIOB
    GPIOB -> MODER |= GPIO_MODER_MODER5_0;  // set PB5 to output mode
    GPIOB -> MODER |= GPIO_MODER_MODER6_0;  // set PB6 to output mode
    GPIOB -> MODER |= GPIO_MODER_MODER7_0;  // set PB7 to output mode

    GPIOB -> MODER |= GPIO_MODER_MODER10_0;  // set PB10 to output mode

    GPIOB -> ODR &= ~GPIO_ODR_5;
    GPIOB -> ODR &= ~GPIO_ODR_6;
    GPIOB -> ODR &= ~GPIO_ODR_7;
    GPIOB -> ODR &= ~GPIO_ODR_10;
    
}

void init_ADC(void) { 
    ADC1->CR &= ~ADC_CR_ADEN;  // ensure ADC is disabled before calibration
    ADC1->CR |= ADC_CR_ADCAL;  // initiate ADC calibration

    RCC->APB2ENR  |=  RCC_APB2ENR_ADCEN;  // connect ADC to APB clock

    ADC1->CFGR1  |= 0b0000;  // configure ADC to res of 12bits
    ADC1->CFGR1  &= ~ADC_CFGR1_CONT;  // set to single conversion mode

    ADC1->CR  |=  ADC_CR_ADEN;  // enable ADC           
    while ( (ADC1->ISR & ADC_ISR_ADRDY) == 0 ){ }  // exit when ADC has stabilizeD
}

void adcLCDdisplay(uint16_t number) {
    /* for 12bit res, we expect a decimal number of up to fourdigits */
    uint8_t first_digit = number/1000; 
    uint8_t second_digit = (number - first_digit*1000)/100;
    uint8_t third_digit = (number - second_digit*100 - first_digit*1000)/10;
    uint8_t fourth_digit = number - third_digit*10 -second_digit*100 - first_digit*1000;
    
    lcd_command(CURSOR_HOME);  // reset cursor to home 
    lcd_putchar(first_digit+'0');
    lcd_putchar(second_digit+'0');
    lcd_putchar(third_digit+'0');
    lcd_putchar(fourth_digit+'0');
    
    }



void init_TIM14 (void){
    RCC -> APB1ENR |= RCC_APB1ENR_TIM14EN; // enable the clock
    TIM14 -> PSC = 122;                   //  If CLK = 8MHz, then Tov = 1s 
    TIM14 -> ARR = 65040;               // If CLK = 8MHz, then Tov = 1s
    TIM14 -> DIER |= TIM_DIER_UIE;    //  enable Timer 14 interrupt
    NVIC_EnableIRQ(TIM14_IRQn);     // unmask the TIM14 interrupt in the NVIC
    TIM14 -> CR1 |= TIM_CR1_CEN;    //  start counter for Timer 14     
}

void TIM14_IRQHandler (void){
    if ((GPIOB->ODR & GPIO_ODR_10) ==0){
        GPIOA -> ODR |= GPIO_ODR_3;     // turn IRs on
        GPIOB -> ODR |= GPIO_ODR_10;     //turn led on

    }else{
        GPIOA -> ODR &= ~GPIO_ODR_3;      // turn IRs off
        GPIOB -> ODR &= ~GPIO_ODR_10;    //turn led off
    } 
    TIM14 -> SR &= ~TIM_SR_UIF;  // clears the interrupt flag   
}

void leftLED(void){
        if (GPIOB -> ODR & GPIO_ODR_10){
            if (ADC_value_left < sideThreshold){
                GPIOB -> ODR |= GPIO_ODR_7;     //turn PB7 led on

            }else{
                GPIOB -> ODR &= ~GPIO_ODR_7;    //turn PB7 led off
            }   
        }
}

void frontLED(void){
        if (GPIOB -> ODR & GPIO_ODR_10){
            if (ADC_value_front < frontThreshold){
                GPIOB -> ODR |= GPIO_ODR_6;     //turn PB6 led on
                if(holdOnFront){TIM14 -> CR1 &= ~TIM_CR1_CEN;}    // stop counter for TIM14

            }else{
                GPIOB -> ODR &= ~GPIO_ODR_6;    //turn PB6 led off
                if(holdOnFront){TIM14 -> CR1 |= TIM_CR1_CEN;}    //start counter for TIM14
            }   
        }
}
void rightLED(void){
        if (GPIOB -> ODR & GPIO_ODR_10){
            if (ADC_value_right < sideThreshold){
                GPIOB -> ODR |= GPIO_ODR_5;     //turn PB5 led on

            }else{
                GPIOB -> ODR &= ~GPIO_ODR_5;    //turn PB5 led off
            }   
        }
}



//********************************************************************
// END OF PROGRAM
//********************************************************************
