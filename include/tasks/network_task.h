#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

struct NetworkTaskParams {
    QueueHandle_t printQueue;
    QueueHandle_t outgoingMqttQueue;
};

[[noreturn]] [[noreturn]] void networkTask(void* pvParameters);
