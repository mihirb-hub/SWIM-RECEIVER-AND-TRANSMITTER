/************************************************************

  SWIM Communication Protocol API

  This library chanels communication between 
  the submerged chamber and the surface machinery. 

  The communication physical layer is actually blue LED. 
  However, the blinking method adopts IR communication
  protocol.

  The command structure and packet contents are already
  defined the SWIM-Protocol presentation document.

  Implementation file.

 ************************************************************/
#include "SWIMProtocol.h"


/***************************************************************************
 *
 * Private methods...
 *
 ***************************************************************************/
/**
 * 
 * SWIM Protocol - private - function matching
 * 
 */
// void SWIMProtocol_create_func_match(SWIMProtocol* s_prot)
// {
//   if (s_prot) {

//     s_prot->SendCmd     = &(sendcmd_swim_protocol);
//     s_prot->SendData    = &(senddata_swim_protocol);

//     s_prot->ReadAll     = &(readall_swim_protocol);
//     s_prot->ReadOne     = &(readone_swim_protocol);
//     s_prot->SendWakeUp  = &(send_wakeup_swim_protocol);
//     s_prot->SendSleep   = &(send_sleep_swim_protocol);
//     s_prot->ReadUptime  = &(read_uptime_swim_protocol);
//     s_prot->ReadTemp    = &(read_temp_swim_protocol);
    
//   }
// }

/**
 * check_parity
 * 
 * Simply, checking up the signal integrity with parity bit methods.
 * 
 */
bool parity_check(uint64_t packet, uint8_t data_bits, uint8_t parity_bits)
{
  uint64_t parity_bit_mask = 0U;
  uint64_t data_bit_mask = 0U;
  uint64_t parity;
  uint64_t data;

  uint8_t  n_ones, n_zeros;

  for (int i=0; i<parity_bits; i++) {
    parity_bit_mask += (1<<i);
  }

  for (int i=0; i<data_bits; i++) {
    data_bit_mask += (1<<i);
  }
  data_bit_mask = (data_bit_mask<<parity_bits);

  data = ((packet & data_bit_mask) >> parity_bits);
  parity = (packet & parity_bit_mask);

  switch (parity_bits) {

    case 0:

      return true;

    case 1:

      n_ones = 0;
      for (int i=data_bits-1; i>=0; i--) {
        if ((data>>i) & 0x1) n_ones++; 
      }

      if ((n_ones%2) == (uint8_t)parity)
        return true;
      else
        return false; 

    case 2:

      n_ones = 0; n_zeros = 0;
      for (int i=data_bits-1; i>=0; i--) {
        ((data>>i) & 0x1) ? n_ones++ : n_zeros++;
      }

      if ( ((n_ones%2) == (((uint8_t)parity>>1)&0x1)) && ((n_zeros%2) == (((uint8_t)parity)&0x1)) )
        return true;
      else
        return false; 

    default:
      return true;

  }

  return false;
}

/**
 * Parses and returns the bit width of a command
 * 
 */
uint8_t cmd_to_data_bit(uint8_t cmd)
{
  switch (cmd) {
    case SWIM_CMD_SLEEP:
      return SWIM_ACK_BITS;
    
    case SWIM_CMD_READ_ALL:
      return SWIM_CHAN_DATA_BITS;

    case SWIM_CMD_READ_ONE:
      return SWIM_CHAN_DATA_BITS;

    case SWIM_CMD_READ_BATT:
      return SWIM_BATT_DATA_BITS;

    case SWIM_CMD_READ_FPGA_TEMP:
      return SWIM_TEMP_DATA_BITS;

    case SWIM_CMD_READ_UPTIME:
      return SWIM_UPTIME_DATA_BITS;

    case SWIM_CMD_WAKEUP:
      return SWIM_ACK_BITS;

    default:
      return SWIM_ACK_BITS;
  }
}


/***************************************************************************
 *
 * SWIM Protocol - Methods
 *
 ***************************************************************************/

/**
 *
 * Actually runs the command...
 * SWIMProtocol->SendCmd(SWIMProtocol*, command in uint8_t)
 * --> Returns 0 if successful ack received, else -1
 *
 */
int sendcmd_swim_protocol(SWIMProtocol* s_prot, uint8_t cmd, uint32_t ch_addr)
{
  uint32_t cmd_packet_formatted = \
    ((cmd&SWIM_CMD_MASK)<<SWIM_CHAN_ADDR_BITS) | (ch_addr & SWIM_CMD_CHADDR_MASK);

  /* Sending a command packet is simple as sending a 8 bit packet */
  if (s_prot->pin_mode != OUTPUT) {
    s_prot->Trans->Init(s_prot->Trans);
  }
  s_prot->Trans->SendPacket(s_prot->Trans, SWIM_CMD_DATA_BITS, cmd_packet_formatted);

  return SWIM_SUCCESS;
}

/**
 *
 * Actually sends the data... according to the received command
 * SWIMProtocol->SendData(SWIMProtocol*)
 * --> Returns 0 if successful ack received, else -1
 *
 */
int senddata_swim_protocol(SWIMProtocol* s_prot)
{
  uint32_t tmp_fifo_data;
  uint8_t  data_bits;
  uint64_t packet;
  uint64_t addr, adc_data;

  if (!s_prot->spFIFO->n_nodes) {
    /* No data stored... */
    return SWIM_FAILURE;
  }

  data_bits = cmd_to_data_bit(s_prot->cmd_cache);

  if (s_prot->pin_mode != OUTPUT) {
    s_prot->Trans->Init(s_prot->Trans);
    s_prot->pin_mode = OUTPUT;
  }

  switch (s_prot->cmd_cache) {

    case SWIM_CMD_SLEEP:

      packet = (uint64_t)SWIM_ACK;
      s_prot->Trans->SendPacket(s_prot->Trans, data_bits, packet);

      return SWIM_SUCCESS;
    
    case SWIM_CMD_READ_ALL:

      /* Clearing up all the data in FIFO */
      while (s_prot->spFIFO->n_nodes > 0) {
        tmp_fifo_data = (s_prot->spFIFO->Pop(s_prot->spFIFO) & FIFO_DATA_MASK);
        addr          = ((tmp_fifo_data&FIFO_ADC_ADDR_MASK)>>FIFO_ADC_ADDR_SHIFT);
        adc_data      = (tmp_fifo_data&FIFO_ADC_DATA_MASK);
        packet        = (addr | adc_data);

        s_prot->Trans->SendPacket(s_prot->Trans, data_bits, packet);  
      }

      return SWIM_SUCCESS;

    case SWIM_CMD_READ_ONE:

      /* Popping a data from FIFO may cause some trouble?? */
      tmp_fifo_data = (s_prot->spFIFO->Pop(s_prot->spFIFO) & FIFO_DATA_MASK);
      addr          = ((tmp_fifo_data&FIFO_ADC_ADDR_MASK)>>FIFO_ADC_ADDR_SHIFT);
      adc_data      = (tmp_fifo_data&FIFO_ADC_DATA_MASK);
      packet        = (addr | adc_data);

      s_prot->Trans->SendPacket(s_prot->Trans, data_bits, packet);

      return SWIM_SUCCESS;

    case SWIM_CMD_READ_BATT:

      /* Be sure to update the battery level manually... */
      packet = (uint64_t)s_prot->battery_level;
      s_prot->Trans->SendPacket(s_prot->Trans, data_bits, packet);

      return SWIM_SUCCESS;

    case SWIM_CMD_READ_FPGA_TEMP:

      /* Not available on Arduino - Doing nothing at the moment... */
      return SWIM_SUCCESS;

    case SWIM_CMD_READ_UPTIME:

      /* Manually update the uptime first */
      packet = (uint64_t)s_prot->uptime;
      s_prot->Trans->SendPacket(s_prot->Trans, data_bits, packet);

      return SWIM_SUCCESS;

    case SWIM_CMD_WAKEUP:

      packet = (uint64_t)SWIM_ACK;
      s_prot->Trans->SendPacket(s_prot->Trans, data_bits, packet);

      return SWIM_SUCCESS; 

    default:
      /* As a default option, just send ACK signal back */
      packet = (uint64_t)SWIM_ACK;
      data_bits = SWIM_ACK_BITS;
      s_prot->Trans->SendPacket(s_prot->Trans, data_bits, packet);

      return SWIM_SUCCESS;
  }

  return SWIM_FAILURE;
}

/**
 *
 * Parses the command to react
 * SWIMProtocol->ReadCmd(SWIMProtocol*)
 * --> Returns 0 if successful
 * 
 */
int readcmd_swim_protocol(SWIMProtocol* s_prot)
{
  uint64_t packet;
  int status;
  bool parity_check_result;

  if (s_prot->pin_mode != INPUT) {
    s_prot->Recv->Init(s_prot->Recv);
  }
 
  status = s_prot->Recv->RecvPacket(s_prot->Recv, &packet, SWIM_CMD_DATA_BITS+SWIM_PARITY_BITS);
  if (!status) {

    if (status == SWIM_SUCCESS) {
      parity_check_result = parity_check(packet, SWIM_CHAN_DATA_BITS, SWIM_PARITY_BITS);
    }

    if (parity_check_result) {
      s_prot->cmd_cache = (uint8_t)((packet&(SWIM_CMD_MASK<<1))>>1);
      return SWIM_SUCCESS;
    }
    else return SWIM_FAILURE;
  }
  else {
    return SWIM_FAILURE;
  }
}

/**
 *
 * Read all data from the FIFO --> Reads data from all 30 data channels
 * SWIMProtocol->ReadAll(SWIMProtocol*)
 * --> Saves all the 30 data into Internal FIFO
 *
 */
int readall_swim_protocol(SWIMProtocol* s_prot)
{
  int status = 0;
  bool parity_check_result;
  uint64_t packet;
  uint32_t addr_shifted;
  uint32_t adc_data;
  uint32_t fifo_data_tmp;

  if (s_prot->pin_mode != INPUT) {
    s_prot->Recv->Init(s_prot->Recv);
  }

  while (status != ERROR_IDLE_TIMEOUT) { 

    status = s_prot->Recv->RecvPacket(
      s_prot->Recv, &packet, SWIM_CHAN_DATA_BITS+SWIM_PARITY_BITS);
    
    if (status == SWIM_SUCCESS) {
      parity_check_result = parity_check(packet, SWIM_CHAN_DATA_BITS, SWIM_PARITY_BITS);
    }
    else {
      /* Ignoring failed parity check signal */
      continue;
    }

    if (parity_check_result) {
      addr_shifted = \
        (uint32_t)((packet&SWIM_CHAN_DATA_RECV_MASK)<<\
          (SWIM_FIFO_ADC_ADDR_GAP_BITS-SWIM_PARITY_BITS));
      adc_data = \
        (uint32_t)((packet&SWIM_ADC_DATA_RECV_MASK)>>SWIM_PARITY_BITS);

      fifo_data_tmp = (addr_shifted|adc_data);

      s_prot->spFIFO->Push(s_prot->spFIFO, fifo_data_tmp);      
    }
    else continue;

  } /* while (status != ERROR_IDLE_TIMEOUT) */

  return SWIM_SUCCESS;
}

/**
 *
 * Read one data from the FIFO
 * SWIMProtocol->ReadOne(SWIMProtocol*)
 * --> Returns the actual data instead of populating FIFO
 *
 */
uint16_t readone_swim_protocol(SWIMProtocol* s_prot)
{
  int status;
  bool parity_check_result;
  uint64_t packet;
  uint32_t addr_shifted;
  uint32_t adc_data;
  uint32_t fifo_data_tmp;

  if (s_prot->pin_mode != INPUT) {
    s_prot->Recv->Init(s_prot->Recv);
  }

  status = s_prot->Recv->RecvPacket(
    s_prot->Recv, &packet, SWIM_CHAN_DATA_BITS+SWIM_PARITY_BITS);
  
  if (status == SWIM_SUCCESS) {
    parity_check_result = parity_check(packet, SWIM_CHAN_DATA_BITS, SWIM_PARITY_BITS);
  }
  else {
    /* Ignoring failed parity check signal */
    return SWIM_FAILURE;
  }

  if (parity_check_result) {
    addr_shifted = \
      (uint32_t)((packet&SWIM_CHAN_DATA_RECV_MASK)<<\
        (SWIM_FIFO_ADC_ADDR_GAP_BITS-SWIM_PARITY_BITS));
    adc_data = \
      (uint32_t)((packet&SWIM_ADC_DATA_RECV_MASK)>>SWIM_PARITY_BITS);

    fifo_data_tmp = (addr_shifted|adc_data);

    s_prot->spFIFO->Push(s_prot->spFIFO, fifo_data_tmp);
    
    return SWIM_SUCCESS;
  }
  else {
    return SWIM_FAILURE;
  }

  return SWIM_SUCCESS;
}

/**
 *
 * Sends 'Wake Up' signal to the submerged unit
 * SWIMProtocol->SendWakeUp(SWIMProtocol*)
 * --> Returns 0 if successful ack, else -1
 *
 */
int send_wakeup_swim_protocol(SWIMProtocol* s_prot)
{
  s_prot->SendCmd(s_prot, SWIM_CMD_WAKEUP, 0);

  return 0;
}

/**
 *
 * Sends 'Sleep' signal to the submerged unit
 * SWIMProtocol->SendSleep(SWIMProtocol*)
 * --> Returns 0 if successful ack, else -1
 *
 */
int send_sleep_swim_protocol(SWIMProtocol* s_prot)
{
  s_prot->SendCmd(s_prot, SWIM_CMD_SLEEP, 0);

  return 0;
}

/**
 *
 * Reads 'Uptime' data from the submerged unit
 * SWIMProtocol->ReadUptime(SWIMProtocol*)
 * --> Returns uptime as uint32_t
 *
 */
uint32_t read_uptime_swim_protocol(SWIMProtocol* s_prot)
{
  /* We need RTC module for Arduino to use this function. */
  /* Not yet implemented in Arduino test */

  return 0;
}

/**
 *
 * Reads 'FPGA temperature: XADC Channel 1' data from the submerged unit
 * SWIMProtocol->ReadTemp(SWIMProtocol*)
 * --> Returns the temperature as uint32_t
 *
 */
uint32_t read_temp_swim_protocol(SWIMProtocol* s_prot)
{
  /* Not implemented in Arduino */
  /* TODO: Write a proper code for Vitis FPGA */

  return 0;
}





/****************************************************************************
 *
 * Constructors and Destructors
 *
 ****************************************************************************/

/**
 * SWIM Protocol - Default Constructor
 */
SWIMProtocol* SWIMProtocol_create(void)
{
  SWIMProtocol* s_prot     = (SWIMProtocol*)malloc(sizeof(SWIMProtocol));
  s_prot->Recv             = IRRecv_create(DEF_IR_PIN);
  s_prot->Trans            = IRTrans_create(DEF_IR_PIN);
  s_prot->spFIFO           = FIFO_create(SWIM_FIFO_DEPTH);

  s_prot->cmd_cache        = 0;
  
  s_prot->pin_mode         = 0;
  s_prot->Trans->Init(s_prot->Trans);

  /* Matching function pointers for methods */
  s_prot->SendCmd     = &(sendcmd_swim_protocol);
  s_prot->SendData    = &(senddata_swim_protocol);

  s_prot->ReadAll     = &(readall_swim_protocol);
  s_prot->ReadOne     = &(readone_swim_protocol);
  s_prot->SendWakeUp  = &(send_wakeup_swim_protocol);
  s_prot->SendSleep   = &(send_sleep_swim_protocol);
  s_prot->ReadUptime  = &(read_uptime_swim_protocol);
  s_prot->ReadTemp    = &(read_temp_swim_protocol);

  return s_prot;
}

/**
 * SWIM Protocol - Constructor with parameters
 */
SWIMProtocol* SWIMProtocol_create_with_params(
                uint8_t ir_pin, uint32_t mod_freq, uint32_t fifo_depth)
{
  SWIMProtocol* s_prot     = (SWIMProtocol*)malloc(sizeof(SWIMProtocol));
  s_prot->Recv             = IRRecv_create_with_freq(ir_pin, mod_freq);
  s_prot->Trans            = IRTrans_create_with_freq(ir_pin, mod_freq);
  s_prot->spFIFO           = FIFO_create(fifo_depth);

  s_prot->cmd_cache        = 0;

  s_prot->pin_mode         = 0;
  s_prot->Trans->Init(s_prot->Trans);

  /* Matching function pointers for methods */
  s_prot->SendCmd     = &(sendcmd_swim_protocol);
  s_prot->SendData    = &(senddata_swim_protocol);

  s_prot->ReadAll     = &(readall_swim_protocol);
  s_prot->ReadOne     = &(readone_swim_protocol);
  s_prot->SendWakeUp  = &(send_wakeup_swim_protocol);
  s_prot->SendSleep   = &(send_sleep_swim_protocol);
  s_prot->ReadUptime  = &(read_uptime_swim_protocol);
  s_prot->ReadTemp    = &(read_temp_swim_protocol);

  return s_prot;
}

/**
 * SWIM Protocol - Destructor
 */
void SWIMProtocol_destroy(SWIMProtocol* s_prot)
{
  if (s_prot) {

    if (s_prot->Recv) {
      IRRecv_destroy(s_prot->Recv);
    }

    if (s_prot->Trans) {
      IRTrans_destroy(s_prot->Trans);
    }

    if (s_prot->spFIFO) {
      FIFO_destroy(s_prot->spFIFO);
    }

    free(s_prot);
  }
}