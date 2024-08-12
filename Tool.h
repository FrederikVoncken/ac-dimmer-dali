/*
 * Tool.h
 */

#ifndef _TOOL_H
#define _TOOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "Arduino.h"
#include <stdint.h>

#define LED_Set      PORTB |= (1<<PORTB5) // Set D13
#define LED_Clear    PORTB &= ~(1<<PORTB5) // Clear D13
#define LED_Toggle   if (PORTB & (1<<PORTB5)) { LED_Clear; } else { LED_Set; } // Toggle D13

// delay 62.5ns on a 16MHz AtMega
#define nop() __asm__ __volatile__ ("nop\n\t")
#define CheckHex(C) (((C >= '0') && (C <= '9')) || ((C >= 'A') && (C <= 'F')))
#define CLIP(Value, Min, Max) if (Value < Min) { Value = Min; } else if (Value > Max) { Value = Max; }
#define CLIPtoMin(Value, Min, Max) if (Value < Min) { Value = Min; } else if (Value > Max) { Value = Min; }

uint8_t ConvertHexToU8(uint8_t *pBuffer);
uint16_t ConvertHexToU16(uint8_t *pBuffer);

#ifdef __cplusplus
}
#endif

#endif // _TOOL_H
