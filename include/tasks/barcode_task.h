#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

struct BarcodeTaskParams {
    QueueHandle_t outgoingQueue;
};

[[noreturn]] void barcodeTask(void* pvParameters);
