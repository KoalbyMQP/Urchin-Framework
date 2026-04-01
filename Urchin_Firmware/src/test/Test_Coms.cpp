///
/// Created by Sam Kim 9/27/25
///

extern "C" {
  #include <unity.h>
  #include "freertos/FreeRTOS.h"
  #include "freertos/queue.h"
}
#include "Test_Coms.h"
#include "Global/Errors.h"
#include "Global/GLOBAL.h"
#include <cstdint>
#include <cstring>
#include "ESP_PI_Communication/Coms.h"
#include "ESP_PI_Communication/Shipping.h"

#include <stdio.h>
#include <stdarg.h>





uint8_t test_VPID = 0x99;


void test_PackfToPI_oversize(void){
    char big[COMS_SIZE + 1];
    memset(big, 'X', sizeof(big));//Fill big with X


    QueueHandle_t tester = xQueueCreate(1, sizeof(MSG));
    TEST_ASSERT_NOT_NULL(tester);
    int t = PackfToPI(tester, test_VPID, big, sizeof(big));
    TEST_ASSERT_EQUAL(COMS_SIZE - sizeof(big), t);
    vQueueDelete(tester);

}

void test_PackfToPI_full(void){//test if function properly checks if queue is full
    MSG test_block{};
    test_block.VPID = test_VPID;


    char buff[COMS_SIZE];
    memset(buff, 'A', COMS_SIZE);
    memcpy(test_block.data, buff, COMS_SIZE);

    QueueHandle_t tester = xQueueCreate(1, sizeof(MSG));
    TEST_ASSERT_NOT_NULL(tester);


    //strncpy(test_block.data, buff, COMS_SIZE);
    xQueueSend(tester, &test_block, 0);//fill queue

    int t = PackfToPI(tester, test_VPID, buff, sizeof(buff));
    TEST_ASSERT_EQUAL(QueFull, t);

    vQueueDelete(tester);

}//Talk w/Gabe about this, you can't test for queFull?

void test_PackfToPI_Success(void){
    MSG test_block{};
    char buff[COMS_SIZE];
    memset(buff, 'A', COMS_SIZE);
    memcpy(test_block.data, buff, COMS_SIZE);

    QueueHandle_t tester = xQueueCreate(1, sizeof(MSG));


    test_block.VPID = test_VPID;

    int t = PackfToPI(tester, test_VPID, buff, COMS_SIZE);

    TEST_ASSERT_EQUAL(Success, t);

    vQueueDelete(tester);
}

void test_PrintfToPI_BadArgsQueue(void){

    const char *t_format = "%s";

    int ret = PrintfToPI(NULL, test_VPID, t_format);

    TEST_ASSERT_EQUAL(URCHIN_ERROR_BadArgs, ret);

}

void test_PrintfToPI_BadArgsFormat(void){

    const char *t_format = NULL;


    /*MSG test_block{};
    char buff[COMS_SIZE];
    memset(buff, 'A', COMS_SIZE);
    memcpy(test_block.data, buff, COMS_SIZE);*/

    QueueHandle_t tester = xQueueCreate(1, sizeof(MSG));

    int ret = PrintfToPI(tester, test_VPID, t_format);

    TEST_ASSERT_EQUAL(URCHIN_ERROR_BadArgs, ret);

    vQueueDelete(tester);

}


void test_PrintfToPI_Malicious(void){
    const char *t_format = "%n";
    QueueHandle_t tester = xQueueCreate(1, sizeof(MSG));

    int ret = PrintfToPI(tester, test_VPID, t_format);

    TEST_ASSERT_EQUAL(URCHIN_ERROR_Malicious, ret);

}
