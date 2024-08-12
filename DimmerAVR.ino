// TODO check if UART Irqs are active or not!
// TODO check if other IRQs except for OCRA, B, ICP are active

// TODO Check the time needed for a fade recalc
// TODO add check "The data overrun (DORn) flag indicates", if then send NAK and reset STX flag
// TODO see datasheet for USART examples, if calculations takes more than 500 uS do lower the baudratae to for example 2400 baud. Question, how slow does communication need to be ?

// TODO constants to be all upercase
// TODO enums to be DIMMER_ and the rest normal
// Variables to be DimmerVar

#include "Arduino.h"
#include <stdio.h>
#include "Dimmer.h"
#include "Dimmer_Config.h"

#include "Tool.h"
#include "TinyPrintf.h"
#include "Command.h"
#include "Settings.h"

void setup() {
  TIMSK0 = 0; // Disable Timer0 (not needed), causes erratic behavior for other Irqs
//
  LED_Set; 
  USARTP_Initialize(9600);
  SET_Initialize();
  Dimmer_Initialize();
  Dimmer_UpdateDaliTable(Settings.RangeMin, Settings.RangeMax);
  Command_Initialize();
  LED_Clear;
}


void loop() {
  Dimmer_Scheduler();
  USARTP_Scheduler();
  Command_Scheduler();
}
