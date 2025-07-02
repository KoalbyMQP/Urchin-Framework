//
// Created by gabri on 3/23/2025.
//

#include "coms.h"
#include "shipping.h"
#include <driver/uart.h>
#include <hal/uart_types.h>


// cppcheck-suppress unusedFunction
int PackfToPI(QueueHandle_t MsgQueue, const uint8_t VPID,const char buff[], size_t buff_size){
  if (buff_size > COMS_SIZE){return COMS_SIZE - buff_size;} //Check against the size of the message buffer with size of COMS_SIZE
  MSG block;
  block.VPID=VPID;
    strcpy(block.data,buff);

  if(pdPASS != xQueueSend(MsgQueue,(const void*)&block,portMAX_DELAY)){ //Add message to Que AND check if it was able to fit on the Que
    return QueFull;
  }
  return Success;
}

// cppcheck-suppress unusedFunction
int PrintfToPI(QueueHandle_t MsgQueue, const uint8_t VPID,const char *format, ...){
  char buff[COMS_SIZE];
  int ReturnValue;
  va_list args;

  va_start(args, format);

  vsnprintf(buff,COMS_SIZE,format, args);
  ReturnValue = PackfToPI(MsgQueue,VPID,buff,(strlen(buff)+1));

  va_end(args);
  return ReturnValue;
}



