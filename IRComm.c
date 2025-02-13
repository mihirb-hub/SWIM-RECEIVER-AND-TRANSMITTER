/****************************************************************

  IR Communication Protocol Implementation

  This file contains a few 'common' default parameters and
  methods for IRTrans and IRRecv libraries for SWIM project.

  Implementation file...

 ****************************************************************/

/* The header */
#include "IRComm.h"

/**
 * 
 * Calculates IR signal period in us
 * 
 * Strictly using integers instead of floating point operation.
 * 
 * MCUs hate floating point stuffs. Also, comparing floating point
 * numbers usually ends up total pain in the butt in C/CPP.
 * 
 */
void calc_period(IRComm* irComm) 
{
  if (irComm->mod_freq == 0) irComm->mod_freq = 1;

  if (irComm->mod_freq < 1000) {
    irComm->mod_freq *= 1000;
  }

  // Rounded up period in us. Roughly 26-27 us.
  uint32_t period = \
    (1000000UL + irComm->mod_freq / 2) / irComm->mod_freq;
  irComm->period = (period > 1) ? period : 1;
}

/**
 * Constructor for default parameters.
 * 1. 38000 Hz
 * 2. D11 pin 
 */
IRComm* IRComm_skel(void)
{
  IRComm* irComm = (IRComm*)malloc( sizeof(IRComm) );
  irComm->mod_freq     = DEF_MOD_FREQ;
  irComm->IR_Pin       = DEF_IR_PIN;

  irComm->CalcPeriod   = &(calc_period);

  irComm->CalcPeriod(irComm);

  return irComm;
}

/**
 * Constructor with custom ir_pin: Recommended for actual usage.
 * 1. 38000 Hz carrier frequency.
 * 2. Pin number
 */
IRComm* IRComm_create(uint8_t ir_pin)
{
  IRComm* irComm = IRComm_skel();
  irComm->IR_Pin = ir_pin;
  irComm->parity_bits = DEF_PARITY_BITS;
  
  return irComm;
}

/** 
 * Constructor for custom modulation frequency.
 * 1. 38000 Hz carrier frequency.
 * 2. Pin number
 */
IRComm* IRComm_create_with_freq(uint8_t ir_pin, uint32_t mod_freq)
{
  IRComm* irComm = IRComm_create(ir_pin);
  irComm->mod_freq = mod_freq;
  irComm->CalcPeriod(irComm);

  return irComm;
}

/**
 * IRComm Destructor 
 */
void IRComm_destroy(IRComm* irComm)
{
  if (irComm) free(irComm);
}