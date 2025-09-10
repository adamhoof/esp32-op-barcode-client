#include "tasks/network_task.h"
#include <Arduino.h>
#include <espMqttClient.h>
#include <ArduinoJson.h>
#include <esp_log.h>
#include "config.h"
#include "request.h"
#include "response.h"
#include <freertos/FreeRTOS.h>
#include "print_message.h"

static const char* TAG = "NETWORK_TASK";

espMqttClientTypes::OnMessageCallback onMessageCallback(QueueHandle_t printQueue)
{
    return [printQueue](const espMqttClientTypes::MessageProperties& properties, const char* topic,
                           const uint8_t* payload, size_t len, size_t index, size_t total) {
        ESP_LOGI(TAG, "Received raw MQTT payload: %.*s", len, (char*)payload);

        MqttProductDataResponse response{};
        StaticJsonDocument<350> doc;
        const DeserializationError error = deserializeJson(doc, payload, len);
        if (error) {
            ESP_LOGW(TAG, "Failed to deserialize JSON from MQTT message: %s", error.c_str());
            return;
        }
        strlcpy(response.name, doc["name"] | "", sizeof(response.name));
        strlcpy(response.unitOfMeasure, doc["unitOfMeasure"] | "", sizeof(response.unitOfMeasure));
        response.price = doc["price"].as<float>();
        response.stock = doc["stock"].as<int>();
        response.unitOfMeasureKoef = doc["unitOfMeasureCoef"].as<float>();

        PrintMessage message = {};
        if (strlen(response.name) == 0 || response.price == 0) {
            message.type = PRINT_ERROR_MESSAGE;
            strlcpy(message.data.errorMessage, "Zkuste znovu\nnebo\nzavolejte obsluhu...", sizeof(message.data.errorMessage));
        } else {
            message.type = PRINT_PRODUCT_DATA;
            message.data.productData = response;
        }

        if (xQueueSend(printQueue, &message, pdMS_TO_TICKS(100)) != pdPASS) {
            ESP_LOGW(TAG, "Failed to send incoming display data to queue.");
        }
    };
}

espMqttClientTypes::OnConnectCallback onConnectCallback(espMqttClient& client)
{
    return [&client](bool sessionPresent) {
        ESP_LOGD(TAG, "Connected to MQTT broker. Session present: %s", sessionPresent ? "true" : "false");
        client.subscribe(MQTT_PRODUCT_DATA_RESPONSE_TOPIC.c_str(), 1);
    };
}

void networkTask(void* pvParameters)
{
    static espMqttClient mqttClient;
    const auto* params = static_cast<NetworkTaskParams*>(pvParameters);
    constexpr TickType_t wifiRestartTimeoutTicks = pdMS_TO_TICKS(30000);

    mqttClient.onMessage(onMessageCallback(params->printQueue));
    mqttClient.onConnect(onConnectCallback(mqttClient));
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setClientId(MQTT_CLIENT_NAME);
    mqttClient.setCleanSession(true);
    mqttClient.setKeepAlive(15);

    ESP_LOGD(TAG, "Initializing WiFi connection...");
    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (!WiFi.isConnected()) {
        delay(300);
        ESP_LOGD(TAG, ".");
    }

    while (true) {
        // always loop before anything to advance internal client state
        mqttClient.loop();

        if (WiFi.isConnected()) {
            if (mqttClient.connected()) {
                MqttProductDataRequest receivedRequest{};
                if (xQueueReceive(params->outgoingMqttQueue, &receivedRequest, portMAX_DELAY) == pdPASS) {
                    ESP_LOGD(TAG, "Publishing MQTT message to topic: %s", receivedRequest.topic);
                    if (mqttClient.publish(receivedRequest.topic, 1, false, receivedRequest.payload)) {
                        ESP_LOGD(TAG, "Published successfully", receivedRequest.topic);
                    } // TODO send data to display
                }
            }

            else {
                ESP_LOGD(TAG, "Attempting to connect to MQTT broker...");
                mqttClient.connect();
                // prevent starving the task
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }


        else {
            // WiFi is disconnected, auto-retry will try to reconnect
            ESP_LOGW(TAG, "WiFi disconnected. Attempting to reconnect...");
            const TickType_t startTime = xTaskGetTickCount();
            while (!WiFi.isConnected() && (xTaskGetTickCount() - startTime < wifiRestartTimeoutTicks)) {
                vTaskDelay(pdMS_TO_TICKS(300));
            }
            if (!WiFi.isConnected()) {
                ESP_LOGW(TAG, "WiFi reconnection failed after timeout. Restarting ESP...");
                ESP.restart();
            }
        }
    }
}
