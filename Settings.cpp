/*
 * Settings.c
 */

#include "Arduino.h"
#include "Settings.h"
#include "Dimmer_Config.h"
#include "Tool.h"
#include "TinyPrintf.h"
#include <EEPROM.h>

#if defined(DEBUG_SETTINGS)
#define debug_tiny_printf(...) tiny_printf(__VA_ARGS__)
#define debug_flush_printf() USARTP_FlushTX_Buffer()
#else
#define debug_tiny_printf(str,...)
#define debug_flush_printf()
#endif

Settings_t Settings;

void SET_Validate(void);

void SET_Initialize(void) {
  debug_tiny_printf("Begin init Settings\n"); 
  SET_Load();
  debug_tiny_printf("End init Settings\n");
}

uint8_t SET_Load(void) {
  EEPROM.get(SETTINGS_EEPROM_ADDR, Settings);
  SET_ShowSettings();
  SET_Validate();
  SET_ShowSettings();
  return 1;
}

void SET_Save(void) {
  Settings.Version = DIMMER_VERSION;
  SET_ShowSettings();
  EEPROM.put(SETTINGS_EEPROM_ADDR, Settings);
}
 
void SET_LoadScratch(void) {
  debug_tiny_printf("Loading Scratch default\n");
  Settings.Version = DIMMER_VERSION;
  Settings.MainzHZ = 50;
  // In range of SET_DIM_RANGE_MIN_50HZ and SET_DIM_RANGE_MAX_50HZ
  Settings.RangeMin =  2000; // range = 0 to 20000
  Settings.RangeMax = 18000; // range = 0 to 20000
}

void SET_Validate(void) {
  debug_tiny_printf("Validating settings\n");
  
  
  if (Settings.Version == 0xFF) { // first time erased flash
    SET_LoadScratch();
    return;
  }

  if (Settings.MainzHZ == 50) {
    CLIPtoMin(Settings.RangeMin, SET_DIM_RANGE_MIN_50HZ, SET_DIM_RANGE_MAX_50HZ);
    CLIP(Settings.RangeMax, SET_DIM_RANGE_MIN_50HZ, SET_DIM_RANGE_MAX_50HZ);
  } else if (Settings.MainzHZ == 60) {
    CLIPtoMin(Settings.RangeMin, SET_DIM_RANGE_MIN_60HZ, SET_DIM_RANGE_MAX_60HZ);
    CLIP(Settings.RangeMax, SET_DIM_RANGE_MIN_60HZ, SET_DIM_RANGE_MAX_60HZ);
  } else {  // first time erased flash
    SET_LoadScratch();
  }
}

void SET_ShowSettings(void) {
  debug_tiny_printf("0x%02x %u\n", Settings.Version, Settings.Version);
  debug_tiny_printf("0x%02x %u\n", Settings.MainzHZ, Settings.MainzHZ);
  debug_tiny_printf("0x%04x %u\n", Settings.RangeMin, Settings.RangeMin);
  debug_tiny_printf("0x%04x %u\n", Settings.RangeMax, Settings.RangeMax);
  debug_flush_printf();
}
