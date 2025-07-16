//
// Created by gabri on 3/23/2025.
//

#include "coms.h"
#include "Shipping.h"
#include <hal/uart_types.h>
#include "Global/Errors.h"
#include "Global/GLOBAL.h"

// cppcheck-suppress unusedFunction
int PackfToPI(QueueHandle_t MsgQueue, const uint8_t VPID,const char buff[], size_t buff_size){
  if (buff_size > COMS_SIZE){return COMS_SIZE - buff_size;} //Check against the size of the message buffer with size of COMS_SIZE
  MSG block;
  block.VPID=VPID;
    strncpy(block.data,buff,COMS_SIZE);

  if(pdPASS != xQueueSend(MsgQueue,(const void*)&block,portMAX_DELAY)){ //Add message to Que AND check if it was able to fit on the Que
    return QueFull;
  }
  return Success;
}

// cppcheck-suppress unusedFunction
int PrintfToPI(QueueHandle_t MsgQueue, const uint8_t VPID,const char *format, ...){
    if (!format || MsgQueue == NULL ) {
        return URCHIN_ERROR_BadArgs;
    }


    // Whitelist check: limit what format specifiers are allowed
    if (strstr(format, "%n")) {
        // Dangerous specifier! Refuse to process.
        return URCHIN_ERROR_Malicious;
    }


  char buff[COMS_SIZE];
  int ReturnValue;
  va_list args;

  va_start(args, format);

  vsnprintf(buff,COMS_SIZE,format, args);
  ReturnValue = PackfToPI(MsgQueue,VPID,buff,(strlen(buff)+1));

  va_end(args);
  return ReturnValue;
}



