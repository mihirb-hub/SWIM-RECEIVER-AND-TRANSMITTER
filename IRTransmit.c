/************************************************************

  IR Transmission Library for SWIM Project

  A simple LED (IR or regular blue) blinking library to
  tranmit and data via IR modulation frequency to utilize
  the VSOP38338 chip based PD receiver.

  Mainly, this library will provide encoding a digital 
  bit data to IR modulated blinking signal chain with 
  SWIM-specific data transmission format.

  The implementation file.

 ************************************************************/
#include "IRTransmit.h"

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
//#include "WProgram.h"
#endif

/**************************

  irTrans related stuffs

***************************/
/**
 * Calculates single pulse's period in us
 * 
 * Wrapper for IRComm->CalcPeriod
 * 
 */
void calc_period_irtrans(IRTrans* irTrans)
{
  irTrans->irComm->CalcPeriod(irTrans->irComm);
  /*
    Strangely, the single modulation cycle's duty cycle isn't really 50%
    for signal stability.
    --> Can be different in LED case. 
  */
  irTrans->high_period = irTrans->irComm->period * 4 / 5;

  irTrans->pulses_one  = PULSES_FOR_ONE;
  irTrans->pulses_zero = PULSES_FOR_ZERO;
  irTrans->pulses_empty = PULSES_FOR_EMPTY;
  irTrans->pulses_header_one = PULSES_FOR_HEADER_ONE;
  irTrans->pulses_header_empty = PULSES_FOR_HEADER_EMPTY;
  irTrans->pulses_gap = PULSES_FOR_GAP;
}

/**
 * Initializes the transmission mode 
 * 
 * 0 for transmission (output)
 * 1 for receive (input)
 * 
 * Caveat: RP2040 Connect cannot understand pinMode()
 * from external library. So, please use 
 * pinMode(your_ir_pin, OUTPUT/INPUT)
 * in your .ino file...
 *
 */
void init_irtrans(IRTrans* irTrans) 
{
  #if defined(ARDUINO)
  /* Pin setting codes for some other Arduino boards... */
  #else
  pinMode(irTrans->irComm->IR_Pin, OUTPUT);
  #endif
}

/**
 * Interface for irPin writer
 * 
 */
int write_ir_pin_irtrans(IRTrans* irTrans, uint8_t logic)
{
  /* Arduino case: gotta implement more 'elegant' way to handle multi-platform */
  #if defined(ARDUINO)
  digitalWrite(irTrans->irComm->IR_Pin, logic);
  return 0;
  #else
  /* fill up with other platform's interface */
  #endif
}

/**
 * Set parity
 * 
 * Determine parity bit polarity depending on 
 * the packet. Private function.
 *
 * Inputs: packet in uint32_t, # of bits to be actually sent out.
 * Output: Parity polarity. 1 or 0.
 *
 * Basically, if a packet has even number of 1s, parity is 0,
 * odd number of 1, parity is 1.
 *
 */
int set_parity(uint32_t data, uint8_t bits, uint8_t parity_bits)
{
  uint32_t cnt_one = 0;

  switch (parity_bits) {

    case 1:
      for (uint8_t i=0; i<bits; i++) {
        if ((data>>i) & 0x1) cnt_one++;
      }
      return (int)(cnt_one%2);
      
    case 2:
      /* First parity bit is even parity, 2nd one is odd parity. */
      for (uint8_t i=0; i<bits; i++) {
        if ((data>>i) & 0x1) cnt_one++;
      }
      return (int)( ((cnt_one%2)<<1) | ((cnt_one+1)%2) );
      
    default:
      return 0;
  
  }
}

/**
 * A private function that sends the timed pulse.
 *
 * Inputs: irTrans struct
 *         high period in # of modulation freq. pulses.
 *         low period in # of modulation freq. pulses.        
 */
void send_bit(IRTrans* irTrans, uint32_t high_cnt, uint32_t low_cnt)
{
  uint32_t signal_cnt;
  uint32_t start;

  if (high_cnt > 0) {
    /* One: signal train fills in entire cycle, defined by irTrans->periods_one */
    signal_cnt = 0;
    start = micros();
    while (signal_cnt < high_cnt) {
      if (micros() - start <= irTrans->high_period) {
        irTrans->WriteIRPin(irTrans, 1);
      } else {
        irTrans->WriteIRPin(irTrans, 0);
      }

      if (micros() - start >= irTrans->irComm->period) {
        signal_cnt++;
        start = micros();
      }
    }
  }

  if (low_cnt > 0) {
    /* Trailing zero, half cycle */
    signal_cnt = 0;
    start = micros();
    irTrans->WriteIRPin(irTrans, 0);
    while (signal_cnt < low_cnt) {
      if (micros() - start > irTrans->irComm->period) {
        start = micros();
        signal_cnt++;
      }
    }
  }
}

/**
 * send_one
 * 
 * Send a positive logic signal 
 * carried by the modulation frequency.
 * 
 * The signal will be compsed of
 * <--      PERIODS_FOR_ONE      --><-- PERIODS_FOR_EMPTY-->
 * _-_-_-_      ...       -_-_-_-_-_________________________  
 * 
 */
void send_one(IRTrans* irTrans) 
{
  send_bit(irTrans, irTrans->pulses_one, irTrans->pulses_empty);
}

/**
 * send_zero
 * 
 * Send a negative logic signal 
 * carried by the modulation frequency.
 * 
 * The signal will be compsed of
 * <--      PERIODS_FOR_ZERO     --><-- PERIODS_FOR_EMPTY-->
 * _-_-_-_      ...       -_-_-_-_-_________________________  
 * 
 */
void send_zero(IRTrans* irTrans) 
{
  send_bit(irTrans, irTrans->pulses_zero, irTrans->pulses_empty);
}

/***************************************
 * 
 * Sends a packet with designated by
 * the SWIM protocol documentation.
 * 
 ***************************************/
/**
 * 
 * send_packet
 *
 * Sends the packet with designated bits.
 * Also includes the parity.
 *
 */
void send_packet(IRTrans* irTrans, uint8_t packet_bits, uint64_t packet) 
{
  uint64_t data_to_send = 0;
  int      parity;

  uint64_t mask = 0;
  for (int i=0; i<packet_bits; ++i) {
    mask += (1<<i);
  }

  /* Extend the data bits with parity bits */
  parity = set_parity(
    mask & packet, packet_bits, irTrans->irComm->parity_bits);
  data_to_send = \
    ((mask & packet) << irTrans->irComm->parity_bits) | (uint64_t)parity;

  /* Sending the start bit */
  irTrans->SendHeader(irTrans);

  for (int repeat=0; repeat<irTrans->repeat; repeat++) {
    /* Sending the actual packet */
    for (int i=packet_bits; i>=0; i--) {
      ( (data_to_send>>i) & 0x1 ) ? \
        irTrans->SendOne(irTrans) : irTrans->SendZero(irTrans); 
    }

    /* Sending the 'Gap' bit */
    if (repeat < irTrans->repeat-1) {
      irTrans->SendGap(irTrans);
    }
  }
}

/**
 * send_header
 * 
 * Send the long header to notify communication start.
 * 
 * The signal will be compsed of
 * <--  PERIODS_FOR_HEADER_ONE   --><-- PERIODS_FOR_HEADER_EMPTY -->
 * _-_-_-_      ...       -_-_-_-_-_________________________________
 *  
 */
void send_header(IRTrans* irTrans)
{
  send_bit(irTrans, irTrans->pulses_header_one, irTrans->pulses_header_empty);
}

/**
 * send_gap
 * 
 * Send the gap before repeat signals.
 * 
 * The signal will be compsed of
 * <--  PERIODS_FOR_GAP   --><-- PERIODS_FOR_EMPTY-->
 * _-_-_-_    ...    -_-_-_-_________________________
 *  
 */
void send_gap(IRTrans* irTrans)
{
  send_bit(irTrans, irTrans->pulses_gap, irTrans->pulses_empty);
}

/***************************************
 
  Basic access interfaces for IRTrans

 ***************************************/
uint32_t get_period_irtrans(IRTrans* irTrans)
{
  return irTrans->irComm->period;
}
uint32_t get_mod_freq(IRTrans* irTrans)
{
  return irTrans->irComm->mod_freq;
}
uint8_t get_ir_pin(IRTrans* irTrans)
{
  return irTrans->irComm->IR_Pin;
}

/***************************************
  
  Constructors and destructors for 
  irTrans struct. (or class if it were c++)

****************************************/
/**
 * 
 * Create IRTrans with ir_pin: Default mode
 * 
 */
IRTrans* IRTrans_create(int ir_pin) 
{
  IRTrans* irTrans = (IRTrans*)malloc(sizeof(IRTrans));

  /* Initialize from IRComm */
  irTrans->irComm = IRComm_create(ir_pin);

  irTrans->CalcPeriod = &(calc_period_irtrans);
  irTrans->Init       = &(init_irtrans);
  irTrans->WriteIRPin = &(write_ir_pin_irtrans);
  irTrans->SendOne    = &(send_one);
  irTrans->SendZero   = &(send_zero);
  irTrans->SendPacket = &(send_packet);
  irTrans->SendHeader = &(send_header);
  irTrans->SendGap    = &(send_gap);
  irTrans->GetPeriod  = &(get_period_irtrans);
  irTrans->GetModFreq = &(get_mod_freq);
  irTrans->GetIRPin   = &(get_ir_pin);

  irTrans->repeat     = PACKET_REPEAT;

  irTrans->CalcPeriod(irTrans);
  return irTrans;
}

/**
 * 
 * Create IRTrans with ir_pin and a custom modulation frequency.
 * 
 */
IRTrans* IRTrans_create_with_freq(int ir_pin, uint32_t mod_freq)
{
  IRTrans* irTrans = IRTrans_create(ir_pin);
  irTrans->irComm->mod_freq = mod_freq;
  irTrans->CalcPeriod(irTrans);

  return irTrans;
}

/**
 *  
 *  Destructor for IRTrans
 * 
 */
void IRTrans_destroy(IRTrans* irTrans)
{
    if(!irTrans) return;

    if (irTrans->irComm) IRComm_destroy(irTrans->irComm);
    free(irTrans);
}