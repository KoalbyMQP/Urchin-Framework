//
// Created by gabri on 5/23/2025.
//

#ifndef TICKET_H
#define TICKET_H
#define StripSize 8

#ifdef __cplusplus
extern "C" {
#endif


#define Width sizeof(long int)*8

typedef struct{
  //512 bits
  long int strip[StripSize];
}TapeRoll;

int FindFree(const TapeRoll *Roll);
void checkOut(TapeRoll *Roll, int Ticket);
void ReturnTicket(TapeRoll *Roll, int Ticket);
void RollINIT(TapeRoll *Roll);

extern TapeRoll TicketTape;

#ifdef __cplusplus
}
#endif

#endif //TICKET_H
