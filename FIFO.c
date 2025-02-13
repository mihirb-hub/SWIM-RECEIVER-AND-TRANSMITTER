/************************************************************

  A FIFO for SWIM-SnH

  A simpleton raw ADC data storage for SWIM Sample & Hold. 
  Adopting a preliminary first-in-first-out, FIFO, data 
  structure to store and manage the data in the SnH control
  FPGA. 

  Implementation file

 ************************************************************/
#include "FIFO.h"

#define DEF_FIFO_DEPTH        60

/*************************************************************

  FIFO Node stuffs

**************************************************************/

FIFONode* FIFONode_create()
{
  FIFONode* fifoNode = (FIFONode*)malloc(sizeof(FIFONode));

  fifoNode->data = (fifo_data_t)0;
  fifoNode->next = NULL;

  return fifoNode;
}

FIFONode* FIFONode_create_with_data(fifo_data_t data_value)
{
  FIFONode* fifoNode = FIFONode_create();
  fifoNode->data = data_value;
  return fifoNode;
}

FIFONode* FIFONode_create_with_data_next(
  fifo_data_t data_value, FIFONode* nextNode)
{
  FIFONode* fifoNode = FIFONode_create_with_data(data_value);
  fifoNode->next = nextNode;
  return fifoNode;
}

void FIFONode_destroy(FIFONode* fifoNode)
{
  if (fifoNode) free(fifoNode);
}

/*************************************************************

  FIFO stuffs

**************************************************************/

void push_fifo(FIFO* fifo, fifo_data_t data)
{

  FIFONode* tmp;

  /* Dropping the last one to insert a new one if the fifo is full */
  if (fifo->n_nodes >= fifo->depth) {
    FIFONode* del_tmp = fifo->last_node;
    FIFONode* del_tmp_prev = fifo->first_node;

    while (del_tmp_prev->next != fifo->last_node) del_tmp_prev = del_tmp_prev->next;

    fifo->last_node = del_tmp_prev;
    fifo->last_node->next = NULL;

    FIFONode_destroy(del_tmp);
    fifo->n_nodes--;
  }

  /* If the fifo is empty */
  if (!fifo->first_node) {
    tmp = FIFONode_create_with_data(data);
    tmp->next = NULL;
    fifo->first_node = tmp;
    fifo->last_node = tmp;
    fifo->n_nodes++;
  }
  else {
    tmp = FIFONode_create_with_data_next(data, fifo->first_node);
    fifo->first_node = tmp;
    fifo->n_nodes++;
  }
  return;
}

fifo_data_t pop_fifo(FIFO* fifo)
{
  if (!fifo->first_node) {
    return 0;
  }

  fifo_data_t ret_data;

  FIFONode* tmp = fifo->last_node;
  ret_data = tmp->data;

  /* Single node case */
  if (fifo->first_node == fifo->last_node) {
    fifo->first_node = NULL;
    fifo->last_node = NULL;
    FIFONode_destroy(tmp);
    fifo->n_nodes = 0;
    return ret_data;
  }

  FIFONode* tmp_prev = fifo->first_node;
  while(tmp_prev->next != fifo->last_node) tmp_prev = tmp_prev->next;
  fifo->last_node = tmp_prev;
  fifo->last_node->next = NULL;
  
  fifo->n_nodes--;
  FIFONode_destroy(tmp);

  return ret_data;
}

FIFO* FIFO_create_skel(void)
{
  FIFO* fifo = (FIFO*)malloc(sizeof(FIFO));

  fifo->n_nodes = 0;
  fifo->depth = DEF_FIFO_DEPTH;
  fifo->first_node = NULL;
  fifo->last_node = NULL;

  fifo->Push =   &(push_fifo);
  fifo->Pop  =   &(pop_fifo);

  return fifo;
}

FIFO* FIFO_create(uint32_t fifo_depth)
{
  FIFO* fifo = FIFO_create_skel();
  fifo->depth = fifo_depth;

  return fifo;
}

void FIFO_destroy(FIFO* fifo)
{
  if (fifo) {
    FIFONode* tmp = fifo->first_node;
    FIFONode* tmp_del;
    while (tmp->next) {
      tmp_del = tmp;
      tmp = tmp->next;
      FIFONode_destroy(tmp_del);
    }
  }
}

