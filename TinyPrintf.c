/*
 * TinyPrintf.c
 */

/*
 * Modified from :
 * Public Domain version of printf
 * Rud Merriam, Compsult, Inc. Houston, Tx.
 * For Embedded Systems Programming, 1991
 *
 */
 
 /*
 * The purpose of this routine is to output data the
 * same as the standard printf function without the
 * overhead most run-time libraries involve. Usually
 * the printf brings in many kilobytes of code and
 * that is unacceptable in most embedded systems.
 */

#include "TinyPrintf.h"
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#define OutBufferSize 32

typedef struct params_s {
  int32_t len;
  int32_t num1;
  int32_t num2;
  char    pad_character;
  uint8_t do_padding;
  uint8_t left_flag;
  uint8_t unsigned_flag;
} params_t;

static void padding(const uint8_t l_flag, const struct params_s *par);
static void out_string(const char *lp, struct params_s *par);
static void out_number(const int32_t n, const int32_t base, struct params_s *par);
static int32_t get_number(char **linep);

static void padding(const uint8_t l_flag, const struct params_s *par) {
  int32_t i;
  if ((par->do_padding != 0) && (l_flag != 0) && (par->len < par->num1)) {
    i = (par->len);
    for (; i < (par->num1); i++) {
      TinyPrintf_CFG_putch(par->pad_character);
    }
  }
}

static void out_string(const char *lp, struct params_s *par) {
  char *LocalPtr;
  LocalPtr = (char *)lp;

  if (LocalPtr != NULL) {
    par->len = (int32_t)strlen((const char*)LocalPtr);
  }
  
  padding(!(par->left_flag), par);

  // Move string to the buffer
  while (((*LocalPtr) != (char)0) && ((par->num2) != 0)) {
    (par->num2)--;
    TinyPrintf_CFG_putch(*LocalPtr);
    LocalPtr += 1;
  }

  // Pad on left if needed
  padding(par->left_flag, par);
}

static void out_number(const int32_t n, const int32_t base, struct params_s *par) {
  uint8_t negative;
  int8_t i;
  char outbuf[OutBufferSize];
  const char digits[] = "0123456789ABCDEF";
  uint32_t num;
  for (i = 0; i < OutBufferSize; i++) {
    outbuf[i] = '0';
  }

  // Check if number is negative
  if ((par->unsigned_flag == 0) && (base == 10) && (n < 0L)) {
    negative = 1;
    num =(-(n));
  } else {
    num = n;
    negative = 0;
  }

  // Build number (backwards) in outbuf
  i = 0;
  do {
    outbuf[i] = digits[(num % base)];
    i++;
    num /= base;
  } while (num > 0);

  if (negative != 0) {
    outbuf[i] = '-';
    i++;
  }

  outbuf[i] = 0;
  i--;

  // Move the converted number to the buffer and add in the padding where needed.
  par->len = (int32_t)strlen(outbuf);
  padding( !(par->left_flag), par);
  while (&outbuf[i] >= outbuf) {
    TinyPrintf_CFG_putch(outbuf[i]);
    i--;
  }
  padding(par->left_flag, par);
}

static int32_t get_number(char **linep) {
  int32_t n;
  int32_t ResultIsDigit = 0;
  char *cptr;
  n = 0;
  cptr = *linep;
  if (cptr != NULL) {
    ResultIsDigit = isdigit(((int)*cptr));
  }
  while (ResultIsDigit != 0) {
    if (cptr != NULL) {
      n = ((n*10) + (((int32_t)*cptr) - (int32_t)'0'));
      cptr += 1;
      if (cptr != NULL) {
        ResultIsDigit = isdigit(((int)*cptr));
      }
    }
    ResultIsDigit = isdigit(((int)*cptr));
  }
  *linep = ((char *)(cptr));
  return n;
}

/*
 * This routine operates just like a printf/sprintf
 * routine. It outputs a set of data under the
 * control of a formatting string. Not all of the
 * standard C format control are supported. The ones
 * provided are primarily those needed for embedded
 * systems work. Primarily the floating point
 * routines are omitted. Other formats could be
 * added easily by following the examples shown for
 * the supported formats.
 */
void tiny_printf(const char *ctrl1, ...) {
  uint8_t Check;
  uint8_t dot_flag;

  params_t par;

  char ch;
  va_list argp;
  char *ctrl = (char *)ctrl1;

  va_start(argp, ctrl1);

  while ((ctrl != NULL) && (*ctrl != (char)0)) {
    // Move format string chars to buffer until a format control is found.
    if (*ctrl != '%') {
      TinyPrintf_CFG_putch(*ctrl);
      ctrl += 1;
      continue;
    }

    // Initialize all the flags for this format.
    Check = 0;
    dot_flag = 0;
    par.unsigned_flag = 0;
    par.left_flag = 0;
    par.do_padding = 0;
    par.pad_character = ' ';
    par.num2=32767;
    par.num1=0;
    par.len=0;

 try_next:
    if (ctrl != NULL) {
      ctrl += 1;
    }
    if(ctrl != NULL) {
      ch = *ctrl;
    } else {
      ch = *ctrl;
    }

    if (isdigit((int)ch) != 0) {
      if (dot_flag != 0) {
        par.num2 = get_number(&ctrl);
      } else {
        if (ch == '0') {
          par.pad_character = '0';
        }
        if (ctrl != NULL) {
          par.num1 = get_number(&ctrl);
        }
        par.do_padding = 1;
      }
      if (ctrl != NULL) {
        ctrl -= 1;
      }
      goto try_next;
    }

    switch (tolower((int)ch)) {
    case '%': // % character
      TinyPrintf_CFG_putch('%');
      Check = 1;
      break;
    case '-': // Left justify (right is default)
      par.left_flag = 1;
      Check = 0;
      break;
    case '.':
      dot_flag = 1;
      Check = 0;
      break;
    case 'l': // No long, max is uint32_t / int32_t
     	TinyPrintf_CFG_putch('%'); // No long output supported with this architecture
      TinyPrintf_CFG_putch('l');
      Check = 0;
      break;
    case 'u': // Unsigned uint32_t
      par.unsigned_flag = 1;
      // Fall through
    case 'i': // Integer int32_t
    case 'd': // Decimal int32_t
      out_number( va_arg(argp, int), 10L, &par);
  		Check = 1;
      break;
    case 'p': // Pointer address
    case 'X': // Hexadecimal
    case 'x': // Hexadecimal
      par.unsigned_flag = 1;
      out_number((int32_t)va_arg(argp, int), 16L, &par);
      Check = 1;
      break;
    case 's': // String
      out_string(va_arg(argp, char *), &par);
      Check = 1;
      break;
    case 'c': // Character
      TinyPrintf_CFG_putch(va_arg(argp, int));
      Check = 1;
      break;
    case '\\':
      switch (*ctrl) {
      case 'a': // Bell
       	TinyPrintf_CFG_putch(((char)0x07));
        break;
      case 'h': // Backspace
       	TinyPrintf_CFG_putch(((char)0x08));
        break;
      case 'r': // Cariage Feed
       	TinyPrintf_CFG_putch(((char)0x0D));
        break;
      case 'n': // Cariage Feed + Line feed
       	//TinyPrintf_CFG_putch(((char)0x0D));
        TinyPrintf_CFG_putch(((char)0x0A));
        break;
      default:
       	TinyPrintf_CFG_putch(*ctrl);
        break;
      }
      ctrl += 1;
      Check = 0;
      break;
    default:
      Check = 1;
      break;
    }
    if (Check == 1) {
      if (ctrl != NULL) {
        ctrl += 1;
      }
      continue;
    }
    goto try_next;
  }
  va_end(argp);
}
