/****************************************************************

  SnH operation test with Raspberry Pi Pico (Or RP2040 Connect)

  Main purpose of this code is to test 
  the anually coded IR Receiver functionality.

  Required Libraries: N/A

*****************************************************************/

// Courtesy include for Platform.IO
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
//#include "WProgram.h"
#endif

#define DEBUG
#include "Debug.h"

#include "IRRecv.h"

/* Some default macros */
#define THE_DELAY          2000
#define SERIAL_BAUD_RATE   115200

#define SIG_SUCCESS        0
#define SIG_FAILURE        255

/* IR will be connected to GPIO 14 of Pi Pico */
#define IR_LED             14

/* ADC and other data bit width */
#define ADC_BITS           12
#define ADDR_BITS          5
#define PARITY_BITS        1

/* Defining IR Communication Protocol */
IRRecv *ir_recv;

/* Some global variables... */
int data, data_old;
uint32_t packet, packet_old;
uint64_t counter = 0;
uint64_t recv_data;
int recv_state;

uint32_t addr_data;
uint32_t adc_data;
uint32_t parity_data;

/* Bit to string */
void prntBits(uint32_t b, uint8_t bits)
{
  uint8_t bit;
  for(int i = bits-1; i >= 0; i--) {
    bit = (uint8_t)((b>>i)&0x1);
    Serial.print(bit);
  }   
}

/* Parse recv data */
void parse_recv_data(uint64_t packet_data, uint32_t* addr, uint32_t* adc_data, uint32_t* parity)
{
  (*addr)     = (packet_data & (0x1F000<<PARITY_BITS)) >> (ADC_BITS+PARITY_BITS);
  (*adc_data) = ((packet_data & (0xFFF<<PARITY_BITS)) >> PARITY_BITS);
  (*parity)   = (packet_data & 0x1);
}

/* parity check */
bool parity_check(uint64_t packet_data)
{
  uint32_t parity = (packet_data & 0x1);
  uint32_t actual_data = ((packet_data >> PARITY_BITS) & 0x1FFFF);
  uint8_t  n_of_one = 0;

  for(int i=(ADDR_BITS+ADC_BITS-1); i>=0; i--) {
    if ( (actual_data>>i) & 0x1 ) n_of_one++;
  }

  return ( (n_of_one % 2) == parity ) ? true : false;
}

/**
 * 
 * The Setup
 *
 */
void setup() {
  
  /* Initializing the serial communication */
  Serial.begin(SERIAL_BAUD_RATE);

  /* Initializing IR receiver */
  Serial.print(F("IR Input pin: GPIO"));
  Serial.print(IR_LED);
  Serial.print(F("\n"));
  ir_recv = IRRecv_create(IR_LED);
  ir_recv->Init(ir_recv);

  pinMode(LED_BUILTIN, OUTPUT);

  data_old = 0;

  recv_data = 0;

} /* The Setup */


/**
 *
 * The Loop()
 *
 */
void loop() {
  
  recv_state = ir_recv->RecvPacket(ir_recv, &recv_data, ADDR_BITS+ADC_BITS+PARITY_BITS);

  parse_recv_data(recv_data, &addr_data, &adc_data, &parity_data);

  digitalWrite(LED_BUILTIN, HIGH);

  Serial.print(F("["));
  Serial.print(counter);
  Serial.print(F("] Ch: "));
  Serial.print(addr_data, DEC);
  Serial.print(F(" ADC_Data: "));
  Serial.print(adc_data);
  Serial.print(F(" BIN: "));
  prntBits((uint32_t)adc_data, ADC_BITS);
  Serial.print(F(" Par: "));
  prntBits(parity_data, PARITY_BITS);
  Serial.print(F(" State: "));
  Serial.print(recv_state);
  Serial.print(F(" Raw: "));
  prntBits(recv_data, ADDR_BITS+ADC_BITS+PARITY_BITS);
  Serial.print(F(" PChk: "));
  Serial.print( parity_check(recv_data) ? F("OK") : F("FAIL") );
  Serial.print(F("\n"));

  packet_old = packet;
  counter++;

  if (counter > 500000) counter = 0;

  digitalWrite(LED_BUILTIN, LOW);

} /* The Loop */
