/*
 * DaliLut.h
 */

#ifndef _DALI_LUT_H
#define _DALI_LUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <avr/pgmspace.h>

#define LUT_DALI_Size  254
#define LUT_DALI_Resolution   131072
#define LUT_DALI_Resolution_2 65536

extern const uint16_t LUT_DALI[LUT_DALI_Size];

#ifdef __cplusplus
}
#endif

#endif /* _DALI_LUT_H */
