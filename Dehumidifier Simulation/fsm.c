//*****************************************************************************
//
// fsm.c
//
// Anthony Bartman
// 1/21/21
// EE 4930
// Lab 4: MSP432
//
// Description:
// Handles the finite state machine logic by using a lookup table to update the
// state of lab.
//*****************************************************************************

#ifndef FSM_C_
#define FSM_C_

#include "fsm.h"
#include "msp.h"

//Holds all states possible for this dehumidifer lab
stateElement stateTable[3][4] = {
  { {NORMAL, outputs_off}, {HUMID, outputs_on}, {NORMAL, outputs_off}, {ICE, defrosting} },
  { {NORMAL, outputs_off}, {HUMID, outputs_on}, {NORMAL, outputs_off}, {ICE, defrosting} },
  { {NORMAL, outputs_off}, {HUMID, outputs_on}, {NORMAL, outputs_off}, {ICE, defrosting} },
};

state stateUpdate(state current, event input)
{
    stateElement currentstate = stateTable[current][input];
    (*currentstate.action)();
    return currentstate.nextstate;
}

void outputs_off()
{
    P1->OUT &= ~0b1;  // Fan (Red LED)
    P2->OUT &= ~0b100; // Compressor (Blue LED)
}

void outputs_on()
{
    P1->OUT |= 0b1;  // Fan (Red LED)
    P2->OUT |= 0b100; // Compressor (Blue LED)
}

void defrosting()
{
    P1->OUT |= 0b1; // Fan (Red LED)
    P2->OUT &= ~0b100; // Compressor (Blue LED)
}

#endif
