/*
 * Tool.c
 */

#include "Tool.h"

// Hex 0..9 and A..F (no lowercase)
uint8_t ConvertHexToU8(uint8_t *pBuffer) {
  uint8_t Value;

  if (pBuffer[0] <= '9') {
    pBuffer[0] -= '0';
  } else {
    pBuffer[0] -= '7';
  }

  if (pBuffer[1] <= '9') {
    pBuffer[1] -= '0';
  } else {
    pBuffer[1] -= '7';
  }

  Value = (uint8_t)(pBuffer[0]<<4) + pBuffer[1];
  return Value;
}

uint16_t ConvertHexToU16(uint8_t *pBuffer) {
  uint8_t ValueLow;
  uint8_t ValueHigh;

  ValueHigh = ConvertHexToU8(pBuffer);
  ValueLow =  ConvertHexToU8(pBuffer+2);

  return (uint16_t)(ValueHigh*256) + (uint16_t)ValueLow;
}
