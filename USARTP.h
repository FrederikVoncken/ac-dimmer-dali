/*
 * USARTP.h
 */

#ifndef _USARTP_H
#define _USARTP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

void USARTP_Initialize(uint32_t Baud);
void USARTP_Scheduler(void);
uint8_t USARTP_Write(uint8_t Value);
uint8_t USARTP_Read(uint8_t *Value);
uint8_t USARTP_ReadEmpty(void);
uint8_t USARTP_WriteEmpty(void);
void USARTP_FlushTX_Buffer(void);

#ifdef __cplusplus
}
#endif

#endif // _USARTP_H
