//
// Created by gabri on 5/23/2025.
//

#include "TicketNum.h"

#include "coms.h"

TapeRoll TicketTape;

int FindFree(TapeRoll *Roll){
for(int i=0; i<StripSize; i++){
  for(int j=0; j<Width ; j++){
    if (!(Roll->strip[i] & (1U<<j))){
      return i*Width+j;
    }
  }
}
return -1;
}

void checkOut(TapeRoll *Roll, unsigned int Ticket) {
  unsigned int Mod = Ticket%Width;
  unsigned int Chunk=Ticket/Width;
  if (Chunk<StripSize){
    (Roll->strip[Chunk]) |= (1U<<Mod);
  }
}


void ReturnTicket(TapeRoll *Roll, unsigned int Ticket){
  unsigned int  Mod = Ticket%Width;
  unsigned int  Chunk=Ticket/Width;
  if (Chunk<StripSize) {
    (Roll->strip[Chunk]) &= ~(1U<<Mod);
  }
}

void RollINIT(TapeRoll *Roll){
  for(int i=0; i<StripSize; i++){
    Roll->strip[i]=0;
  }
}






