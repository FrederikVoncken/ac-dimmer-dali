/*
 * DimmerCmdList.h
 */

#ifndef _DIMMER_CMD_LIST_H
#define _DIMMER_CMD_LIST_H

#ifdef __cplusplus
extern "C" {
#endif

#define DIMMER_CMD_VERSION  1

// Protocol:
// Send => <STX><data bytes><ETX>
// Return => <ACK>[<STX><data bytes><ETX>]

// DIMMER_CMD_ Address _SIZE (bytes) Command Param1 Size min max Scratch Param2 Size min max Scratch
#define DIMMER_CMD_OFF_ADDR                   0x00 // 0	Off
#define DIMMER_CMD_OFF_SIZE                   0
#define DIMMER_CMD_ON_MAX_ADDR                0x02 // 0 OnMax (254)
#define DIMMER_CMD_ON_MAX_SIZE                0
#define DIMMER_CMD_STOP_ADDR                  0x03 // 0 Stop (255) - -
#define DIMMER_CMD_STOP_SIZE                  0

#define DIMMER_CMD_SET_ADDR                   0x10 // 1 Set DaliValue uin8_t 0 255 -
#define DIMMER_CMD_SET_SIZE                   2
#define DIMMER_CMD_GET_SET_ADDR               0x90 // 1 Return Set value DaliValue uin8_t
#define DIMMER_CMD_GET_SET_SIZE               0
#define DIMMER_CMD_SET_FADE_TIME_ADDR         0x11 // 3 SetFadeTime DaliValue uint8_t 0 254 0 Time_ms uint16_t 100 65535 100
#define DIMMER_CMD_SET_FADE_TIME_SIZE         6
#define DIMMER_CMD_SET_FADE_STEPS_ADDR        0x12 // 2 SetFadeSteps DaliValue uint8_t 0 254 0 1/steps per fade uint8_t 1 255 1
#define DIMMER_CMD_SET_FADE_STEPS_SIZE        4

#define DIMMER_CMD_SAVE_ADDR                  0x30 // 1 Save MagicNumber uint8_t - - 0x55 -
#define DIMMER_CMD_SAVE_SIZE                  2
#define DIMMER_CMD_SAVE_MAGIC_NUMBER          0x55
#define DIMMER_CMD_LOAD_ADDR                  0x31 // 1 Load MagicNumber uint8_t - - 0x66 -
#define DIMMER_CMD_LOAD_SIZE                  2
#define DIMMER_CMD_LOAD_MAGIC_NUMBER          0x66
#define DIMMER_CMD_LOAD_SCRATCH_ADDR          0x32 // 1 LoadScratch MagicNumber uint8_t - - 0x77 -
#define DIMMER_CMD_LOAD_SCRATCH_SIZE          2
#define DIMMER_CMD_LOAD_SCRATCH_MAGIC_NUMBER  0x77

#define DIMMER_CMD_SET_MAINZ_HZ_VAL_ADDR      0x70
#define DIMMER_CMD_SET_MAINZ_HZ_VAL_SIZE      2
#define DIMMER_CMD_GET_MAINZ_HZ_VAL_ADDR      0xF0
#define DIMMER_CMD_GET_MAINZ_HZ_VAL_SIZE      0

#define DIMMER_CMD_SET_TIMER_VAL_ADDR         0x71 // 2 SetTimerValue (stops all other actions) TimerValue uint16_t 0 20000
#define DIMMER_CMD_SET_TIMER_VAL_SIZE         4  
#define DIMMER_CMD_GET_TIMER_VAL_ADDR         0xF1
#define DIMMER_CMD_GET_TIMER_VAL_SIZE         0
#define DIMMER_CMD_SET_CAL_LOW_VAL_ADDR       0x72
#define DIMMER_CMD_SET_CAL_LOW_VAL_SIZE       4
#define DIMMER_CMD_GET_CAL_LOW_VAL_ADDR       0xF2
#define DIMMER_CMD_GET_CAL_LOW_VAL_SIZE       0
#define DIMMER_CMD_SET_CAL_HIGH_VAL_ADDR      0x73 // 2 SetCalHigherValue (update Dali table) (use Save function) TimerValue uint16_t 0 20000
#define DIMMER_CMD_SET_CAL_HIGH_VAL_SIZE      4
#define DIMMER_CMD_GET_CAL_HIGH_VAL_ADDR      0xF3
#define DIMMER_CMD_GET_CAL_HIGH_VAL_SIZE      0
#define DIMMER_CMD_GET_CAL_RANGE_VAL_ADDR     0xF4 // GetCalRange, 20000 for 50Hz and 16666 for 60Hz
#define DIMMER_CMD_GET_CAL_RANGE_VAL_SIZE     0

#define DIMMER_CMD_GET_CMD_VERSION_ADDR       0xF9	// 0 GetCmdVersion	Version	uint8_t 1
#define DIMMER_CMD_GET_CMD_VERSION_SIZE       0

#define DIMMER_CMD_GET_VERSION_ADDR           0xFA	// 0 GetVersion	Version	uint8_t 1
#define DIMMER_CMD_GET_VERSION_SIZE           0

#ifdef __cplusplus
}
#endif

#endif // _DIMMER_CMD_LIST_H