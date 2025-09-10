//
// Created by gabri on 5/23/2025.
//

#include "TicketNum.h"
#include "ESP_PI_Communication/MSGQueue.h"
#include "ESP_PI_Communication/Coms.h"

//rolint: ignore
TapeRoll TicketTape;
//rolint: ignore
SemaphoreHandle_t TicketTapeMutex;

unsigned int FindFree(const TapeRoll *Roll){
    (void)PrintfToPI(DebugQueue,0,"FindFree");
for(int i=0; i<StripSize; i++){
  for(int j=0; j<Width ; j++){
    if (!(Roll->strip[i] & (1U<<j))){
        (void) PrintfToPI(DebugQueue,0,"Found at bit:%d chunk:%d",j+1,i);
      return i*Width+j;

    }
  }
}
return -1;
}

void checkOut(TapeRoll *Roll, unsigned int Ticket) {
    (void) PrintfToPI(DebugQueue,0,"CheckOut");


  unsigned int Mod = Ticket%Width;
  if (Ticket<Width){Mod=Ticket;}
    (void) PrintfToPI(DebugQueue,0,"    Mod:%u",Mod);


  unsigned int Chunk=(Ticket-Mod)/(Width);
     PrintfToPI(DebugQueue,0,"    Chunk:%u",Chunk);


  if (Chunk<StripSize){
    (Roll->strip[Chunk]) |= (1U<<Mod);
  }else{/*printf("checkOut error");*/};
}


void ReturnTicket(TapeRoll *Roll, unsigned int Ticket){
  unsigned int Mod = Ticket%Width;
  if (Ticket<=Width){Mod=Ticket;}

  unsigned int Chunk=(Ticket-Mod)/(Width);

  if (Chunk<StripSize) {
    (Roll->strip[Chunk]) &= ~(1U<<Mod);
  }
}

void RollINIT(TapeRoll *Roll){
  for(int i=0; i<StripSize; i++){
    Roll->strip[i]=0U;
  }
}







