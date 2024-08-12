/*
 * Command.c
 */
#include "Arduino.h"
#include "Command.h"
#include "Command_CFG.h"
#include "Tool.h"
#include "TinyPrintf.h"
#include "Settings.h"
#include "DimmerCmdList.h"
#include "Dimmer.h"
#include "Dimmer_Config.h"

#if defined(DEBUG_COMMAND)
#define debug_tiny_printf(...) tiny_printf(__VA_ARGS__)
#else
#define debug_tiny_printf(str,...)
#endif

#define COM_STX   '#'
#define COM_ETX   10
#define COM_ACK   6
#define COM_NAK   21
#define COM_SPACE 32
#define COM_NULL  0

#define CMD_MAX_RX_BUFFER 16 // AA CC D1 D2 D3 ..

#define CheckSize(S, M) if (CMD.MultiAddress) { if (S != M) { return; } } else { if (S != M) { CMD_Write(COM_NAK); return; } else { CMD_Write(COM_ACK); }}

typedef struct {
  uint8_t RX_Index = 0;
  uint8_t RX_Buffer[CMD_MAX_RX_BUFFER];
  uint8_t MultiAddress;
} COMMAND_t;

static volatile COMMAND_t CMD;

void Command_Initialize(void) {
  debug_tiny_printf("Command: Begin init\n");
  CMD.RX_Index = 0;
  CMD.MultiAddress = 0;
  debug_tiny_printf("Command: End init\n");
}

void Command_Scheduler(void) {
  static uint8_t State = 0;
  uint8_t C;
  if (CMD_Read(&C) == 0) {
    return;
  }

  if ((C == COM_NULL)  || (C == COM_SPACE)) {
    return;
  }

  switch(State) {
  case 0:
    if (C == COM_STX) {
      CMD.RX_Index = 0;
      State++;
    }
    break;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
  case 1:
    if (C == COM_ETX) {
      State++;
    } else {
      if (CMD.RX_Index < CMD_MAX_RX_BUFFER) {
        CMD.RX_Buffer[CMD.RX_Index++] = C;
        return;
      } else {
        State = 0;
        debug_tiny_printf("Fail: Receive buffer overflow\n");
        return;
      }
    }
  case 2:
    debug_tiny_printf("Executing CommandHandler\n");
    Command_Handler((uint8_t *)CMD.RX_Buffer, CMD.RX_Index);
    State = 0;
    break;
#pragma GCC diagnostic pop
  default:
    State = 0;
    break;
  }
}

// TODO every return is always with an address?

void Command_Handler(uint8_t *pBuffer, uint8_t Size) {
  uint8_t Address;
  uint8_t Command;
  uint8_t Magic;
  uint16_t DurationMS;
  uint8_t Brightness;
  Dimmer_Select_t DimmerSelect;
  uint8_t Value_u8;
  uint16_t Value_u16;
#if defined(DEBUG_COMMAND)
  tiny_printf("Command: ");
  for (uint8_t i = 0; i < Size; i++) {
    CMD_Write(pBuffer[i]);
  }
  tiny_printf(" with size %d\n", Size);
#endif

  if (Size < 4) {
    debug_tiny_printf("Message too small\n");
    CMD_Write(COM_NAK);
    return;
  }
  
  if (Size%2 != 0) {
    debug_tiny_printf("Message not multiple of 2 bytess\n");
    CMD_Write(COM_NAK);
    return;
  }
  for (uint8_t i = 0; i< Size; i++) {
    if (CheckHex(pBuffer[i])) {
    } else {
      debug_tiny_printf("Message not hex format\n");
      CMD_Write(COM_NAK);
    }
  }

  Size -= 4; // Remove Address and Command size

  Address = ConvertHexToU8(pBuffer);
  pBuffer+=2;

  switch(Address) {
  case 0x00:
    DimmerSelect = Dimmer0;
    break;
  case 0x01:
    DimmerSelect = Dimmer1;
    break;
  default:
      debug_tiny_printf("Incorrect Address 0x%02x, ignoring\n", Address);
      // don not reply, ignore
      return;
  }

  /*switch(Address) {
  case 0x00:
    CMD.MultiAddress = 0;
    break;
  case 0x10:
    CMD.MultiAddress = 1;
    break;
  default:
      debug_tiny_printf("Incorrect Address 0x%02x, ignoring\n", Address);
      // don not reply, ignore
      return;
  }*/

  Command = ConvertHexToU8(pBuffer);
  pBuffer+=2;
  debug_tiny_printf("Command %i\n", Command);
  debug_tiny_printf("Size %i\n", Size);
  // Handle commands
  if ((Command & 0x80) == 0) {
// Write commands
    switch (Command) {
    case DIMMER_CMD_OFF_ADDR:
      CheckSize(Size, DIMMER_CMD_OFF_SIZE);
      Dimmer_SetBrightness(DimmerSelect, 0);
      break;
    case DIMMER_CMD_ON_MAX_ADDR:
      CheckSize(Size, DIMMER_CMD_ON_MAX_SIZE);
      Dimmer_SetBrightness(DimmerSelect, 254);
      break;
    case DIMMER_CMD_STOP_ADDR:
      CheckSize(Size, DIMMER_CMD_STOP_SIZE);
      Dimmer_SetBrightness(DimmerSelect, 255);
      break;
    case DIMMER_CMD_SET_ADDR:
      CheckSize(Size, DIMMER_CMD_SET_SIZE);
      Brightness = ConvertHexToU8(pBuffer);
      debug_tiny_printf("Set %i\n", Brightness);
      Dimmer_SetBrightness(DimmerSelect, Brightness);
      break;
    case DIMMER_CMD_SET_FADE_TIME_ADDR:
      CheckSize(Size, DIMMER_CMD_SET_FADE_TIME_SIZE);
      Brightness = ConvertHexToU8(pBuffer);
      DurationMS = ConvertHexToU16(pBuffer+2);
      Dimmer_SetFade(DimmerSelect, DurationMS, Brightness);
      break;
    case DIMMER_CMD_SET_FADE_STEPS_ADDR:
      CheckSize(Size, DIMMER_CMD_SET_FADE_STEPS_SIZE);
      Value_u16 = ConvertHexToU16(pBuffer);
      if (Settings.MainzHZ == 60) {
        CLIPtoMin(Value_u16, SET_DIM_RANGE_MIN_60HZ, SET_DIM_RANGE_MAX_60HZ);
      } else {
        CLIPtoMin(Value_u16, SET_DIM_RANGE_MIN_50HZ, SET_DIM_RANGE_MAX_50HZ);
      }
      // TODO create Fade steps algoritmn
      Dimmer_SetDirectValue(DimmerSelect, Value_u16);
      break;
    case DIMMER_CMD_SAVE_ADDR:				
      CheckSize(Size, DIMMER_CMD_SAVE_SIZE);
      Magic = ConvertHexToU8(pBuffer);
      if (Magic == DIMMER_CMD_SAVE_MAGIC_NUMBER) {
        SET_Save();
      } // else ignore command
      break;
    case DIMMER_CMD_LOAD_ADDR:				
      CheckSize(Size, DIMMER_CMD_LOAD_SIZE);
      Magic = ConvertHexToU8(pBuffer);
      if (Magic == DIMMER_CMD_LOAD_MAGIC_NUMBER) {
        SET_Load();
        Dimmer_UpdateDaliTable(Settings.RangeMin, Settings.RangeMax);
      } // else ignore command
      break;
    case DIMMER_CMD_LOAD_SCRATCH_ADDR:		
      CheckSize(Size, DIMMER_CMD_LOAD_SCRATCH_SIZE);
      Magic = ConvertHexToU8(pBuffer);
      if (Magic == DIMMER_CMD_LOAD_SCRATCH_MAGIC_NUMBER) {
        SET_LoadScratch();
        Dimmer_UpdateDaliTable(Settings.RangeMin, Settings.RangeMax);
      } // else ignore command
      break;
    case DIMMER_CMD_SET_MAINZ_HZ_VAL_ADDR:	
      CheckSize(Size, DIMMER_CMD_SET_MAINZ_HZ_VAL_SIZE);
      Value_u8 = ConvertHexToU16(pBuffer);
      if (Value_u8 == 50) {
        Settings.MainzHZ = 50;
      }
      if (Value_u8 == 60) {
        Settings.MainzHZ = 60;
      }
      // else ignore command
      break;
    case DIMMER_CMD_SET_TIMER_VAL_ADDR:
      CheckSize(Size, DIMMER_CMD_SET_TIMER_VAL_SIZE);
      Value_u16 = ConvertHexToU16(pBuffer);
      if (Settings.MainzHZ == 60) {
        CLIPtoMin(Value_u16, SET_DIM_RANGE_MIN_60HZ, SET_DIM_RANGE_MAX_60HZ);
      } else {
        CLIPtoMin(Value_u16, SET_DIM_RANGE_MIN_50HZ, SET_DIM_RANGE_MAX_50HZ);
      }
      Dimmer_SetDirectValue(DimmerSelect, Value_u16);
      break;		
    case DIMMER_CMD_SET_CAL_LOW_VAL_ADDR:
      CheckSize(Size, DIMMER_CMD_SET_CAL_LOW_VAL_SIZE);
      Settings.RangeMin = ConvertHexToU16(pBuffer);
      Dimmer_UpdateDaliTable(Settings.RangeMin, Settings.RangeMax);
      break;
    case DIMMER_CMD_SET_CAL_HIGH_VAL_ADDR:
      CheckSize(Size, DIMMER_CMD_SET_CAL_HIGH_VAL_SIZE);
      Settings.RangeMax = ConvertHexToU16(pBuffer);
      Dimmer_UpdateDaliTable(Settings.RangeMin, Settings.RangeMax);
      break;
    default:
      debug_tiny_printf("Incorrect CMD WR 0x%02x (Addr 0x%02x)\n", Command, Address);
      if (CMD.MultiAddress == 0) {
        CMD_Write(COM_NAK);
      }
      break;
    }
  } else {
    if (CMD.MultiAddress) {
      return; // Multi addresses do not return an answer, NAK or ACK
    }
    debug_tiny_printf("Read\n");
    switch (Command) {
    case DIMMER_CMD_GET_SET_ADDR:								
      CheckSize(Size, DIMMER_CMD_GET_SET_SIZE);
      Value_u8 =  Dimmer_GetBrightness(DimmerSelect);
      tiny_printf("#%02x\n", Value_u8);
      break;
    case DIMMER_CMD_GET_MAINZ_HZ_VAL_ADDR:
      CheckSize(Size, DIMMER_CMD_GET_MAINZ_HZ_VAL_SIZE);
      tiny_printf("#%02x\n", Settings.MainzHZ);
      break;		
    case DIMMER_CMD_GET_TIMER_VAL_ADDR:
      CheckSize(Size, DIMMER_CMD_GET_TIMER_VAL_SIZE);
      Value_u16 = Dimmer_GetDirectValue(DimmerSelect);
      tiny_printf("#%04x\n", Value_u16);
      break;
    case DIMMER_CMD_GET_CAL_LOW_VAL_ADDR:
      CheckSize(Size, DIMMER_CMD_GET_CAL_LOW_VAL_SIZE);
      // TODO create range for every dimmer separately
      tiny_printf("#%04x\n", Settings.RangeMin);
      break;
    case DIMMER_CMD_GET_CAL_HIGH_VAL_ADDR:
      CheckSize(Size, DIMMER_CMD_GET_CAL_HIGH_VAL_SIZE);
      // TODO create range for every dimmer separately
      tiny_printf("#%04x\n", Settings.RangeMax);
      break;
    case DIMMER_CMD_GET_CAL_RANGE_VAL_ADDR:
      CheckSize(Size, DIMMER_CMD_GET_CAL_RANGE_VAL_SIZE);
      if (Settings.MainzHZ == 50) {
        tiny_printf("#%04x\n", SET_DIM_RANGE_MAX_50HZ);
      } else {
        tiny_printf("#%04x\n", SET_DIM_RANGE_MAX_60HZ);
      }
      break;
    case DIMMER_CMD_GET_CMD_VERSION_ADDR:
    	CheckSize(Size, DIMMER_CMD_GET_CMD_VERSION_SIZE);
      tiny_printf("#%02x\n", DIMMER_CMD_VERSION);	
      break;
    case DIMMER_CMD_GET_VERSION_ADDR:
    	CheckSize(Size, DIMMER_CMD_GET_VERSION_SIZE);
      tiny_printf("#%02x\n", DIMMER_VERSION);	
      break;
    default:
      debug_tiny_printf("Incorrect CMD RD 0x%02x (Addr 0x%02x)\n", Command, Address);
      CMD_Write(COM_NAK);
      break;
    }
  }
}
