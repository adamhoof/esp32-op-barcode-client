#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

struct BarcodeTaskParams {
    QueueHandle_t printQueue;
    QueueHandle_t outgoingMqttQueue;
};

[[noreturn]] void barcodeTask(void* pvParameters);
