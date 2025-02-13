/************************************************************

  IR Reception Library for SWIM Project

  The IR Reception, handled with VSOP38338 equipped IR 
  receiver so that we can match the signal process
  mechanism with the SWIM's LED transmission.

  This library will handle reception and decoding of the 
  IR signal prepared with the IRTrans library.

  Header file.

 ************************************************************/
#ifndef __IRRECV_H__
#define __IRRECV_H__

/**
 * 
 * Some basic includes
 * 
 */
#include <stdint.h>
#include <stdlib.h>

#include "IRComm.h"

/* Timeout pulse length */
#define PULSE_TIMEOUT    25000
#define PACKET_TIMEOUT   100000

/* ERROR handling stuffs */
#define ERROR_RECV         -1
#define ERROR_PKT_READ     -2
#define ERROR_GAP_READ     -3
#define ERROR_IDLE_TIMEOUT -6
#define IRRECV_SUCCESS     0

/**
 * 
 * The main struct for IRRecv
 * 
 */
typedef struct __ir_recv__ {

  IRComm* irComm;

  uint32_t high_period; // Should be 1/2 of period for 50% duty cycle. But...
  uint32_t period_one;  // positive period for one and zero
  uint32_t period_zero;
  uint32_t period_empty;
  uint32_t period_header_one;
  uint32_t period_gap;
  uint8_t  repeat;

  uint64_t* tmp_buf;

  void (*CalcPeriod)(struct __ir_recv__*);
  void (*Init)(struct __ir_recv__*);

  int (*ReadIRPin)(struct __ir_recv__*);
  int (*Recv)(struct __ir_recv__*);
  uint32_t (*ReadData)(struct __ir_recv__*, uint8_t);
  int (*RecvPacket)(struct __ir_recv__*, uint64_t*, uint8_t);

} IRRecv;

/**
 * An enum to handle the packet receiving
 */
typedef enum __recv_state__ {
  IDLE,
  PKT_ARRIVED,
  PKT_READ,
  PKT_GAP,
  ERROR,
  FINISH
} RecvState; 

/**
 * 
 * Method definitions for IRRecv
 * 
 */
#ifdef __cplusplus
extern "C" {
#endif

void calc_period_irrecv(IRRecv* irRecv);
void init_irrecv(IRRecv* irRecv);

int read_ir_pin_irrecv(IRRecv* irRecv);
int recv_irrecv(IRRecv* irRecv);
uint32_t read_data_irrecv(IRRecv* irRecv, uint8_t bits);
int recv_packet_irrecv(IRRecv* irRecv, uint64_t* buf, uint8_t bits);

/**
 *
 * Constructors and Destructors for IRRecv
 * 
 */
IRRecv* IRRecv_create_skel(void);
IRRecv* IRRecv_create(uint8_t ir_pin);
IRRecv* IRRecv_create_with_freq(uint8_t ir_pin, uint32_t mod_freq);

void IRRecv_destroy(IRRecv* irRecv);

#ifdef __cplusplus
} /* Matching } for the extern C */
#endif


#endif /* Include Guard */