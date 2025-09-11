#include <Arduino.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "print_message.h"
#include "request.h"
#include "tasks/barcode_task.h"
#include "tasks/network_task.h"
#include "tasks/print_task.h"
#include "tasks/ota_task.h"

static const char* TAG = "MAIN";

void setup() {
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGD(TAG, "Starting system...");

    static QueueHandle_t outgoingMqttQueue = xQueueCreate(5, sizeof(MqttProductDataRequest));
    static QueueHandle_t printQueue = xQueueCreate(5, sizeof(PrintMessage));

    if (outgoingMqttQueue == nullptr || printQueue == nullptr) {
        ESP_LOGW(TAG, "Error creating queues, restarting...");
        ESP.restart();
    }

    static TaskHandle_t otaTaskHandle = nullptr;
    xTaskCreate(ota_task, "ota_task", 4096, nullptr, 1, &otaTaskHandle);

    static NetworkTaskParams networkTaskParams = { printQueue, outgoingMqttQueue, otaTaskHandle };
    xTaskCreate(networkTask, "network_task", 8192, &networkTaskParams, 1, nullptr);

    static BarcodeTaskParams barcodeTaskParams = {  printQueue, outgoingMqttQueue };
    xTaskCreate(barcodeTask, "barcode_task", 4096, &barcodeTaskParams, 1, nullptr);

    static PrintTaskParams printTaskParams = { printQueue };
    xTaskCreate(printTask, "print_task", 4096, &printTaskParams, 1, nullptr);

    ESP_LOGD(TAG, "All tasks created. Deleting setup task.");
    vTaskDelete(nullptr);
}

void loop() {}
