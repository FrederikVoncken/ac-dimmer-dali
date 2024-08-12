/*
 * Dimmer_Config.h
 */

#ifndef _DIMMER_CONFIG_H
#define _DIMMER_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Arduino.h"
#include "Settings.h"

#define DEBUG_DIMMER
//#define DEBUG_DIMMER_DALI

//#define DEBUG_DIMMER_DEMO

#define DIMMER_VERSION  0

#define ExternalDimmer_MAINS_HZ Settings.MainzHZ
#define ExternalDimmer_RangeDefault Settings.RangeMin

#define Dimmer_PulseWidth 50 // uS

// TODO make GPIO lib call (header define file)
#define ExternalDebugPinCAPT_Init     DDRB |= (1<<DDB3); PORTB &= ~(1<<PORTB3) // Init D11
#define ExternalDebugPinCAPT_Set      PORTB |= (1<<PORTB3); // Set D11
#define ExternalDebugPinCAPT_Clear1   PORTB &= ~(1<<PORTB3);  // Clear D11
#define ExternalDebugPinCAPT_Clear2   //PORTB &= ~(1<<PORTB3);  // Clear D11

#define ExternalDebugPinOCRA_Init     //DDRB |= (1<<DDB4); PORTB &= ~(1<<PORTB4) // Init D12
#define ExternalDebugPinOCRA_Set      //PORTB |= (1<<PORTB4); // Set D12
#define ExternalDebugPinOCRA_Clear    //PORTB &= ~(1<<PORTB4) // Clear D12 

#define ExternalDebugPinOCRB_Init     //DDRB |= (1<<DDB5); PORTB &= ~(1<<PORTB5) // Init D12
#define ExternalDebugPinOCRB_Set      //PORTB |= (1<<PORTB5); // Set D13
#define ExternalDebugPinOCRB_Clear    //PORTB &= ~(1<<PORTB5) // Clear D13 

#ifdef __cplusplus
}
#endif

#endif /* _DIMMER_CONFIG_H */
