/********************************************************

  IR Transmitter test code for SWIM system operation
  black pipe test.

  This code will directly send 1 and 0 encoded signal
  to the IR or blue LED pin.

*********************************************************/

/* Courtesy include for Platform.IO */
#include <Arduino.h>

#define DEBUG
#include "Debug.h"

/**
 * The IRTransmit library 
 */
#include "IRTransmit.h"

/** 
 * Default pins 
 */
#define SIGNAL_TRANSMIT_LED     11

/**
 * Some other default parameters
 */
#define SERIAL_BAUD_RATE        115200
#define THE_DELAY               2000

/**
 * Data size per each transmission
 */
#define ADC_BITS                12
#define ADDR_BITS               5
#define DATA_WIDTH              ADC_BITS+ADDR_BITS
#define PARITY_BITS             1

/** 
 * The IR Transmitter library for SWIM 
 */
IRTrans* ir_trans;

/**
 Some other utils for signal encoding/decoding
 */
void prntBits(uint32_t b, uint8_t bits)
{
  uint8_t bit;
  for(int i = bits-1; i >= 0; i--) {
    bit = (uint8_t)((b>>i)&0x1);
    Serial.print(bit);
  }
}

/**
 * The data table for IR or LED communication
 *
 * >>> Change this portion of the code to manipulate the communication
 *     data.
 */
#define DATA_LIST_LEN           30
const uint32_t data_list[DATA_LIST_LEN] PROGMEM = {
  0x00000001,
  0x00000002,
  0x00000003,
  0x00000004,
  0x00000005,
  0x00000006,
  0x00000007,
  0x00000008,
  0x00000009,
  0x0000000A,
  0x0000000B,
  0x0000000C,
  0x0000000D,
  0x0000000E,
  0x0000000F,
  0x00000010,
  0x00000011,
  0x00000012,
  0x00000013,
  0x00000014,
  0x00000015,
  0x00000016,
  0x00000017,
  0x00000018,
  0x00000019,
  0x0000001A,
  0x0000001B,
  0x0000001C,
  0x0000001D,
  0x0000001E
}; 
/** END of data table **/

/* The index variable */
uint32_t data_index = 0;

/** 
 * IR Signal encoder function 
 * Input1: data, up to 32 bits.
 * Input2: the actual number of bits to be sent out.
 *
 */
void IRBlink(IRTrans* IR_trans, uint32_t data, uint8_t bits)
{
  for (int i=bits-1; i>=0; i--) {
    ( (data>>i) & 0x1 ) ? IR_trans->SendOne(IR_trans) : IR_trans->SendZero(IR_trans);
  }
}


void setup() {
  /* Initializing the Serial communication */
  Serial.begin(SERIAL_BAUD_RATE);

  /* Initializing the IR library */
  ir_trans = IRTrans_create(SIGNAL_TRANSMIT_LED);
  ir_trans->Init(ir_trans);

  /* RP2040 MBED specific */
  #if defined(ARDUINO_NANO_RP2040_CONNECT) || defined(ARDUINO_ARCH_MBED_RP2040)
  pinMode(SIGNAL_TRANSMIT_LED, OUTPUT);
  #endif

  /* data index initialization*/
  data_index = 0;
}

void loop() {
  
  uint32_t d_send = (data_index << (ADC_BITS))|(data_list[data_index]&0xFFF);
  int parity = set_parity(d_send, DATA_WIDTH, PARITY_BITS);

  /* Blink LED */
  #ifdef DEBUG
  Debug(F("["));
  Debug(data_index);
  Debug(F("] "));
  Debug(F("Sending "));
  Debug(F("[Ch]: "));
  Debug(data_index);
  Debug(F(" Data: "));
  Debug(d_send & 0x7F);
  Debug(F(" ["));
  prntBits(d_send, DATA_WIDTH);
  Debug(F("]"));
  Debug(F(" Parity: "));
  prntBits((uint32_t)parity, 1);
  Debugln(F(""));
  #endif

  // IRBlink(ir_trans, data_list[data_index], DATA_WIDTH);
  ir_trans->SendPacket(
    ir_trans, 
    DATA_WIDTH, 
    (data_index << (ADC_BITS))|(data_list[data_index]&0xFFF));

  data_index++;
  if (data_index > DATA_LIST_LEN-1) {
    data_index = 0;
  }

  delay(THE_DELAY);
}
