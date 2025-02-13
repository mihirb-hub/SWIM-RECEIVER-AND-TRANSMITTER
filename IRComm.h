/****************************************************************

  IR Communication Protocol Implementation

  This file contains a few 'common' default parameters and
  methods for IRTrans and IRRecv libraries for SWIM project.

  Header file...

*****************************************************************/
#ifndef __IRComm_h__
#define __IRComm_h__

/**
 * Arduino compiler detection
 */
#if defined(ARDUINO) && ARDUINO >= 100
#include <Arduino.h>
#else
/*
 * Some other GPIO toggling library will be going here.
 * WProgram.h is a placeholder.
 */
//#include "WProgram.h"
#endif

/*
 * Some boolean stuffs...
 */
#ifndef __cplusplus
#include <stdbool.h>
#endif

/**
 * The default IR-modulation frequency
 */
#define DEF_MOD_FREQ                38000      /* 38kHz, 26.31 us */

/**
 * IR Communication pulse lengths
 */
#define PULSES_FOR_ONE              23         /* 23 for 600 us, roughly */
#define PULSES_FOR_ZERO             12         /* 35 for 900 us, roughly */
#define PULSES_FOR_EMPTY            23

#define PULSES_FOR_HEADER_ONE       46         /* 46 for 1200 us */
#define PULSES_FOR_HEADER_EMPTY     35         /* 35 for 900 us */

#define PULSES_FOR_GAP              35         /* 35 for 900 us */

/**
 * Some other IR Transmission parameters
 */
#define PACKET_REPEAT               3

/**
 * Default IR Pin - Assuming Arduino RP2040 Connect 
 */
#define DEF_IR_PIN                  11

/**
 * Some default data parameters.
 */
#define DEF_PARITY_BITS             1


/** 
 * Yet more includes 
 */
#include <stdint.h>
#include <stdlib.h>

/**
 * IRComm class
 *   
 * contains some common properties and methods 
 * that will be used by IRTrans and IRRecv.
 * 
 * This class just provides some pointers for those common
 * properties. It won't be directly accessed by a user. 
 *  
 */
typedef struct __ir_comm__ {

  uint32_t mod_freq;               /* The modulation frequency */
  uint32_t period;                 /* Single modulation pulse period. 1/mod_freq */
  uint8_t  IR_Pin;                 /* The digital signal pin number to work with */
  uint8_t  parity_bits;            /* Parity bits */

  void (*CalcPeriod)(struct __ir_comm__*);  /* The period calculation to provide modulation frequency in us */

} IRComm;

/**
 * Methods
 */
#ifdef __cplusplus
extern "C" {
#endif

/** 
 * Calculates the single modulation pulse period
 * from the mod_freq.
 *
 * Strictly using Integer operation 
 */
void calc_period(IRComm* irComm);

/**
 * Constructor for default parameters.
 * 1. 38000 Hz
 * 2. D11 pin 
 */
IRComm* IRComm_skel(void);

/**
 * Constructor with custom ir_pin: Recommended for actual usage.
 * 1. 38000 Hz carrier frequency.
 * 2. Pin number
 */
IRComm* IRComm_create(uint8_t ir_pin);

/** 
 * Constructor for custom modulation frequency.
 * 1. 38000 Hz carrier frequency.
 * 2. Pin number
 */
IRComm* IRComm_create_with_freq(uint8_t ir_pin, uint32_t mod_freq);

/**
 * IRComm Destructor 
 */
void IRComm_destroy(IRComm* irComm);


#ifdef __cplusplus
} /* Matching } for the extern C */
#endif

#endif /* Include Guard */

