#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

struct NetworkTaskParams {
    QueueHandle_t incomingQueue;
    QueueHandle_t outgoingQueue;
};

[[noreturn]] [[noreturn]] void networkTask(void* pvParameters);
