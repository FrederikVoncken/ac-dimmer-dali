/*
 * Dimmer.h
 */

#ifndef _DIMMER_H
#define _DIMMER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "DaliLut.h"

extern volatile uint8_t DimmerDemo;

#define DimmerOCR_Calc_uS(a) ((a*16)/8)

typedef enum {
  Dimmer0 = 0,
  Dimmer1 = 1,
  DimmerMAX = 2,
} Dimmer_Select_t;


void Dimmer_Initialize(void);
void Dimmer_Scheduler(void);
void Dimmer_UpdateDaliTable(uint16_t TriacPulseMin, uint16_t TriacPulseMax);
void Dimmer_SetFade(Dimmer_Select_t Select, uint32_t DurationMS, uint8_t EndBrightness);
void Dimmer_SetBrightness(Dimmer_Select_t Select, uint8_t Brightness);
uint8_t Dimmer_GetBrightness(Dimmer_Select_t Select);
void Dimmer_SetDirectValue(Dimmer_Select_t Select, uint16_t Value);
uint16_t Dimmer_GetDirectValue(Dimmer_Select_t Select);

#ifdef __cplusplus
}
#endif

#endif /* _DIMMER_H */
