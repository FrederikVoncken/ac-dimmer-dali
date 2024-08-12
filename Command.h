/*
 * Command.h
 */

#ifndef _COMMAND_H
#define _COMMAND_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
void Command_Initialize(void);
void Command_Scheduler(void);
void Command_Handler(uint8_t *pBuffer, uint8_t Size);

#ifdef __cplusplus
}
#endif

#endif /* _COMMAND_H */
