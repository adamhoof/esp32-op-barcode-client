#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

struct PrintTaskParams {
    QueueHandle_t incomingQueue;
};

[[noreturn]] void printTask(void* pvParameters);
