//*****************************************************************************
//
// main.c
//
// Anthony Bartman
// 2/23/21
// EE 4930
// Lab 7: Igloo Lab
//
// Description:
// This lab creates an RTOS program that will simulate an Igloo environment and
// change the temperature based on an I2C TMP102 sensor and using a PWM fan all
// powered by an external battery pack.
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

/* RTOS Drivers */
#include <ti/drivers/Board.h>
#include <ti/drivers/PWM.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/GPIO.h>
#include "ti_drivers_config.h"

#define TASKSTACKSIZE   512
#define CLKPERIOD       250 //Clock waits about 0.25 ms in order to check if their is a new ADC value from Potentiometer
#define CLKTIMEOUT      1000 //If Clock waits longer than 1 second to finish it's task, stop the task

// RTOS objects and methods
Void clkFxn(UArg arg0);
Void lcdTask(UArg arg0, UArg arg1);
Void inputTask(UArg arg0, UArg arg1);
Void fanPWMTask(UArg arg0, UArg arg1);
Void tempI2CTask(UArg arg0, UArg arg1);
Void swiFxn(UArg arg0, UArg arg1);
Void hwiFxn(UArg arg0);

// Creates 4 Tasks
Task_Struct lcdTaskStruct, inputTaskStruct, fanPWMStruct, tempI2CStruct;
Char lcdTaskStack[TASKSTACKSIZE], inputTaskStack[TASKSTACKSIZE],
        fanPWMStack[TASKSTACKSIZE], tempI2CStack[TASKSTACKSIZE];
// Creates 2 Events
Event_Struct newTempReadingStruct, updateLCDEventStruct;
Event_Handle newTempReadingHandle, updateLCDEventHandle;
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
// Stores setpoint temperature reading from pot (i.e. desired temp in the room
uint8_t desired_temperature = 65;
uint8_t actual_temperature = 65;  //Actual Temp reading in room from i2c

// C methods
void init_potentiometer(void);
void init_adc(void);
void init_lcd(void);
void init_fan_and_heater(void);

int main()
{
    // Init methods
    init_lcd();
    init_adc();
    init_potentiometer();
    init_fan_and_heater();

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

    /* Construct Task threads */
    Task_Params_init(&taskParams);
    taskParams.stackSize = TASKSTACKSIZE;
    taskParams.arg0 = 0; //Set to 0, just like in test program.
    taskParams.arg1 = 0;

    taskParams.priority = 6; // Last priority because LCD is the last thing in RTOS flow
    taskParams.stack = &lcdTaskStack;
    Task_construct(&lcdTaskStruct, (Task_FuncPtr) lcdTask, &taskParams, NULL);

    taskParams.priority = 5; // Set before LCD because it's crucial we receive correct data from HWI and SWI
    taskParams.stack = &inputTaskStack;
    Task_construct(&inputTaskStruct, (Task_FuncPtr) inputTask, &taskParams,
    NULL);

    taskParams.priority = 4; // Turn on fan before updating/reading input data, 2nd TOP PRIORITY
    taskParams.stack = &fanPWMStack;
    Task_construct(&fanPWMStruct, (Task_FuncPtr) fanPWMTask, &taskParams, NULL);

    taskParams.priority = 3; // Read temp sensor data in order to determine fan, TOP PRIORITY
    taskParams.stack = &tempI2CStack;
    Task_construct(&tempI2CStruct, (Task_FuncPtr) tempI2CTask, &taskParams,
    NULL);

    /* Setup Events */
    Event_construct(&newTempReadingStruct, NULL);
    newTempReadingHandle = Event_handle(&newTempReadingStruct);
    Event_construct(&updateLCDEventStruct, NULL);
    updateLCDEventHandle = Event_handle(&updateLCDEventStruct);

    /* Setup Clock */
    Clock_Params_init(&clkParams);
    clkParams.startFlag = TRUE; // Turn's clock on
    Clock_construct(&adcClkStruct, clkFxn, CLKTIMEOUT, &clkParams);
    adcClkHandle = Clock_handle(&adcClkStruct);
    Clock_setPeriod(adcClkHandle, CLKPERIOD); // Sets clock period to 0.25s

    /* Setup SWI */
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
    LCD_print_str(" Temp: ");
    LCD_goto_xy(7, 0);
    LCD_print_udec3(actual_temperature);
    LCD_goto_xy(10, 0);
    LCD_print_str(" F");

    LCD_goto_xy(0, 2);
    LCD_print_str(" Setpt: ");
    LCD_goto_xy(7, 2);
    LCD_print_udec3(desired_temperature);
    LCD_goto_xy(10, 2);
    LCD_print_str(" F");

    LCD_goto_xy(0, 4);
    LCD_print_str(" FINAL LAB");

}

// Used to update temperature
void init_potentiometer(void)
{
    // Set P4.7(A6) to ALT Function
    P4->SEL1 |= (1 << 7); // gives ADC control of pin
    P4->SEL0 |= (1 << 7);
}

// Uses simple GPIO Output pins to start fan and heater
void init_fan_and_heater(void)
{
    //P2.5 for Heater
    P2->SEL0 &= ~0b100000;
    P2->SEL1 &= ~0b100000;
    // Make Output
    P2->DIR |= 0b100000;
    // Make Fan output low
    P2->OUT &= ~0b100000;

    //P2.6 for FAN
    P2->SEL0 &= ~0b1000000;
    P2->SEL1 &= ~0b1000000;
    // Make Output
    P2->DIR |= 0b1000000;
    // Make Fan output low
    P2->OUT &= ~0b1000000;

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
        posted = Event_pend(updateLCDEventHandle, Event_Id_00,
        Event_Id_NONE,
                            BIOS_WAIT_FOREVER); // Wait for input task or PWM Fan task to finish before updating LCD

        // If the event succeeded and this task is running,
        if (posted)
        {
            //Update LCD with temperature values
            LCD_goto_xy(7, 0);
            LCD_print_udec3(actual_temperature);
            LCD_goto_xy(7, 2);
            LCD_print_udec3(desired_temperature);

            //Go back to waiting for new temperature reading values
            Event_post(newTempReadingHandle, Event_Id_00);
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
        posted = Event_pend(newTempReadingHandle, Event_Id_00, Event_Id_NONE,
        BIOS_WAIT_FOREVER);

        //If SWI has finished, tell update LCD task to begin
        if (posted)
        {
            //After SWI has finished, run update LCD task
            Event_post(updateLCDEventHandle, Event_Id_00);
        }
    }
}

Void fanPWMTask(UArg arg0, UArg arg1)
{
    UInt posted;

    /* Initialize RTOS PWM Tasks */
    PWM_Handle handle;
    PWM_Params params;

    /* Call driver init functions */
    PWM_init();

    PWM_Params_init(&params);
    params.idleLevel = PWM_IDLE_LOW;      // Output low when PWM is not running
    params.periodUnits = PWM_PERIOD_HZ;   // Period is in Hz
    params.periodValue = 13;            // 12.5 Hz Cycle
    params.dutyUnits = PWM_DUTY_FRACTION; // Duty is fraction of period
    params.dutyValue = 0;                // 20% duty cycle

    handle = PWM_open(0, &params);
    if (handle == NULL)
    {
        while (1)
            ;
    }

    /* PWM Business Logic */
    while (1)
    {

        // Waiting for I2C calculation to finish
        posted = Event_pend(newTempReadingHandle, Event_Id_00,
        Event_Id_NONE,
                            BIOS_WAIT_FOREVER);

        //If I2C has finished, tell update LCD task to begin
        if (posted)
        {
            if ((actual_temperature + 1) <= desired_temperature)
            {
                // Turn on FAN and heater
                P2->OUT |= ((1 << 5) | (1 << 6));
                PWM_start(handle);
            }
            else if ((actual_temperature - 1) >= desired_temperature)
            {
                // Turn off FAN and heater
                P2->OUT &= ~((1 << 5) | (1 << 6));
                PWM_stop(handle);
            }
        }
    }
    BIOS_exit(0);
}

Void tempI2CTask(UArg arg0, UArg arg1)
{
    UInt posted;

    I2C_Handle i2c;
    I2C_Params i2cParams;
    I2C_Transaction i2cTransaction;

    I2C_init();

    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_100kHz;
    i2c = I2C_open(CONFIG_I2C_0, &i2cParams);

    if(i2c == NULL){
        while(1);
    }


    /* I2C Business Logic */
    while (1)
    {
        // Wait for SWI to finish, then check for I2C reading
        posted = Event_pend(newTempReadingHandle, Event_Id_00,
        Event_Id_NONE,
                            BIOS_WAIT_FOREVER);
        if (posted)
        {
            // Post back to PWM task
            Event_post(newTempReadingHandle, Event_Id_00);
        }
    }
}

/*
 *  This function is used to convert the ADC value into Farenheit from 50-90 F
 */
Void swiFxn(UArg arg0, UArg arg1)
{
    desired_temperature = ((((float) ADC14->MEM[4]) / (255)) * (90 - 50) + 50);
    Event_post(newTempReadingHandle, Event_Id_00); // Tells task SWI has completed adc conversion.
}

/*
 *  This function is used to read the ADC when the it's interrupt triggers from the 0.25s clock
 */
Void hwiFxn(UArg arg0)
{
    int temp_reading = ADC14->MEM[4]; // NEED TO READ this in order to CLEAR ADC INTERRUPT FLAG, or infinite loop here
    Swi_post(toFarenheitSwiHandle); //Tells SWI we received an interrupt, time to convert data to Farenheit
}

