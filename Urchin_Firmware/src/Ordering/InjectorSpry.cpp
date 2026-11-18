//
// Created by gabri on 12/8/25.
//

#include "Ordering/InjectorSpry.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "Coms.h"
#include "Ordering/Ordering.h"
#include "Herkulex/Herkulex.h"
#include "Ticketing/Ticket.h"

#include "Ticketing/Packet.h"

SemaphoreHandle_t UART1Mutex;

void InjectorSpryInit() {
    UART1Mutex = xSemaphoreCreateMutex();
}

[[noreturn]] void Spray(void *pvParameters) {
    while (true) {
        Ticket ticket;

        if (xQueueReceive(AsynchronousQueue, &ticket, portMAX_DELAY)) {

            // ----------Take mutex----------------------------------------------------------------------------------------------------
            if (xSemaphoreTake(UART1Mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
                int VPID = ticket.VPID;
                int TicketNum = ticket.TicketNum;
                int length = PacketLength(ticket.Packets);
                (void) PrintfToPI(DebugQueue,VPID,"Spray->Running, Ticket:%d, Num of Packets:%d",TicketNum,length);

                for (int i = 0; i < length; ++i) {
                    Packet *packet = PacketGet(ticket.Packets,i);
                    (void) PrintfToPI(DebugQueue,VPID,"Spray->Sending Packet VV VV");
                    PrintPackets(DebugQueue,VPID,packet);
					(void) PrintfToPI(DebugQueue, 0, "About to print packet contents");
					for (int i = 0; i < packet->ContentsSize; i++) {
						(void) PrintfToPI(DebugQueue, 0, "%u", packet->contents[i]);
					}


                    Herkulex.sendData(reinterpret_cast<byte *>(packet->contents),packet->ContentsSize);
                }
                xSemaphoreGive(UART1Mutex);
                // ----------Give mutex--------------------------------------------------------------------------------------------------
			}else {
				(void) PrintfToPI(DebugQueue,0,"Spray FAILED to clam Mutex UART1");
           	}
        }
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}


[[noreturn]] void Injector(void *pvParameters) {
    UART1Mutex = xSemaphoreCreateMutex();

    while (true) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
