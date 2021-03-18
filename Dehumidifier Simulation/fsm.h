//*****************************************************************************
//
// fsm.h
//
// Anthony Bartman
// 1/21/21
// EE 4930
// Lab 4: MSP432
//
// Description:
// Header file to describe the methods and enumerated types used in the .c file
// to handle the finite state machine of the logic.
//*****************************************************************************

#ifndef FSM_H_
#define FSM_H_
// Structs and enumerated types to create a Lookup Table Finite State Machine
typedef enum
{
    NORMAL, HUMID, ICE
} state;
typedef enum
{
    STABLE, TURN_ON, TURN_OFF, HAS_ICE
} event;
typedef void (*fp)(void);
typedef struct
{
    state nextstate;
    fp action;
} stateElement;

// Handles logic for updating the current state of the dehumidifier
// Param: current - current state, input - what happened on board
state stateUpdate(state current, event input);
// Turns off Fan(red LED) and Compressor(blue LED)
void outputs_off();
// Turns on Fan(red LED) and Compressor(blue LED)
void outputs_on();
// Turns on Fan(red LED) and the Compressor(blue LED) off
void defrosting();

#endif
