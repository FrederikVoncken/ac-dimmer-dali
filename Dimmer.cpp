/*
 * Dimmer.c
 */

#include "Arduino.h"
#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>

#include "Dimmer_Config.h"
#include "Dimmer.h"
#include "DaliLut.h"
#include "TinyPrintf.h"

#if defined(DEBUG_DIMMER)
#define debug_tiny_printf(...) tiny_printf(__VA_ARGS__)
#else
#define debug_tiny_printf(str,...)
#endif

#if defined(DEBUG_DIMMER_DALI)
#define debug_dali_tiny_printf(...) tiny_printf(__VA_ARGS__)
#else
#define debug_dali_tiny_printf(str,...)
#endif

typedef enum {
  DimmerModeOff = 0,
  DimmerModeOn = 1,
  DimmerModeFadeUp = 2,
  DimmerModeFadeDown = 3,
  DimmerModeFadePostAction = 4,
  DimmerModeFadeNoAction = 5,
} Dimmer_Mode_t;

typedef struct {
  Dimmer_Mode_t Mode;
  uint8_t  StartBrightness;
  uint8_t  EndBrightness;
  uint8_t  DeltaBrightness;
  uint8_t  CurrentBrightness;
  uint32_t CurrentCycle;
  uint32_t StartCycle;
  uint32_t EndCycle;
  uint32_t DeltaCycle;
  uint16_t CurrentOCR;
} Dimmer_t;
static volatile Dimmer_t Dimmer[DimmerMAX];

typedef struct {
  uint8_t Enable;
  int16_t NextPulseValue[2];
  uint8_t NextPulseValueSemaphore;
  uint8_t State;
} Dimmer_OCR_t;
static volatile Dimmer_OCR_t DimmerOCR[DimmerMAX];

static volatile uint16_t DaliTable[LUT_DALI_Size];

static volatile uint16_t Dimmer_CurrentPulsePeriod = 0;
static volatile uint8_t  Dimmer_CurrentCountIrq = 0;
static volatile uint8_t  Dimmer_CaptureFlag = 0;
static volatile uint32_t Dimmer_CurrentCycle = 0;


void Dimmer_CalcFade(Dimmer_Select_t Select, uint32_t CurrentCount);

ISR(TIMER1_CAPT_vect) {
  ExternalDebugPinCAPT_Set;
  
  Dimmer_CurrentPulsePeriod = TCNT1;
  TCNT1 = 0;

  if (DimmerOCR[Dimmer0].Enable == 0) {
    // OCR output is being disabled
    TCCR1A &= ~((1<<COM1A1)+(1<<COM1A0));
    TIMSK1 &= ~(1<<OCIE1A);
  } else {
    // OCR (is enabled) will be set on next compare
    TCCR1A |= ((1<<COM1A1)+(1<<COM1A0));
    TIMSK1 |= (1<<OCIE1A);
    if (DimmerOCR[0].NextPulseValueSemaphore == 0) {
      OCR1A = (uint16_t)DimmerOCR[Dimmer0].NextPulseValue[0];
    } else {
      OCR1A = (uint16_t)DimmerOCR[Dimmer0].NextPulseValue[1];
    }
  }
  DimmerOCR[Dimmer0].State = 0;
  
  if (DimmerOCR[Dimmer1].Enable == 0) {
    // OCR output is being disabled
    TCCR1A &= ~((1<<COM1B1)+(1<<COM1B0));
    TIMSK1 &= ~(1<<OCIE1B);
  } else {
    // OCR (is enabled) will be set on next compare
    TCCR1A |= ((1<<COM1B1)+(1<<COM1B0));
    TIMSK1 |= (1<<OCIE1B);
    if (DimmerOCR[Dimmer1].NextPulseValueSemaphore == 0) {
      OCR1B = (uint16_t)DimmerOCR[Dimmer1].NextPulseValue[0];
    } else {
      OCR1B = (uint16_t)DimmerOCR[Dimmer1].NextPulseValue[1];
    }
  }
  DimmerOCR[Dimmer1].State = 0;

  // Clear all interrupt flags
  TIFR1   = (1<<ICF1) + (1<<OCF1A) + (1<<OCF1B);

  ExternalDebugPinCAPT_Clear1;
  
  Dimmer_CurrentCountIrq++; // The actualCounter outside the interrupt is 32 bit and is updated with this counter
  Dimmer_CaptureFlag = 1; // Allow new calulcation round
  
  ExternalDebugPinCAPT_Clear2; 
}

ISR(TIMER1_COMPA_vect) {
  uint8_t OCR_TEMP;
  ExternalDebugPinOCRA_Set;
  
  if (DimmerOCR[Dimmer0].State == 0) {
    OCR1A += DimmerOCR_Calc_uS(Dimmer_PulseWidth);
    // OCR will be cleared on next compare
    OCR_TEMP = TCCR1A & ~((1<<COM1A1)+(1<<COM1A0));
    OCR_TEMP |= (1<<COM1A1);
    TCCR1A = OCR_TEMP;
  }
  DimmerOCR[Dimmer0].State++;

  ExternalDebugPinOCRA_Clear;
}

ISR(TIMER1_COMPB_vect) {
  uint8_t OCR_TEMP;
  ExternalDebugPinOCRB_Set;
  
  if (DimmerOCR[Dimmer1].State == 0) {
    OCR1B += DimmerOCR_Calc_uS(Dimmer_PulseWidth);
    // OCR will be cleared on next compare
    OCR_TEMP = TCCR1A & ~((1<<COM1B1)+(1<<COM1B0));
    OCR_TEMP |= (1<<COM1B1);
    TCCR1A = OCR_TEMP;
  }
  DimmerOCR[Dimmer1].State++;

  ExternalDebugPinOCRB_Clear;
}

void Dimmer_Initialize(void) {
  debug_tiny_printf("Dimmer: Begin init\n");

  TIMSK0 = 0; // Disable Timer0 (not needed), causes erratic behavior for other Irqs
  
  DDRB &= ~(1<<DDB0); // Input D8

  DDRB |= (1<<DDB1); // Output D9
  PORTB &= ~(1<<PORTB1); // Clear D9

  DDRB |= (1<<DDB2); // Output D10
  PORTB &= ~(1<<PORTB2); // Clear D10 
// TODO move, not part of Dimmer
  ExternalDebugPinCAPT_Init;
  ExternalDebugPinOCRA_Init;
  ExternalDebugPinOCRA_Init;

  DimmerOCR[Dimmer0].Enable = 0;
  DimmerOCR[Dimmer0].NextPulseValue[0] = ExternalDimmer_RangeDefault;
  DimmerOCR[Dimmer0].NextPulseValue[1] = DimmerOCR[Dimmer0].NextPulseValue[0];
  DimmerOCR[Dimmer0].NextPulseValueSemaphore = 0;
  DimmerOCR[Dimmer0].State = 0;

  DimmerOCR[Dimmer1].Enable = 0;
  DimmerOCR[Dimmer1].NextPulseValue[0] = ExternalDimmer_RangeDefault;
  DimmerOCR[Dimmer1].NextPulseValue[1] = DimmerOCR[Dimmer1].NextPulseValue[0];
  DimmerOCR[Dimmer1].NextPulseValueSemaphore = 0;
  DimmerOCR[Dimmer1].State = 0;

  Dimmer[Dimmer0].Mode = DimmerModeOff;
  Dimmer[Dimmer0].CurrentBrightness = 0;
  Dimmer[Dimmer1].Mode = DimmerModeOff;
  Dimmer[Dimmer1].CurrentBrightness = 0;
 
  // Clear OCR1A and OCR1B
  TCCR1A = (1<<COM1A1) + (1<<COM1B1);
  TCCR1C = (1<<FOC1A) + (1<<FOC1B);
  // Input Capture Noise Canceler
  // Input Capture Rising Edge
  // Timer clock = I/O clock / 8
  TCCR1B = (1<<ICNC1) + (1<<ICES1) + (1<<CS11);

	// Clear pending interrupts Input Capture, OCR1A and OCR1B
  TIFR1   = (1<<ICF1) + (1<<OCF1A) + (1<<OCF1B);

	// Enable Timer 1 Capture Event, OCR1A and OCR1B Interrupt
	TIMSK1  = (1<<ICIE1) + (1<<OCIE1A) + (1<<OCIE1B);
  debug_tiny_printf("Dimmer: End init\n");
}

void Dimmer_Scheduler(void) {
  uint8_t LocalCount;
  if (Dimmer_CaptureFlag != 0) {  
    Dimmer_CaptureFlag =  0;
    // Atomic actions are only possible for 8 bit actions
    // Dimmer_CurrentCycle is updated with DimmerCurrentCountIrq
    //  DimmerCurrentCountIrq is not set to 0, because in between an IRQ can happen and in so, DimmerCurrentCountIrq-LocalCount can be 0, but also 1 (or 2 ..) 
    LocalCount = Dimmer_CurrentCountIrq;
    Dimmer_CurrentCountIrq -= LocalCount;
    Dimmer_CurrentCycle += LocalCount;

    switch (Dimmer[Dimmer0].Mode) {
    default:
    case DimmerModeOff:
      break;
    case DimmerModeOn:
      break;
    case DimmerModeFadeDown:
    case DimmerModeFadeUp:  
      Dimmer_CalcFade(Dimmer0, Dimmer_CurrentCycle);
      break;
    }

    switch (Dimmer[Dimmer1].Mode) {
    default:
    case DimmerModeOff:
      break;
    case DimmerModeOn:
      break;
    case DimmerModeFadeDown:
    case DimmerModeFadeUp:  
      Dimmer_CalcFade(Dimmer1, Dimmer_CurrentCycle);
      break;
    }
// ****
    //if the semaphore is active, the backup (1) is read in the interupt, otherwise the main (0)
    for (uint8_t i = 0; i < DimmerMAX; i++) {
      DimmerOCR[i].NextPulseValueSemaphore = 1;
      DimmerOCR[i].NextPulseValue[0] = Dimmer[i].CurrentOCR;
      DimmerOCR[i].NextPulseValueSemaphore = 0;
      DimmerOCR[i].NextPulseValue[1] = Dimmer[i].CurrentOCR;
    }

#if defined(DEBUG_DIMMER_DEMO)
    if (Dimmer[Dimmer0].Mode == DimmerModeFadePostAction) {
      if (Dimmer[Dimmer0].CurrentBrightness < 128) {
        Dimmer_SetFade(Dimmer0, 20000, 254);
      } else {
        Dimmer_SetFade(Dimmer0, 20000, 1);
      }
    }
    if (Dimmer[Dimmer1].Mode == DimmerModeFadePostAction) {
      if (Dimmer[Dimmer1].CurrentBrightness < 128) {
        Dimmer_SetFade(Dimmer1, 20000, 254);
      } else {
        Dimmer_SetFade(Dimmer1, 20000, 1);
      }
    }
#endif
  }
}

void Dimmer_UpdateDaliTable(uint16_t TriacPulseMin, uint16_t TriacPulseMax) {
  uint32_t DaliValue = 0;
  uint16_t Value16 = 0;
  uint32_t Value32 = 0;
  uint16_t TriacPulseDelta;

  debug_dali_tiny_printf("Dimmer: Begin update Dali table");


  TriacPulseDelta = TriacPulseMax - TriacPulseMin;
  
  // Value = DimmerMAX_RANGE - ((Dimmer_DELTA*DaliValue) + (LUT_DALI_Resolution/2))/LUT_DALI_Resolution
  for (uint8_t i=0; i< LUT_DALI_Size; i++) {
    DaliValue += pgm_read_word_near(LUT_DALI+i);

    Value32 =  (uint32_t)TriacPulseDelta * (uint32_t)DaliValue + (uint32_t)LUT_DALI_Resolution_2; 
    Value32 += (uint32_t)LUT_DALI_Resolution_2;
    Value16 = Value32 / (uint32_t)LUT_DALI_Resolution;
    DaliTable[i] = TriacPulseMax - Value16;
  }

  debug_dali_tiny_printf("Dimmer: End update Dali table\n");


  #if defined(DEBUG_DIMMER_DALI)
  tiny_printf("Dimmer: Begin show Dali table\n");
  for (uint8_t i=0; i< LUT_DALI_Size; i++) {
    tiny_printf(i);
    tiny_printf(" ");
    tiny_printf(DaliTable[i]);
    tiny_printf("/n");
  }
  tiny_printf("Dimmer: End show Dali table\n");
  #endif
}

void Dimmer_CalcFade(Dimmer_Select_t Select, uint32_t CurrentCount) {
  uint32_t DurationDone;
  uint16_t Value16;
  uint32_t Value32; 
  int16_t CurrentBrightness;
  uint16_t OCR_Value;
  uint16_t  DeltaDali;
  uint8_t DeltaCurrentBrightness;
  
  PORTB |= (1<<PORTB3); // Set D11
  
  if ((Dimmer[Select].Mode != DimmerModeFadeUp) && (Dimmer[Select].Mode != DimmerModeFadeDown)) {
    PORTB &= ~(1<<PORTB3);  // Clear D11
    return; // not fading at the moment
  }

  if (Dimmer[Select].StartCycle < Dimmer[Select].EndCycle) {
    if ((CurrentCount < Dimmer[Select].StartCycle) || (CurrentCount > Dimmer[Select].EndCycle)) {
      CurrentCount = Dimmer[Select].EndCycle;
    } 
  } else {
    if ((CurrentCount < Dimmer[Select].StartCycle) && (CurrentCount > Dimmer[Select].EndCycle)) {
      CurrentCount = Dimmer[Select].EndCycle;
    }
  }

  DurationDone = CurrentCount - Dimmer[Select].StartCycle;
  Value32 = (uint32_t)DurationDone * Dimmer[Select].DeltaBrightness;
    
  if (Dimmer[Select].Mode == DimmerModeFadeUp) {
    CurrentBrightness = Dimmer[Select].StartBrightness + (uint8_t)(Value32/Dimmer[Select].DeltaCycle);
  } else {
    CurrentBrightness = Dimmer[Select].StartBrightness - (uint8_t)(Value32/Dimmer[Select].DeltaCycle);
  }
  
  if (CurrentBrightness < 0) {
    CurrentBrightness = 0;
  } else if (CurrentBrightness > LUT_DALI_Size) {
    CurrentBrightness = LUT_DALI_Size;
  }
  Dimmer[Select].CurrentBrightness = CurrentBrightness;

  if (CurrentBrightness == 0) {
    OCR_Value = 0; // turn off
    DeltaDali = 0;
  } else {
    OCR_Value = DaliTable[CurrentBrightness-1];
    if (Dimmer[Select].Mode == DimmerModeFadeUp) {
      if (CurrentBrightness == LUT_DALI_Size) {
        DeltaDali = 0;
      } else {
        DeltaDali = DaliTable[CurrentBrightness-1]-DaliTable[CurrentBrightness];
        DeltaCurrentBrightness = (uint8_t)((Value32*256) / Dimmer[0].DeltaCycle);
        Value16 = (uint16_t)((((uint32_t)DeltaDali*DeltaCurrentBrightness)+128)/256);
        OCR_Value -= Value16;
      }
    } else {
      if (CurrentBrightness == 1) {
        DeltaDali = 0;
      } else {
        DeltaDali = DaliTable[CurrentBrightness-2]-DaliTable[CurrentBrightness-1];
        DeltaCurrentBrightness = (uint8_t)((Value32*256) / Dimmer[Select].DeltaCycle);
        Value16 = (uint16_t)((((uint32_t)DeltaDali*DeltaCurrentBrightness)+128)/256); 
        OCR_Value += Value16;
      }
    }
  }  
  Dimmer[Select].CurrentOCR = OCR_Value;
  // Check if done with fade
  if (CurrentCount == Dimmer[Select].EndCycle) {
    Dimmer[Select].Mode = DimmerModeFadePostAction;
    debug_tiny_printf("Done with Fade\n");
  }
  PORTB &= ~(1<<PORTB3);  // Clear D11
}

void Dimmer_SetFade(Dimmer_Select_t Select, uint32_t DurationMS, uint8_t EndBrightness) {
  Dimmer[Select].StartCycle = Dimmer_CurrentCycle;
  Dimmer[Select].DeltaCycle = ((ExternalDimmer_MAINS_HZ*2)*(uint32_t)DurationMS)/1000;
  Dimmer[Select].EndCycle = Dimmer[Select].StartCycle + Dimmer[Select].DeltaCycle;

#if defined(DIMMER_DEBUG_INFO)
  //tiny_printf("Dur %i StartC %i DeltaC %i EndC %i .\n", DurationMS, Dimmer[Select].StartCycle, Dimmer[Select].DeltaCycle, Dimmer[Select].EndCycle);
  debug_tiny_printf("Dur %i\n", DurationMS);
  debug_tiny_printf("StartC %i\n", Dimmer[Select].StartCycle);
  debug_tiny_printf("DeltaC %i\n", Dimmer[Select].DeltaCycle);
  debug_tiny_printf("EndC %i\n", Dimmer[Select].EndCycle);
#endif

  Dimmer[Select].StartBrightness = Dimmer[Select].CurrentBrightness;
  Dimmer[Select].EndBrightness = EndBrightness;

  if (Dimmer[Select].StartBrightness < Dimmer[Select].EndBrightness) {
    Dimmer[Select].Mode = DimmerModeFadeUp;
    Dimmer[Select].DeltaBrightness = Dimmer[Select].EndBrightness - Dimmer[Select].StartBrightness;
    DimmerOCR[Select].Enable = 1;
  } else if (Dimmer[Select].StartBrightness > Dimmer[Select].EndBrightness) {
    Dimmer[Select].Mode = DimmerModeFadeDown;
    Dimmer[Select].DeltaBrightness = Dimmer[Select].StartBrightness - Dimmer[Select].EndBrightness;
    DimmerOCR[Select].Enable = 1;
  } else {
    // Do nothing
  }
  
#if defined(DIMMER_DEBUG_INFO)
  //tiny_printf("StartB %u EndB %u DeltaB %u Mode %u .\n", Dimmer[Select].StartBrightness, Dimmer[Select].EndBrightness, Dimmer[Select].DeltaBrightness, Dimmer[Select].Mode);
  debug_tiny_printf("StartB %i\n", Dimmer[Select].StartBrightness);
  debug_tiny_printf("EndB %i\n", Dimmer[Select].EndBrightness);
  debug_tiny_printf("DeltaB %i\n", Dimmer[Select].DeltaBrightness);
  debug_tiny_printf("Mode %i\n", Dimmer[Select].Mode);
// TODO add more specific who was selected to debug
#endif
}

void Dimmer_SetBrightness(Dimmer_Select_t Select, uint8_t Brightness) {
  uint16_t OCR_Value;
  Dimmer[Select].DeltaBrightness = 0;
  if (Brightness == 0) {
    Dimmer[Select].Mode = DimmerModeOff;
    Dimmer[Select].CurrentBrightness = 0;
    Dimmer[Select].EndBrightness = 0;
    Dimmer[Select].CurrentOCR = 0;
    DimmerOCR[Select].Enable = 0;
  } else if (Brightness == 255) {
    if (Dimmer[Select].CurrentBrightness != 0) {
      Dimmer[Select].EndBrightness = Dimmer[Select].CurrentBrightness;
      Dimmer[Select].Mode = DimmerModeOn;
    }
  } else {
    Dimmer[Select].Mode = DimmerModeOn;
    DimmerOCR[Select].Enable = 1;
    Dimmer[Select].CurrentBrightness = Brightness;
    Dimmer[Select].EndBrightness = Brightness;
    OCR_Value = DaliTable[Brightness-1];
    Dimmer[Select].CurrentOCR = OCR_Value;
  }
  debug_tiny_printf("CurB %i\n", Dimmer[Select].CurrentBrightness);
  debug_tiny_printf("DeltaB %i\n", Dimmer[Select].DeltaBrightness);
  debug_tiny_printf("EndB %i\n", Dimmer[Select].EndBrightness);
  debug_tiny_printf("Mode %i\n", Dimmer[Select].Mode);
}

uint8_t Dimmer_GetBrightness(Dimmer_Select_t Select) {
  return Dimmer[Select].CurrentBrightness;
}

void Dimmer_SetDirectValue(Dimmer_Select_t Select, uint16_t Value) {
  Dimmer[Select].Mode = DimmerModeOn;
  Dimmer[Select].CurrentBrightness = 0;
  Dimmer[Select].EndBrightness = 0;
  Dimmer[Select].DeltaBrightness = 0;
  DimmerOCR[Select].Enable = 1;
  Dimmer[Select].CurrentOCR = Value;
  // Remark
  //  Cannot set CurrentBrightness or EndBrightness, because these are DaliValues and not OCR values
  //  To make a Fade correctly work after a SetDirect, first a Set command needs to be executed
  debug_tiny_printf("Set direct %i\n", Value);
  debug_tiny_printf("CurB %i\n", Dimmer[Select].CurrentBrightness);
  debug_tiny_printf("DeltaB %i\n", Dimmer[Select].DeltaBrightness);
  debug_tiny_printf("EndB %i\n", Dimmer[Select].EndBrightness);
  debug_tiny_printf("Mode %i\n", Dimmer[Select].Mode);
}

uint16_t Dimmer_GetDirectValue(Dimmer_Select_t Select) {
  uint16_t Value;
  Value = Dimmer[Select].CurrentOCR;
  debug_tiny_printf("Get direct %i\n", Value);
  return Value;
}