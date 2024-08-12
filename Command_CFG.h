/*
 * Command_CFG.h
 */

#ifndef COMMAND_CFG_H_
#define COMMAND_CFG_H_

#define DEBUG_COMMAND

#include "USARTP.h"

#define CMD_Write(c) USARTP_Write(c)
#define CMD_Read(c) USARTP_Read(c)

#endif // COMMAND_CFG_H_
