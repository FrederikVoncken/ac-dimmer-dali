/*
 * TinyPrinf.h
 */

#ifndef TINY_PRINTF_H
#define TINY_PRINTF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "TinyPrintf_CFG.h"

void tiny_printf(const char *ctrl1, ...);

#ifdef __cplusplus
}
#endif

#endif // TINY_PRINTF_H
