//
// Created by Sam Kim on 9/27/25.
//

#ifndef URCHIN_FIRMWARE_TESTCOMS_H
#define URCHIN_FIRMWARE_TESTCOMS_H

void test_PackfToPI_oversize();

//void test_PackfToPI_full();

void test_PackfToPI_Success();

void test_PrintfToPI_BadArgsQueue();

void test_PrintfToPI_BadArgsFormat();

void test_PrintfToPI_Malicious();

#endif //URCHIN_FIRMWARE_TESTCOMS_H
