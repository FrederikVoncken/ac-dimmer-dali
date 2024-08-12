/*
 * Settings.h
 */

#ifndef _SETTINGS_H
#define _SETTINGS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "Settings_CFG.h"

#define SET_DIM_RANGE_MIN_60HZ  10
#define SET_DIM_RANGE_MAX_60HZ  16666

#define SET_DIM_RANGE_MIN_50HZ  10
#define SET_DIM_RANGE_MAX_50HZ  20000

typedef struct {
  uint8_t Version;
  uint8_t MainzHZ;
  uint16_t RangeMin;
  uint16_t RangeMax;
} Settings_t;

extern Settings_t Settings;

void SET_Initialize(void);

uint8_t SET_Load(void);
void SET_Save(void); 
void SET_LoadScratch(void);

void SET_ShowSettings(void);

#ifdef __cplusplus
}
#endif

#endif // _SETTINGS_H
