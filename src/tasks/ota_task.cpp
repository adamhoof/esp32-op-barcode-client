#include "tasks/ota_task.h"
#include <Arduino.h>
#include <esp_log.h>
#include <ArduinoOTA.h>

static const char* TAG = "OTA_TASK";

void ota_task(void *pvParameters) {
    for (;;) {
        // Wait for notification to start OTA
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        ESP_LOGI(TAG, "OTA update available, starting...");

        bool ota_running = true;
        int last_progress = -10;

        ArduinoOTA.onStart([]() {
            String type;
            if (ArduinoOTA.getCommand() == U_FLASH) {
                type = "sketch";
            } else { // U_SPIFFS
                type = "filesystem";
            }
            ESP_LOGI(TAG, "Start updating %s", type.c_str());
        }).onEnd([&ota_running]() {
            ESP_LOGI(TAG, "Update complete");
            ota_running = false; // This will likely not be hit as it reboots.
        }).onProgress([&last_progress](unsigned int progress, unsigned int total) {
            int percentage = (progress / (total / 100));
            if (percentage >= last_progress + 10) {
                last_progress = percentage;
                ESP_LOGI(TAG, "Progress: %u%%", percentage);
            }
        }).onError([&ota_running](ota_error_t error) {
            ESP_LOGE(TAG, "Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR) ESP_LOGE(TAG, "Auth Failed");
            else if (error == OTA_BEGIN_ERROR) ESP_LOGE(TAG, "Begin Failed");
            else if (error == OTA_CONNECT_ERROR) ESP_LOGE(TAG, "Connect Failed");
            else if (error == OTA_RECEIVE_ERROR) ESP_LOGE(TAG, "Receive Failed");
            else if (error == OTA_END_ERROR) ESP_LOGE(TAG, "End Failed");
            ota_running = false;
        });

        ArduinoOTA.begin();

        while (ota_running) {
            ArduinoOTA.handle();
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        ESP_LOGI(TAG, "OTA process finished. Task will now wait for next notification.");
    }
}
