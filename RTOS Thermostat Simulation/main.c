//*****************************************************************************
//
// main.c
//
// Anthony Bartman
// 2/11/21
// EE 4930
// Lab 6: RTOS System - Phase 1
//
// Description:
// This lab creates a simple RTOS program that will use a potentiometer to simulate
// a temperature value that will be read by the ADC and then displayed to the LCD
// in a range of 50-90 degrees Farenheit.
//*****************************************************************************

/* XDC module Headers */
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/Timestamp.h>

/* BIOS module Headers */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/hal/Hwi.h>
#include <ti/sysbios/knl/Swi.h>

#include <ti/drivers/Board.h>

#define TASKSTACKSIZE   512
#define CLKPERIOD       250 //Clock waits about 0.25 ms in order to check if their is a new ADC value from Potentiometer
#define CLKTIMEOUT      1000 //If Clock waits longer than 1 second to finish it's task, stop the task

// RTOS objects and methods
Void clkFxn(UArg arg0);
Void lcdTask(UArg arg0, UArg arg1);
Void inputTask(UArg arg0, UArg arg1);
Void swiFxn(UArg arg0, UArg arg1);
Void hwiFxn(UArg arg0);

// Creates 2 Tasks
Task_Struct lcdTaskStruct, inputTaskStruct;
Char lcdTaskStack[TASKSTACKSIZE], inputTaskStack[TASKSTACKSIZE];
// Creates 2 Events
Event_Struct newPotValEventStruct, updateSetpointEventStruct;
Event_Handle newPotValEventHandle, updateSetpointEventHandle;
// Creates Clock for ADC
Clock_Struct adcClkStruct;
Clock_Handle adcClkHandle;
// Creates SWI to convert ADC to Farenheit
Swi_Struct toFarenheitSwiStruct;
Swi_Handle toFarenheitSwiHandle;
//Creates HWI to read ADC
Hwi_Struct adcInterruptHwiStruct;
Hwi_Handle adcInterruptHwiHandle;

#define __MSP432P401R__
#include <msp.h>
#include "msoe_lib_clk.c"
#include "msoe_lib_lcd.c"
#include "msoe_lib_delay.c"
#include <stdint.h>

// Clock waits about 0.25 ms in order to check if their is a new ADC value from Potentiometer
#define ADC_INTERRUPT 40
// Stores temperature reading from pot
uint8_t temperature = 50;

// C methods
void init_potentiometer(void);
void init_adc(void);
void init_lcd(void);

int main()
{
    // Init methods
    init_lcd();
    init_adc();
    init_potentiometer();

    // RTOS SETUP
    /* Globally Enables HWI */
    Hwi_enable();
    /* Construct BIOS Objects */
    Task_Params taskParams;
    Clock_Params clkParams;
    Hwi_Params hwiParams;
    Swi_Params swiParams;

    /* Call driver init functions */
    Board_init();

    /* Construct lcd/input Task threads */
    Task_Params_init(&taskParams);
    taskParams.stackSize = TASKSTACKSIZE;
    taskParams.priority = 1; // Can't be 0, that is reserved for idle, should be 1
    taskParams.arg0 = 0; //Set to 0, just like in test program.
    taskParams.arg1 = 0;

    taskParams.priority = 4; // Last priority because LCD is the last thing in RTOS flow
    taskParams.stack = &lcdTaskStack;
    Task_construct(&lcdTaskStruct, (Task_FuncPtr) lcdTask, &taskParams, NULL);

    taskParams.priority = 3; // Set before LCD because it's crucial we receive correct data from HWI and SWI
    taskParams.stack = &inputTaskStack;
    Task_construct(&inputTaskStruct, (Task_FuncPtr) inputTask, &taskParams,
    NULL);

    /* Setup Events */
    Event_construct(&newPotValEventStruct, NULL);
    newPotValEventHandle = Event_handle(&newPotValEventStruct);
    Event_construct(&updateSetpointEventStruct, NULL);
    updateSetpointEventHandle = Event_handle(&updateSetpointEventStruct);

    /* Setup Clock */
    Clock_Params_init(&clkParams);
    clkParams.startFlag = TRUE; // Turn's clock on
    Clock_construct(&adcClkStruct, clkFxn, CLKTIMEOUT, &clkParams);
    adcClkHandle = Clock_handle(&adcClkStruct);
    Clock_setPeriod(adcClkHandle, CLKPERIOD); // Sets clock period to 0.25s

    /* Setup SWI */
    //https://software-dl.ti.com/dsps/dsps_public_sw/sdo_sb/targetcontent/bios/sysbios/6_41_04_54/exports/bios_6_41_04_54/docs/cdoc/ti/sysbios/knl/Swi.html
    Swi_Params_init(&swiParams);
    swiParams.arg0 = 1;
    swiParams.arg1 = 0;
    swiParams.priority = 2; // Priority after HWI in order to change value to farenheit
    swiParams.trigger = 0;
    Swi_construct(&toFarenheitSwiStruct, swiFxn, &swiParams, NULL);
    toFarenheitSwiHandle = Swi_handle(&toFarenheitSwiStruct);

    /* Setup HWI */
    Hwi_Params_init(&hwiParams);
    hwiParams.eventId = ADC_INTERRUPT; //Sets eventID to interrupt for ADC; 40
    hwiParams.priority = 1; //Top priority since it handles reading ADC from pot
    Hwi_construct(&adcInterruptHwiStruct, ADC_INTERRUPT, hwiFxn, &hwiParams,
    NULL); // 40 for ADC Interrupt
    adcInterruptHwiHandle = Hwi_handle(&adcInterruptHwiStruct);

    BIOS_start(); /* Does not return */
    return (0);
}

// Used to convert analog potentiometer signals into a digital signal to go to LCD
void init_adc(void)
{
    // Enable ADC | Sample and Hold time=96 1st registers | ' ' 2nd registers | Single channel single conversion | SMCLK | Timer Sourced ADC
    ADC14->CTL0 &= 0x0;
    ADC14->CTL0 |= (1 << 26) | (0b1 << 21) | (0b00 << 17) | (0b101 << 12)
            | (0b101 << 8) | (1 << 4);
    //8-bit resolution | store data in mem address 0x4 to start and stop at mem address 0x5
    ADC14->CTL1 &= 0xF0000000;
    ADC14->CTL1 |= (0b00 << 4) | (0b100 << 16); //Set MEM[4]  //9 Clock cycle conversion time

    ADC14->MCTL[4] |= 0x6; //Use MEM[4] to set input for A6

    ADC14->IER0 |= (1 << 4); //Enable interrupt for MEM[4]
    //Enable ADC and ADC NVIC interrupt
    ADC14->CTL0 |= 0b10;
//    // Should be interrupt 40 because RTOS, handled with HWI.eventId
//    NVIC->ISER[0] |= (1 << 40);

}

// Create a LCD interface for a this simple RTOS lab
void init_lcd(void)
{
    LCD_Config();
    LCD_clear();
    LCD_home();
    LCD_contrast(5);

    LCD_goto_xy(0, 0);
    LCD_print_str(" Setpt: ");
    LCD_goto_xy(7, 0);
    LCD_print_udec3(temperature);
    LCD_goto_xy(10, 0);
    LCD_print_str(" F");

    LCD_goto_xy(0,2);
    LCD_print_str("  RTOS LAB");

}

// Used to update temperature
void init_potentiometer(void)
{
    // Set P4.7(A6) to ALT Function
    P4->SEL1 |= (1 << 7); // gives ADC control of pin
    P4->SEL0 |= (1 << 7);
}

/*
 *  This function is used for reading the setpoint pot. Should be read every 0.25 seconds.
 */
Void clkFxn(UArg arg0)
{
    //Clock triggers only to start reading ADC
    ADC14->CTL0 |= 0b1;
}

/*
 *  This function is used to manage what temperature value is shown on the LCD display
 */
Void lcdTask(UArg arg0, UArg arg1)
{
    UInt posted;

    while (1)
    {
        posted = Event_pend(updateSetpointEventHandle, Event_Id_00,
        Event_Id_NONE,
                            BIOS_WAIT_FOREVER); // Wait for input task to finish before updating LCD

        // If the event succeeded and this task is running,
        if (posted)
        {
            //Update LCD with new temperature values
            LCD_goto_xy(7, 0);
            LCD_print_udec3(temperature);

            //Go back to waiting for new potentiometer values
            Event_post(newPotValEventHandle, Event_Id_00);
        }
    }
}

/*
 *  This function is used to handle when the potentiometer value has moved
 */
Void inputTask(UArg arg0, UArg arg1)
{
    UInt posted;

    while (1)
    {
        // Waiting for SWI to finish
        posted = Event_pend(newPotValEventHandle, Event_Id_00, Event_Id_NONE,
        BIOS_WAIT_FOREVER);

        //If SWI has finished, tell update LCD task to begin
        if (posted)
        {
            //After SWI has finished, run update LCD task
            Event_post(updateSetpointEventHandle, Event_Id_00);
        }
    }
}

/*
 *  This function is used to convert the ADC value into Farenheit from 50-90 F
 */
Void swiFxn(UArg arg0, UArg arg1)
{
    temperature = ((((float) ADC14->MEM[4]) / (255)) * (90 - 50) + 50);
    Event_post(newPotValEventHandle, Event_Id_00); // Tells task SWI has completed adc conversion.
}

/*
 *  This function is used to read the ADC when the it's interrupt triggers from the 0.25s clock
 */
Void hwiFxn(UArg arg0)
{
    int temp_reading = ADC14->MEM[4];  // NEED TO READ this in order to CLEAR ADC INTERRUPT FLAG, or infinite loop here
    Swi_post(toFarenheitSwiHandle); //Tells SWI we received an interrupt, time to convert data to Farenheit
}

