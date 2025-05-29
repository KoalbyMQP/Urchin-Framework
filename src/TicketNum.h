//
// Created by gabri on 5/24/2025.
//

#ifndef TICKETN_H
#define TICKETN_H


#define StripSize 8

#ifdef __cplusplus
extern "C" {
#endif


#define Width sizeof(long int)*8

    typedef struct{
        //512 bits
        long int strip[StripSize];
    }TapeRoll;

    int FindFree(TapeRoll *Roll);
    void checkOut(TapeRoll *Roll, int Ticket);
    void ReturnTicket(TapeRoll *Roll, int Ticket);
    void RollINIT(TapeRoll *Roll);

    extern TapeRoll TicketTape;

#ifdef __cplusplus
}
#endif

#endif //TICKETN_H
