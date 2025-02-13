/************************************************************

  IR Reception Library for SWIM Project

  The IR Reception, handled with VSOP38338 equipped IR 
  receiver so that we can match the signal process
  mechanism with the SWIM's LED transmission.

  This library will handle reception and decoding of the 
  IR signal prepared with the IRTrans library.

  Implementation file.

 ************************************************************/
#include "IRRecv.h"

/* Detect Arduino */
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
//#include "WProgram.h"
#endif

/**
 * The incoming signal doesn't really have the same
 * timing we defined. It can be a bit shorter or longer.
 * We are giving 20 % margin 
 */
#define SIGNAL_TIME_MODIFIER           6/10

/**
 *
 * A private methods to read incoming IR
 * signal..
 *
 */
/***************************************
 *
 * Reads pulse width in um...
 * --> perhaps not very useful at an
 *     embedded Linux based stuff.
 * 
 * --> Could use libuv library on those
 *     Linux based ones. But lets see.
 * 
 ***************************************/
uint32_t pulse_width(IRRecv* irRecv) {

  uint32_t start;

  start = micros();
  while (irRecv->ReadIRPin(irRecv) == 0) {
    if (micros() - start > PULSE_TIMEOUT) {
      return PULSE_TIMEOUT;
    }
  }

  start = micros();
  while (irRecv->ReadIRPin(irRecv) == 1) {
    if (micros() - start > PULSE_TIMEOUT) {
      return PULSE_TIMEOUT;
    }
  }

  return micros() - start;
}

/**
 * Voting algorithm for the repeated buffers (uint64_t data)
 */
uint64_t vote(uint64_t* data_set, uint8_t n_data, uint8_t bits)
{
  if (!data_set) return 0;
  if (n_data <= 1) return data_set[0];

  int i, di;
  uint64_t data = 0;
  uint32_t n_ones, n_zeros;

  for(i=bits-1; i>=0; i--) {
    n_ones = 0;
    n_zeros = 0;

    for (di=0; di<n_data; ++di) {
      ((data_set[di]>>i) & 0x1) ? n_ones++ : n_zeros++ ;
    }

    if (n_ones >= n_zeros) data += (1<<i);
  }

  return data;
}

/**
 *
 * Method definitions for IRRecv
 *
 */
/**
 * calc_period wrpper for IRRecv
 */
void calc_period_irrecv(IRRecv* irRecv)
{
  irRecv->irComm->CalcPeriod(irRecv->irComm);

  irRecv->period_one = \
    irRecv->irComm->period * PULSES_FOR_ONE;
  irRecv->period_zero = \
    irRecv->irComm->period * PULSES_FOR_ZERO;
  irRecv->period_empty = \
    irRecv->irComm->period * PULSES_FOR_EMPTY;
  irRecv->period_gap = \
    irRecv->irComm->period * PULSES_FOR_GAP;
  irRecv->period_header_one = \
    irRecv->irComm->period * PULSES_FOR_HEADER_ONE;
}

/**
 * Initialize IR Recv pin
 */
void init_irrecv(IRRecv* irRecv)
{
  #if defined(ARDUINO) && ARDUINO >= 100
  /* So far, we found Pi Pico can work with pinMode in library. But not in MBED RP2040 */
  #  if !defined(ARDUINO_ARCH_MBED_NANO) && !defined(ARDUINO_ARCH_MBED_RP2040) && !defined(ARDUINO_NANO_RP2040_CONNECT)
    pinMode(irRecv->irComm->IR_Pin, INPUT);
  #  else
     /* pinMode does not work with MBED RP2040 or any MBED boards. */
     /* Do try to define pinMode in the main setup() instead */
  #  endif
  #else
    /* Some other wrapper code for GPIO pin mode setting. */
  #endif
}

/**
 * Read in IR Pin wrapper - Hardware code that actually sets up GPIO
 * Update this part as we add up more devices.
 */
int read_ir_pin_irrecv(IRRecv* irRecv)
{
  /* Arduino case: gotta implement more 'elegant' way to handle multi-platform */
  /* 
   Note that VSOP38338 returns negative logic, 
   normally 1, signal as 0
   */
  #if defined(ARDUINO)
  return ( digitalRead(irRecv->irComm->IR_Pin) ? 0 : 1 );
  #else
  /* fill up with other platform's interface */
  #endif
}

/**
 * Decode pulse length encoded digit
 */
int recv_irrecv(IRRecv* irRecv)
{
  uint32_t duration;
  bool signal_arrived = false;

  while (true) {

    while(!signal_arrived) {
      if (irRecv->ReadIRPin(irRecv) == 1) {
        signal_arrived = true;
        break;
      }
    }

    while(signal_arrived) {
      duration = pulse_width(irRecv);
      if ( duration >= PULSE_TIMEOUT ) {
        return ERROR_RECV;
      }
      else {
        return ( duration >= irRecv->period_one ? 1 : 0 );
      }
    }
  }
}

/**
 * Read in IR encoded 1/0 signal with designated number of bits
 */
uint32_t read_data_irrecv(IRRecv* irRecv, uint8_t bits)
{
  int data;
  uint32_t retn_data = 0;
  
  /* Receiving the data and filling up the buffer */
  for (uint8_t i=0; i<bits; ++i) {

    data = irRecv->Recv(irRecv);

    if (data != 0 || data != 1) {
      i--;
      continue;
    }

    if (data == 1) {
      retn_data += (1<<(bits-1-i));
    }
  }
  
  return retn_data;
}

/**
 * Receive the SWIM packet.
 *
 * Basically, we need to detect long 1-0 start bit and 
 * the repeats. Then loads into the buf.
 *
 */
int recv_packet_irrecv(IRRecv* irRecv, uint64_t* buf, uint8_t bits)
{
  uint32_t duration, start;
  RecvState state = IDLE;
  uint8_t data_index, buf_index, data;
  int err_code;

  (*buf) = 0;
  for(int i=0; i<irRecv->repeat; i++) {
    irRecv->tmp_buf[i] = 0U;
  }

  while (true) {

    /* Waiting for the start signal */
    start = millis();
    while(state == IDLE) {

      if (irRecv->ReadIRPin(irRecv) == 1) {
        state = PKT_ARRIVED;
        break;
      }

      if (millis() - start > PACKET_TIMEOUT*irRecv->irComm->period) {
        state = ERROR_IDLE_TIMEOUT;
        break;
      }

    } /* while(state == IDLE) */
    
    /* Once the packet is arrived */
    while(state == PKT_ARRIVED) {

      duration = pulse_width(irRecv);

      if (duration >= PULSE_TIMEOUT) {
        err_code = ERROR_RECV;
        state = IDLE;
        break;
      }
      else {
        if (duration >= irRecv->period_header_one) {
          state = PKT_READ;
          buf_index  = 0;
          data_index = 0;
        }
      }
      break;
    } /* while(state == PKT_ARRIVED) */

    /* Actually reading the packets, 1/0 data */
    while(state == PKT_READ) {

      duration = pulse_width(irRecv);

      if (duration >= PULSE_TIMEOUT) {
        err_code = ERROR_PKT_READ;
        state = ERROR;
        break;
      }
      
      data = (duration >= irRecv->period_one) ?  1 : 0;
      if (data) {
        /* Collect data to tmp_buf */
        irRecv->tmp_buf[buf_index] += (1<<(bits-1-data_index));
      }
      data_index++;
      
      if (data_index >= bits) {
        state = PKT_GAP;
        data_index = 0;
        break;
      }
    } /* while(state == PKT_READ) */

    /* Handling the gap */
    while(state == PKT_GAP) {

      duration = pulse_width(irRecv);

      if (duration >= PULSE_TIMEOUT) {
        err_code = ERROR_GAP_READ;
        state = ERROR;
        break;
      }

      if (duration >= irRecv->period_gap) {

        buf_index++;
      
        if (buf_index >= irRecv->repeat - 1) {
          err_code = 0;
          state = FINISH;
        }
        else {
          state = PKT_READ;
        }
        break;
      }
    } /* while(state == PKT_GAP) */

    /* Finalizing the received signal */
    if (state == FINISH) {
      (*buf) = vote(irRecv->tmp_buf, irRecv->repeat, bits);
      //(*buf) = irRecv->tmp_buf[0];
      return IRRECV_SUCCESS;
    } /* if (state == FINISH) */

    /* Error exit */
    if (state == ERROR) {
      return err_code;
    } /* if (state == ERROR) */

  } /* while(true) */
}


/****************************************************
 *
 * Constructors and Destructors for IRRecv
 *
 ****************************************************/
IRRecv* IRRecv_create_skel(void)
{
  IRRecv* irRecv = (IRRecv*)malloc(sizeof(IRRecv));

  /* Initializing the parent class(?) */
  irRecv->irComm     =   IRComm_create(DEF_IR_PIN);
  irRecv->CalcPeriod =   &(calc_period_irrecv);
  irRecv->Init       =   &(init_irrecv);

  irRecv->ReadIRPin  =   &(read_ir_pin_irrecv);
  irRecv->Recv       =   &(recv_irrecv);
  irRecv->ReadData   =   &(read_data_irrecv);
  irRecv->RecvPacket =   &(recv_packet_irrecv);

  irRecv->irComm->mod_freq = DEF_MOD_FREQ;
  irRecv->CalcPeriod(irRecv);

  irRecv->repeat = PACKET_REPEAT;

  irRecv->tmp_buf = (uint64_t*)malloc(sizeof(uint64_t)*irRecv->repeat);
  for (int i=0; i<irRecv->repeat; i++) {
    irRecv->tmp_buf[i] = 0U;
  }

  return irRecv;
}

IRRecv* IRRecv_create(uint8_t ir_pin)
{
  IRRecv* irRecv = IRRecv_create_skel();
  irRecv->irComm->IR_Pin = ir_pin;

  return irRecv;
}

IRRecv* IRRecv_create_with_freq(uint8_t ir_pin, uint32_t mod_freq)
{
  IRRecv* irRecv = IRRecv_create(ir_pin);
  irRecv->irComm->mod_freq = mod_freq;
  irRecv->CalcPeriod(irRecv);

  return irRecv;
}

void IRRecv_destroy(IRRecv* irRecv)
{
  if(irRecv) {
    if (irRecv->irComm) IRComm_destroy(irRecv->irComm);
    free(irRecv->tmp_buf);
    free(irRecv);
  }
}