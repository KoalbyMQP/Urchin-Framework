//
// Created by gabri on 5/23/2025.
//

#ifndef ERRORS_H
#define ERRORS_H


typedef enum {
    URCHIN_OK = 0,
    URCHIN_ERROR_NULL_POINTER=-1,
    URCHIN_ERROR_NoFreeTicket=1,
    URCHIN_ERROR_BUFFER_OVERFLOW=4,
    URCHIN_ERROR_QUEUE_FULL=2,
    URCHIN_ERROR_CommandNotFound=3,
    URCHIN_ERROR_FailedToAllocate=5,
    URCHIN_ERROR_Malicious=6,
    URCHIN_ERROR_BadArgs=7
} urchin_error_t;


#endif //ERRORS_H
