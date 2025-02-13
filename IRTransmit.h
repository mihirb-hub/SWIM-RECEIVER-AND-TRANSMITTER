/************************************************************

  IR Transmission Library for SWIM Project

  A simple LED (IR or regular blue) blinking library to
  tranmit and data via IR modulation frequency to utilize
  the VSOP38338 chip based PD receiver.

  Mainly, this library will provide encoding a digital 
  bit data to IR modulated blinking signal chain with 
  SWIM-specific data transmission format.

  Header file.

 ************************************************************/
#ifndef __IR_TRANSMIT_H__
#define __IR_TRANSMIT_H__

/**
 *
 * Some basic includes 
 *
 */
#include <stdint.h>
#include <stdlib.h>

/******************************
 * 
 * Some default parameters
 * and hardware control 
 * methods..
 * 
 ******************************/
#include "IRComm.h"

/**
 * 
 * The main struct for IRComm
 * 
 */
typedef struct __ir_transmit__ {

  IRComm*  irComm; // Super class, irComm (or Parent class)

  uint32_t high_period; // Should be 1/2 of modulation period for 50% duty cycle. But...
  uint32_t pulses_one;  // positive period for one and zero
  uint32_t pulses_zero;
  uint32_t pulses_empty;
  uint32_t pulses_header_one;
  uint32_t pulses_header_empty;
  uint32_t pulses_gap;

  uint8_t  repeat;

  void (*CalcPeriod)(struct __ir_transmit__*);
  void (*Init)(struct __ir_transmit__*);

  int  (*WriteIRPin)(struct __ir_transmit__*, uint8_t);

  void (*SendOne)(struct __ir_transmit__*);
  void (*SendZero)(struct __ir_transmit__*);
  void (*SendPacket)(struct __ir_transmit__*, uint8_t, uint64_t);
  void (*SendHeader)(struct __ir_transmit__*);
  void (*SendGap)(struct __ir_transmit__*);

  uint32_t (*GetPeriod)(struct __ir_transmit__*);
  uint32_t (*GetModFreq)(struct __ir_transmit__*);
  uint8_t  (*GetIRPin)(struct __ir_transmit__*);

} IRTrans;

#ifdef __cplusplus
extern "C" {
#endif

/******************************************
 * 
 * The actual methods 
 * 
 ******************************************/
void calc_period_irtrans(IRTrans* irTrans);
void init_irtrans(IRTrans* irTrans);

int  write_ir_pin_irtrans(IRTrans* irTrans, uint8_t);

int set_parity(uint32_t data, uint8_t bits, uint8_t parity_bits);

void send_bit(IRTrans* irTrans, uint32_t high_cnt, uint32_t low_cnt);
void send_one(IRTrans* irTrans);
void send_zero(IRTrans* irTrans);
void send_packet(IRTrans* irTrans, uint8_t packet_bits, uint64_t packet);
void send_header(IRTrans* irTrans);
void send_gap(IRTrans* irTrans);

uint32_t get_period_irtrans(IRTrans* irTrans);
uint32_t get_mod_freq(IRTrans* irTrans);
uint8_t get_ir_pin(IRTrans* irTrans);

/***************************************
 * 
 * Constructors and destructors for 
 * irTrans struct. (or class if it were c++)
 * 
 ****************************************/
/**
 * 
 * Create IRTrans with ir_pin: Default mode
 * 
 */
IRTrans* IRTrans_create(int ir_pin);
/**
 * 
 * Create IRTrans with ir_pin and a custom modulation frequency.
 * 
 */
IRTrans* IRTrans_create_with_freq(int ir_pin, uint32_t mod_freq);
/**
 * 
 * Destructor for IRTrans
 * 
 */
void IRTrans_destroy(IRTrans* irTrans);

#ifdef __cplusplus
} /* Matching } for the extern C */
#endif



#endif /* Include Guard */