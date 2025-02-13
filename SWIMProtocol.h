/************************************************************

  SWIM Communication Protocol API

  This library chanels communication between 
  the submerged chamber and the surface machinery. 

  The communication physical layer is actually blue LED. 
  However, the blinking method adopts IR communication
  protocol.

  The command structure and packet contents are already
  defined the SWIM-Protocol presentation document.

  Header file.

  TODO: Devise a way to actively switch the IR pin mode in RP2040

 ************************************************************/
#ifndef __SWIM_PROTOCOL_H__
#define __SWIM_PROTOCOL_H__

/* Some basic includes */
#include <stdlib.h>
#include <stdint.h>
/*
 * Some boolean stuffs...
 */
#ifndef __cplusplus
#include "cbool.h"
#endif

/* SWIM IR Libraries */
#include "IRTransmit.h"
#include "IRRecv.h"

/* FIFO library */
#include "FIFO.h"

/* SWIM Communication parameters */
#ifndef SWIM_FIFO_DEPTH
#define SWIM_FIFO_DEPTH                  30
#endif

/************************************************************
 * 
 * SWIM Command List 
 *
 * Refer SWIM-Communication_Protocol document.
 *
 ************************************************************/
#define SWIM_CMD_SLEEP                   0x0
#define SWIM_CMD_READ_ALL                0x1
#define SWIM_CMD_READ_ONE                0x2
#define SWIM_CMD_READ_BATT               0x3
#define SWIM_CMD_READ_FPGA_TEMP          0x4
#define SWIM_CMD_READ_UPTIME             0x5
#define SWIM_CMD_RESERVED                0x6        /* Reserved for later use */
#define SWIM_CMD_WAKEUP                  0x7

/************************************************************
 *
 * Data bits for the data types
 *
 ************************************************************/
#define SWIM_CMD_DATA_BITS               8
#define SWIM_CHAN_DATA_BITS              17
#define SWIM_BATT_DATA_BITS              8
#define SWIM_UPTIME_DATA_BITS            32
#define SWIM_TEMP_DATA_BITS              8
#define SWIM_PARITY_BITS                 1
#define SWIM_ACK_BITS                    3
#define SWIM_CHAN_ADDR_BITS              5
#define SWIM_ADC_DATA_BITS               12

#define SWIM_FIFO_ADC_ADDR_GAP_BITS      3
#define SWIM_FIFO_HEADER_SPACE_BITS      12

/************************************************************
 *
 * Command Data Masks for packet bit operations
 *
 ************************************************************/
#define SWIM_CHAN_DATA_TRANS_MASK        0x1F000    /* or, 0b11111000000000000 */
#define SWIM_ADC_DATA_TRANS_MASK         0xFFF      /* or, 0b00000111111111111 */

#if SWIM_PARITY_BITS == 1
#define SWIM_CHAN_DATA_RECV_MASK         0x3E000
#define SWIM_ADC_DATA_RECV_MASK          0x1FFE
#elif SWIM_PARITY_BITS == 2
#define SWIM_CHAN_DATA_RECV_MASK         0x7C000
#define SWIM_ADC_DATA_RECV_MASK          0x3FFC
#else
#define SWIM_CHAN_DATA_RECV_MASK         (SWIM_CHAN_DATA_TRANS_MASK<<SWIM_PARITY_BITS)  
#define SWIM_ADC_DATA_RECV_MASK          (SWIM_ADC_DATA_TRANS_MASK<<SWIM_PARITY_BITS)            
#endif

#define SWIM_CMD_MASK                    0x7
#define SWIM_CMD_CHADDR_MASK             0x1F

/************************************************************
 *
 * FIFO Masks for bit operations
 *
 ************************************************************/
#define FIFO_ADC_DATA_MASK               0xFFF
#define FIFO_ADC_ADDR_MASK               0xF8000
#define FIFO_ADC_ADDR_SHIFT              SWIM_FIFO_ADC_ADDR_GAP_BITS
#define FIFO_DATA_MASK                   (FIFO_ADC_DATA_MASK|FIFO_ADC_ADDR_MASK) /* or 0xF8FFF */

/************************************************************
 *
 * Some basic status report codes
 *
 ************************************************************/
#define SWIM_SUCCESS                     0
#define SWIM_FAILURE                     -1
#define SWIM_PIN_OUTPUT                  0
#define SWIM_PIN_INPUT                   1

/************************************************************
 *
 * SWIM ACK code.
 *
 ************************************************************/
#define SWIM_ACK                         0b111

/************************************************************
 *
 * The SWIM Protocol Struct
 *
 ************************************************************/
typedef struct __swim_protocol__ {

  IRRecv*       Recv;
  IRTrans*      Trans;
  FIFO*         spFIFO;

  uint8_t       cmd_cache;
  uint8_t       battery_level;
  uint32_t      uptime;

  uint8_t       pin_mode; /* 0 for output, 1 for input */

  int           (*SendCmd)(struct __swim_protocol__*, uint8_t, uint32_t);
  int           (*SendData)(struct __swim_protocol__*);
  int           (*ReadCmd)(struct __swim_protocol__*);

  int           (*ReadAll)(struct __swim_protocol__*);
  uint16_t      (*ReadOne)(struct __swim_protocol__*);
  int           (*SendWakeUp)(struct __swim_protocol__*);
  int           (*SendSleep)(struct __swim_protocol__*);
  uint32_t      (*ReadUptime)(struct __swim_protocol__*);
  uint32_t      (*ReadTemp)(struct __swim_protocol__*);

} SWIMProtocol;

/************************************************************
 *
 * Methods for SWIMProtocol
 *
 ************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

/**
 *
 * Actually sends the command...
 * SWIMProtocol->SendCmd(SWIMProtocol*, command in uint8_t)
 * --> Returns 0 if successful ack received, else -1
 *
 */
int sendcmd_swim_protocol(SWIMProtocol* s_prot, uint8_t cmd, uint32_t ch_addr);

/**
 *
 * Actually sends the data... according to the received command
 * SWIMProtocol->SendData(SWIMProtocol*)
 * --> Returns 0 if successful ack received, else -1
 *
 */
int senddata_swim_protocol(SWIMProtocol* s_prot);

/**
 *
 * Parses the command to react
 * SWIMProtocol->ReadCmd(SWIMProtocol*)
 * --> Returns 0 if successful
 * 
 */
int readcmd_swim_protocol(SWIMProtocol* s_prot);

/**
 *
 * Read all data from the FIFO --> Reads data from all 30 data channels
 * SWIMProtocol->ReadAll(SWIMProtocol*)
 * --> Saves all the 30 data into Internal FIFO
 *
 */
int readall_swim_protocol(SWIMProtocol* s_prot);

/**
 *
 * Read one data from the FIFO
 * SWIMProtocol->ReadOne(SWIMProtocol*)
 * --> Returns the actual data instead of populating FIFO
 *
 */
uint16_t readone_swim_protocol(SWIMProtocol* s_prot);

/**
 *
 * Sends 'Wake Up' signal to the submerged unit
 * SWIMProtocol->SendWakeUp(SWIMProtocol*)
 * --> Returns 0 if successful ack, else -1
 *
 */
int send_wakeup_swim_protocol(SWIMProtocol* s_prot);

/**
 *
 * Sends 'Sleep' signal to the submerged unit
 * SWIMProtocol->SendSleep(SWIMProtocol*)
 * --> Returns 0 if successful ack, else -1
 *
 */
int send_sleep_swim_protocol(SWIMProtocol* s_prot);

/**
 *
 * Reads 'Uptime' data from the submerged unit
 * SWIMProtocol->ReadUptime(SWIMProtocol*)
 * --> Returns uptime as uint32_t
 *
 */
uint32_t read_uptime_swim_protocol(SWIMProtocol* s_prot);

/**
 *
 * Reads 'FPGA temperature: XADC Channel 1' data from the submerged unit
 * SWIMProtocol->ReadTemp(SWIMProtocol*)
 * --> Returns the temperature as uint32_t
 *
 */
uint32_t read_temp_swim_protocol(SWIMProtocol* s_prot);


/************************************************************
 *
 * Constructors and Destructors
 *
 ************************************************************/
/**
* SWIMProtocol Constructors
*/
SWIMProtocol* SWIMProtocol_create(void);
SWIMProtocol* SWIMProtocol_create_with_params(
                uint8_t ir_pin, uint32_t mod_freq, uint32_t fifo_depth);

/**
 * SWIMProtocol Destructor
 */
void SWIMProtocol_destroy(SWIMProtocol* s_prot);


#ifdef __cplusplus
} /* Matching } for the extern C */
#endif


#endif /* Include Guard */