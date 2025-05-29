//
// Created by gabri on 5/23/2025.
//

#include "TicketNum.h"

TapeRoll TicketTape;

int FindFree(TapeRoll *Roll){
for(int i=0; i<StripSize; i++){
  for(int j=0; j<Width ; j++){
    if (!(Roll->strip[i] & (1<<i))){
      return i*Width+j;
    }
  }
}
return -1;
}

void checkOut(TapeRoll *Roll, const int Ticket){
  const int Mod = Ticket%Width;
  const int Chunk=(Ticket-Mod)/Width;
  (Roll->strip[Chunk]) |= (1<<Mod);
  }

void ReturnTicket(TapeRoll *Roll, int Ticket){
int Mod = Ticket%Width;
int Chunk=(Ticket-Mod)/Width;
 (Roll->strip[Chunk]) &= ~(1<<Mod);

}

void RollINIT(TapeRoll *Roll){
  for(int i=0; i<StripSize; i++){
    Roll->strip[i]=0;
  }
}

