/************************************************************

  A FIFO for SWIM-SnH

  A simpleton raw ADC data storage for SWIM Sample & Hold. 
  Adopting a preliminary first-in-first-out, FIFO, data 
  structure to store and manage the data in the SnH control
  FPGA. 

  Header file

 ************************************************************/
#ifndef __SWIM_SNH_FIFO_H__
#define __SWIM_SNH_FIFO_H__

/**
 Some standard includes
 */
#include <stdlib.h>
#include <stdint.h>
/*
 * Some boolean stuffs...
 */
#ifndef __cplusplus
#include "cbool.h"
#endif

/* Defning the data type... */
typedef uint32_t fifo_data_t;

/**
 The FIFO Node - the main data storage nodes.
 */
typedef struct __fifo_node__ {

  fifo_data_t data;

  struct __fifo_node__* next;

} FIFONode;

#ifdef __cplusplus
extern "C" {
#endif

FIFONode* FIFONode_create();
FIFONode* FIFONode_create_with_data(fifo_data_t data_value);
FIFONode* FIFONode_create_with_data_next(
  fifo_data_t data_value, FIFONode* nextNode);

void FIFONode_destroy(FIFONode* fifoNode);

#ifdef __cplusplus
} /* Matching } for the extern C */
#endif

/**
 The FIFO - the FIFO class itself.
 */

typedef struct __snh_fifo__ {

  uint32_t n_nodes;
  uint32_t depth;

  FIFONode* first_node;
  FIFONode* last_node;

  void (*Push)(struct __snh_fifo__*, fifo_data_t data);
  fifo_data_t (*Pop)(struct __snh_fifo__*);

} FIFO;


#ifdef __cplusplus
extern "C" {
#endif

void push_fifo(FIFO* fifo, fifo_data_t data);
fifo_data_t pop_fifo(FIFO* fifo); 

FIFO* FIFO_create_skel(void);
FIFO* FIFO_create(uint32_t fifo_depth);

void FIFO_destroy(FIFO* fifo);

#ifdef __cplusplus
} /* Matching } for the extern C */
#endif

#endif /* Include Guard */