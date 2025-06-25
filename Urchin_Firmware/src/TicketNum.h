

//
// Created by gabri on 5/24/2025.
//

#ifndef TICKETN_H
#define TICKETN_H

#include <stdint.h>
/**
 * @def StripSize
 * @brief sets the strip size to be 8 this will make tape 512 bits long
 */
#define StripSize 8

#ifdef __cplusplus
extern "C" {
#endif


#define Width (sizeof(uint64_t)*8)

    /**
     * @brief a data type ment to hold alot of bit flags
     * @date 2025-05-29
     * @author Gabriel Weaver
     */
    typedef struct{
        //512 bits
       uint64_t strip[StripSize];
    }TapeRoll;

    /**
     *
     * @param Roll The variable that holds the flags
     * @return a flag that is set to 0
     * @date 2025-05-29
     * @author Gabriel Weaver
     */
    unsigned int FindFree(const TapeRoll *Roll);

    /**
     * Will set the desired ticket in the tape to 1
     * @param Roll The variable that holds the flags
     * @param Ticket the desired ticket
     * @date 2025-05-29
     * @author Gabriel Weaver
     */
    void checkOut(TapeRoll *Roll, unsigned int Ticket);

    /**
     * Will set the desired ticket in the tape to 0
     * @param Roll The variable that holds the flags
     * @param Ticket the desired ticket
     * @date 2025-05-29
     * @author Gabriel Weaver
     */
    void ReturnTicket(TapeRoll *Roll, unsigned int Ticket);

    /**
     * Initialises the tape roll by setting everything to 0
     * @param Roll The variable that holds the flags
     * @date 2025-05-29
     * @author Gabriel Weaver
     */
    void RollINIT(TapeRoll *Roll);


    /**
     * @brief Global variable for the task ticket system
     */
    extern TapeRoll TicketTape;





#ifdef __cplusplus
}
#endif

#endif //TICKETN_H
