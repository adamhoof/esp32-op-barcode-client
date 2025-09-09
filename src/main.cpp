#include <Arduino.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "response.h"
#include "request.h"
#include "tasks/barcode_task.h"
#include "tasks/network_task.h"
#include "tasks/print_task.h"

static const char* TAG = "MAIN";

void setup() {
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGD(TAG, "Starting system...");

    static QueueHandle_t outgoingMqttQueue = xQueueCreate(5, sizeof(MqttProductDataRequest));
    static QueueHandle_t incomingDisplayDataQueue = xQueueCreate(5, sizeof(MqttProductDataResponse));

    if (outgoingMqttQueue == nullptr || incomingDisplayDataQueue == nullptr) {
        ESP_LOGW(TAG, "Error creating queues, restarting...");
        ESP.restart();
    }

    static NetworkTaskParams networkTaskParams = { incomingDisplayDataQueue, outgoingMqttQueue };
    xTaskCreate(networkTask, "network_task", 8192, &networkTaskParams, 1, nullptr);

    static BarcodeTaskParams barcodeTaskParams = { outgoingMqttQueue };
    xTaskCreate(barcodeTask, "barcode_task", 4096, &barcodeTaskParams, 1, nullptr);

    static PrintTaskParams printTaskParams = { incomingDisplayDataQueue };
    xTaskCreate(printTask, "print_task", 4096, &printTaskParams, 1, nullptr);

    ESP_LOGD(TAG, "All tasks created. Deleting setup task.");
    vTaskDelete(nullptr);
}

void loop() {}
