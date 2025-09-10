#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

struct PrintTaskParams {
    QueueHandle_t printQueue;
};

[[noreturn]] void printTask(void* pvParameters);
