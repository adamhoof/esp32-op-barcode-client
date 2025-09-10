#include "tasks/barcode_task.h"
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <esp_log.h>
#include "request.h"
#include "config.h"
#include "print_message.h"

static const char* TAG = "BARCODE_TASK";

bool isNumeric(const char* str)
{
    for (size_t i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) {
            return false;
        }
    }
    return true;
}

void barcodeTask(void* pvParameters)
{
    const auto* params = static_cast<const BarcodeTaskParams*>(pvParameters);

    SoftwareSerial barcodeReaderSerial(BARCODE_RX_PIN, BARCODE_TX_PIN);
    barcodeReaderSerial.begin(BARCODE_BAUD_RATE);

    for (;;) {
        char barcodeBuffer[BARCODE_BUFFER_SIZE];
        if (barcodeReaderSerial.available() > 0) {
            const size_t bytesRead = barcodeReaderSerial.readBytesUntil(BARCODE_DELIMITER, barcodeBuffer,
                                                                        sizeof(barcodeBuffer) - 1);
            barcodeBuffer[bytesRead] = '\0';

            if (bytesRead > 0) {
                ESP_LOGI(TAG, "Barcode: %s", barcodeBuffer);

                if (!isNumeric(barcodeBuffer)) {
                    ESP_LOGW(TAG, "Invalid barcode format: %s", barcodeBuffer);
                    PrintMessage errorMessage = {};
                    errorMessage.type = PRINT_ERROR_MESSAGE;
                    snprintf(errorMessage.data.errorMessage, MAX_ERROR_MESSAGE_LENGTH,
                             "Blbe sem to\nnaskenoval.\nZkuste znovu...");
                    if (xQueueSend(params->printQueue, &errorMessage, pdMS_TO_TICKS(100)) != pdPASS) {
                        ESP_LOGW(TAG, "Failed to send error message to display queue.");
                    }
                }
                else {
                    MqttProductDataRequest request = {};
                    StaticJsonDocument<200> doc;

                    strncpy(request.topic, MQTT_PRODUCT_DATA_REQUEST_TOPIC, sizeof(request.topic));

                    doc["clientTopic"] = MQTT_PRODUCT_DATA_RESPONSE_TOPIC.c_str();
                    doc["barcode"] = barcodeBuffer;
                    doc["includeDiacritics"] = false;
                    serializeJson(doc, request.payload, sizeof(request.payload));

                    if (xQueueSend(params->outgoingMqttQueue, &request, pdMS_TO_TICKS(100)) != pdPASS) {
                        ESP_LOGW(TAG, "Failed to send barcode request to MQTT queue.");
                    }
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
