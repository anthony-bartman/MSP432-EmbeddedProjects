//*****************************************************************************
//
// main.c
//
// Anthony Bartman
// 1/21/21
// EE 4930
// Lab 4: MSP432
//
// Description:
// The LCD will display values from an interrupt-driven A/D converter from the
// potentiometers to signify a temperature and humidity.  A humidity setpoint
// will be used to turn on or off the fan and compressor if the humidity reading
// is within or outside 5% of the humidity setpoint. If there is ice, then the
// fan will turn on and the compressor will turn off until the ice is gone. This
// program is backed by a finite state machine model with a lookup table in fsm.c
// and fsm.h. All inputs are interrupt driven.
//*****************************************************************************

#include <stdint.h>
#include "msp.h"
#include "msoe_lib_clk.c"
#include "msoe_lib_lcd.c"
#include "msoe_lib_delay.c"

#include "fsm.h"

/**
 * main.c
 */

// INPUTS
void init_buttons(void);
void init_potentiometers(void);
void init_adc(void);

// OUTPUTS
void init_lcd(void);
void init_leds(void);

// GLOBAL VARS
uint8_t temperature = 70;
uint8_t humidity_setpoint = 55;
uint8_t humidity = 50;
uint8_t ice = 1;

void main(void)
{
    WDT_A->CTL = WDT_A_CTL_PW | WDT_A_CTL_HOLD;     // stop watchdog timer

    //Variables
    event input;
    state currentState = NORMAL;

    // Setup input and output GPIO's
    init_buttons();
    init_potentiometers();
    init_adc();
    init_lcd();
    init_leds();

    //Ice is not detected initially
    ice = 0;
    while (1)
    {
        //Start collecting ADC data
        ADC14->CTL0 |= 1;

        // Update Display
        LCD_goto_xy(8, 2);
        LCD_print_udec3(temperature);
        LCD_goto_xy(8, 3);
        LCD_print_udec3(humidity_setpoint);
        LCD_goto_xy(8, 4);
        LCD_print_udec3(humidity);

        //Determine current state of dehumidifer
        if (humidity > (humidity_setpoint + 5) && !ice)
        {
            input = TURN_ON;
            LCD_goto_xy(0, 5);
            LCD_print_str("           ");
        }
        else if (humidity > (humidity_setpoint - 5) && !ice)
        {
            input = TURN_OFF;
            LCD_goto_xy(0, 5);
            LCD_print_str("           ");
        }
        else if (ice)
        {
            input = HAS_ICE;
            LCD_goto_xy(0, 5);
            LCD_print_str("   DEFROST ");
        }
        else
        {
            input = STABLE;
            LCD_goto_xy(0, 5);
            LCD_print_str("           ");
        }
        // Updates current state of dehumidifier
        currentState = stateUpdate(currentState, input);
    }
}

// Using 3 Buttons
// Button 1.1 : Increase Humidity Setpoint
// Button 1.4 : Decrease Humidity Setpoint
// Pin 1.6 : Ice Sensor (Jumper Cable)
void init_buttons(void)
{
    // P1.1
    //Set Sel0/1 registers to be set to GPIO
    P1->SEL0 &= ~0b10;
    P1->SEL1 &= ~0b10;
    //Set GPIO pin to INPUT
    P1->DIR &= ~0b10;
    //Enable Pull up/Pull down mode
    P1->REN |= 0b10;
    //Setup as Pull up resistor
    P1->OUT |= 0b10;
    //Enable interrupt and on Rising edge
    P1->IES &= ~0b10;
    P1->IE |= 0b10;

    // P1.4
    //Set Sel0/1 registers to be set to GPIO
    P1->SEL0 &= ~0b10000;
    P1->SEL1 &= ~0b10000;
    //Set GPIO pin to INPUT
    P1->DIR &= ~0b10000;
    //Enable Pull up/Pull down mode
    P1->REN |= 0b10000;
    //Setup as Pull up resistor
    P1->OUT |= 0b10000;
    //Enable interrupt and on Rising edge
    P1->IES &= ~0b10000;
    P1->IE |= 0b10000;

    //P1.6 Ice Sensor(Jumper Wire)
    //Set Sel0/1 registers to be set to GPIO
    P1->SEL0 &= ~(0b1 << 6);
    P1->SEL1 &= ~(0b1 << 6);
    //Set GPIO pin to INPUT
    P1->DIR &= ~(0b1 << 6);
    //Enable Pull up/Pull down mode
    P1->REN |= (0b1 << 6);
    //Setup as Pull up resistor
    P1->OUT |= (0b1 << 6);
    //Enable interrupt and on Rising edge
    P1->IES &= ~(0b1 << 6);
    P1->IE |= (0b1 << 6);

    //Enable P1 in NVIC
    NVIC->ISER[1] |= (1 << 3);

}

// Used to update temperature or humidity
void init_potentiometers(void)
{
    // Set P4.5(A8) to ALT Function
    P4->SEL1 |= (1 << 5); // gives ADC control of pin
    P4->SEL0 |= (1 << 5);

    // Set P4.7(A6) to ALT Function
    P4->SEL1 |= (1 << 7); // gives ADC control of pin
    P4->SEL0 |= (1 << 7);
}

// Used to convert analog potentiometer signals into a digital signal to go to LCD
void init_adc(void)
{
    // Enable ADC | Sample and Hold time=96 1st registers | ' ' 2nd registers | Repeat sequence of channels | SMCLK | Timer Sourced ADC
    ADC14->CTL0 &= 0x0;
    ADC14->CTL0 |= (1 << 26) | (0b1 << 21) | (0b11 << 17) | (0b101 << 8)
            | (1 << 4);
    //8-bit resolution | store data in mem address 0x4 to start and stop at mem address 0x5
    ADC14->CTL1 &= 0xF0000000;
    ADC14->CTL1 |= (0b00 << 4) | (0b100 << 16); //Set MEM[4]  //9 Clock cycle conversion time

    ADC14->MCTL[4] |= 0x6; //Use MEM[4] to set input for A6
    ADC14->MCTL[5] |= 0x8; //Use MEM[5] to set input for A8
    ADC14->MCTL[5] |= (1 << 7); //Tells ADC this is last memory address in sequence (EOS bit)

    ADC14->IER0 |= (1 << 4); //Enable interrupt for MEM[4]
    ADC14->IER0 |= (1 << 5); //Enable interrupt for MEM[5]
    //Enable ADC and ADC NVIC interrupt
    ADC14->CTL0 |= 0b10;
    NVIC->ISER[0] |= (1 << 24);

}

// Create a LCD interface for a dehumidifier
void init_lcd(void)
{
    LCD_Config();
    LCD_clear();
    LCD_home();
    LCD_contrast(5);

    LCD_print_str("Dehumidifier");
    LCD_goto_xy(0, 2);
    LCD_print_str(" Temp: ");
    LCD_goto_xy(8, 2);
    LCD_print_udec3(temperature);
    LCD_goto_xy(11, 2);
    LCD_print_str("F");
    LCD_goto_xy(0, 3);
    LCD_print_str(" Set: ");
    LCD_goto_xy(8, 3);
    LCD_print_udec3(humidity_setpoint);
    LCD_goto_xy(11, 3);
    LCD_print_str("%");
    LCD_goto_xy(0, 4);
    LCD_print_str(" Humid:");
    LCD_goto_xy(8, 4);
    LCD_print_udec3(humidity);
    LCD_goto_xy(11, 4);
    LCD_print_str("%");
}

// Used to signify a Compressor(blue LED) or a Fan(red LED)
void init_leds(void)
{
    //P1.0 For FAN (RED)
    //Set Sel0/1 registers to be set to GPIO
    P1->SEL0 &= ~0b1;
    P1->SEL1 &= ~0b1;
    // Make Output
    P1->DIR |= 0b1;
    // Make LED output low
    P1->OUT &= ~0b1;

    //P2.2 for COMPRESSOR (BLUE)
    P2->SEL0 &= ~0b100;
    P2->SEL1 &= ~0b100;
    // Make Output
    P2->DIR |= 0b100;
    // Make LED output low
    P2->OUT &= ~0b100;
}

/*
 * Interrupts
 */

// Increases or Decreases temperature or humidity
void ADC14_IRQHandler(void)
{
    uint32_t which_interrupt = ADC14->IFGR0; // Holds whether an interrupt was triggered or not
    if (((which_interrupt & 0b10000) >> 4) == 1)
    {  //Temperature Increase or Decrease
        temperature = ((((float) ADC14->MEM[4]) / (255)) * (110 - 40) + 40);
        //Turn off interrupt
        ADC14->CLRIFGR0 |= (1 << 4);
    }
    else if (((which_interrupt & 0b100000) >> 5) == 1)
    {  //Humidity Increase or Decrease
        humidity = ((((float) ADC14->MEM[5]) / (255)) * (100 - 0) + 0);
        //Turn off interrupt
        ADC14->CLRIFGR0 |= (1 << 5);
    }
}

// Increases and Decreases Humidity Setpoint, and detects ice with jumper wire
void PORT1_IRQHandler(void)
{
    uint16_t which_interrupt = P1->IV;
    if (((which_interrupt & 0b100) >> 2) == 1
            && (which_interrupt & 0b1110) != 0xE)
    { // Increase setpoint
        humidity_setpoint =
                humidity_setpoint < 100 ?
                        humidity_setpoint + 5 : humidity_setpoint;
    }
    else if ((which_interrupt & 0b1010) == 0xA
            && (which_interrupt & 0b1110) != 0xE)
    { // Decrease setpoint
        humidity_setpoint =
                humidity_setpoint > 0 ?
                        humidity_setpoint - 5 : humidity_setpoint;
    }
    else
    { //Jumper wire detects ice sensor, does nothing here, but updates in main method
        ice = !((P1->IN & 1 << 6) >> 6);
    }
}

