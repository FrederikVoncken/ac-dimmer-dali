/*
 * USARTP.c
 */

#include "Arduino.h"
#include "USARTP.h"
#include "USARTP_CFG.h"
#if defined(DEBUG_USARTP_TEST)
#include "TinyPrintf.h"
#endif

typedef struct {
  uint8_t RX_Head;
  uint8_t RX_Tail;
  uint8_t RX_Buffer[USARTP_CFG_RX_BUFFER_SIZE];
  uint8_t TX_Head;
  uint8_t TX_Tail;
  uint8_t TX_Buffer[USARTP_CFG_TX_BUFFER_SIZE];
} USARTP_t;

static USARTP_t USARTP;


void USARTP_Baudrate(uint32_t Baud);
#if defined(DEBUG_USARTP_TEST)
void USARTP_Test(void);
#endif

void USARTP_Initialize(uint32_t Baud) {
  UCSR0A = 0;
  UCSR0B = 0;
  UCSR0C = (1 << UCSZ01) + (1 << UCSZ00);  // 8 bit
  USARTP_Baudrate(Baud);

  USARTP.RX_Tail = 0;
  USARTP.RX_Head = 0;
  USARTP.TX_Tail = 0;
  USARTP.TX_Head = 0;
#if defined(DEBUG_USARTP_TEST)
  USARTP_Test();
#endif
}

uint8_t USARTP_ReadEmpty(void) {
  if (USARTP.RX_Tail == USARTP.RX_Head) {
    return 1;
  } else {
    return 0;
  }
}

uint8_t USARTP_WriteEmpty(void) {
  if (USARTP.TX_Tail == USARTP.TX_Head) {
    return 1;
  } else {
    return 0;
  }
}

void USARTP_FlushTX_Buffer(void) {
  while (USARTP.TX_Tail != USARTP.TX_Head) {
    USARTP_Scheduler();  
  }
}

void USARTP_Baudrate(uint32_t Baud) {
  uint16_t ConfigBaud;
  uint8_t Use_U2X = 0;
  uint8_t NON_U2X_BaudError;
  uint8_t U2X_BaudError;

  // U2X mode is needed for baud rates higher than (CPU Hz / 16)
  if (Baud > F_CPU / 16) {
    Use_U2X = 1;
  } else {
    // Figure out if U2X mode would allow for a better connection

    // Calculate the percentual difference between the baud-rate specified and
    // the real baud rate for both U2X and non-U2X mode (0-255 error percent)
    NON_U2X_BaudError = abs((int)(255 - ((F_CPU / (16 * (((F_CPU / 8 / Baud - 1) / 2) + 1)) * 255) / Baud)));
    U2X_BaudError = abs((int)(255 - ((F_CPU / (8 * (((F_CPU / 4 / Baud - 1) / 2) + 1)) * 255) / Baud)));

    // Prefer non-U2X mode because it handles clock skew better
    if (NON_U2X_BaudError > U2X_BaudError)
      Use_U2X = 1;
  }

  if (Use_U2X) {
    UCSR0A |= (1 << U2X0);
    ConfigBaud = (F_CPU / 4 / Baud - 1) / 2;
  } else {
    UCSR0A &= ~(1 << U2X0);
    ConfigBaud = (F_CPU / 8 / Baud - 1) / 2;
  }

  UBRR0 = ConfigBaud;
  UCSR0B |= (1 << RXEN0) + (1 << TXEN0);
}

// TODO check for DOR0 and set flag message corrupt
void USARTP_Receive(void) {
  uint8_t i;
#if defined(DEBUG_USARTP_DIRECT_LOOPBACK) || defined(DEBUG_USARTP_LOOPBACK)
  uint8_t Value;
#endif
  if (UCSR0A & (1 << RXC0)) {
    i = (USARTP.RX_Head + 1) & (USARTP_CFG_RX_BUFFER_SIZE - 1);
    if (i != USARTP.RX_Tail) {
#if defined(DEBUG_USARTP_DIRECT_LOOPBACK)
      Value = UDR0;
      while (!(UCSR0A & (1 << UDRE0)));
      UDR0 = Value;
      USARTP.RX_Buffer[USARTP.RX_Head] = Value;
#elif defined(DEBUG_USARTP_LOOPBACK)
      Value = UDR0;
      USARTP_Write(Value);
      USARTP.RX_Buffer[USARTP.RX_Head] = Value;
#else
      USARTP.RX_Buffer[USARTP.RX_Head] = UDR0;
#endif
      USARTP.RX_Head = i;
    }
  }
}

void USARTP_Transmit(void) {
  uint8_t i;
  if (UCSR0A & (1 << UDRE0)) {
    if (USARTP.TX_Tail != USARTP.TX_Head) {
      UDR0 = USARTP.TX_Buffer[USARTP.TX_Tail];
      i = (USARTP.TX_Tail + 1) & (USARTP_CFG_RX_BUFFER_SIZE - 1);
      USARTP.TX_Tail = i;
    }
  }
}

uint8_t USARTP_Write(uint8_t Value) {
  uint8_t i;
  i = (USARTP.TX_Head + 1) & (USARTP_CFG_RX_BUFFER_SIZE - 1);
  if (i != USARTP.TX_Tail) {
    USARTP.TX_Buffer[USARTP.TX_Head] = Value;
    USARTP.TX_Head = i;
    return 1;
  } else {
    return 0;
  }
}

uint8_t USARTP_Read(uint8_t *Value) {
  if (USARTP.RX_Tail != USARTP.RX_Head) {
    *Value = USARTP.RX_Buffer[USARTP.RX_Tail];
    USARTP.RX_Tail = (USARTP.RX_Tail + 1) & (USARTP_CFG_RX_BUFFER_SIZE - 1);
    return 1;
  } else {
    *Value = 0;
    return 0;
  }
}

void USARTP_Scheduler(void) {
  static uint8_t State = 0;
#if defined(DEBUG_USARTP_LOOPBACK)
  uint8_t C;
#endif
  switch (State++) {
    case 0:
      USARTP_Receive();
      break;
    case 1:
      USARTP_Transmit();
      break;
#if defined(DEBUG_USARTP_LOOPBACK)
    case 2:
      if (USARTP_Read(&C)) {
        USARTP_Write(C);
      }
      break;
#endif
    default:
      break;
  }
}

#if defined(DEBUG_USARTP_TEST)
void USARTP_Test(void) {
  uint8_t C;

  uint8_t Buffer[4];
  uint8_t Value0;
  uint32_t Value1;
  uint32_t Value2;
  uint32_t Value3;
  uint32_t Value4;

  C = 'T';
  USARTP_Write(C);
  C = 'E';
  USARTP_Write(C);
  C = 'X';
  USARTP_Write(C);
  C = 'T';
  USARTP_Write(C);
  C = 0x0D;
  USARTP_Write(C);
  C = 0x0A;
  USARTP_Write(C);

  Value0 = 231;

  Value1 = 0xE412DE56;
  Value2 = 0x12BC;
  Value3 = 0x34EF;
  Value4 = 0x1F;
  Buffer[0] = 'T';
  Buffer[1] = 'X';
  Buffer[2] = 'T';
  Buffer[3] = 0;
  
  tiny_printf("Decimal %d\n", Value0);
  USARTP_FlushTX_Buffer();

  tiny_printf("Example\n");
  USARTP_FlushTX_Buffer();
  tiny_printf("String %s\n", Buffer);
  USARTP_FlushTX_Buffer(); 
  tiny_printf("Decimal %d end\n", 123456);
  USARTP_FlushTX_Buffer();
  tiny_printf("Decimal %d\n", Value1);
  USARTP_FlushTX_Buffer();  
  tiny_printf("Integer %i\n", Value1);
  USARTP_FlushTX_Buffer();
  tiny_printf("Unsigned %u\n", Value1);
  USARTP_FlushTX_Buffer();
  tiny_printf("Unsigned %u\n", Value2);
  USARTP_FlushTX_Buffer();
  tiny_printf("Hexadecimal 32b %x\n", Value1);
  USARTP_FlushTX_Buffer();
  tiny_printf("Hexadecimal 32b %x\n", Value2);
  USARTP_FlushTX_Buffer();
  tiny_printf("Hexadecimal 32b %8x\n", Value2);
  USARTP_FlushTX_Buffer();
  tiny_printf("Hexadecimal 32b %08x\n", Value2);
  USARTP_FlushTX_Buffer();
  tiny_printf("Hexadecimal 32b %08x\n", Value1);
  USARTP_FlushTX_Buffer();
  tiny_printf("Hexadecimal 16b %x\n", Value3);
  USARTP_FlushTX_Buffer();
  tiny_printf("Hexadecimal 16b %x\n", Value4);
  USARTP_FlushTX_Buffer();
  tiny_printf("Hexadecimal 16b %4x\n", Value4);
  USARTP_FlushTX_Buffer();
  tiny_printf("Hexadecimal 16b %04x\n", Value4);
  USARTP_FlushTX_Buffer();
  tiny_printf("Pointer %p\n", Value1);
  USARTP_FlushTX_Buffer();

  tiny_printf("Text via Scheduler\n");
}
#endif