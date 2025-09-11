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

espMqttClientTypes::OnMessageCallback onMessageCallback(QueueHandle_t printQueue, TaskHandle_t otaTaskHandle)
{
    return [printQueue, otaTaskHandle](const espMqttClientTypes::MessageProperties& properties, const char* topic,
                                        const uint8_t* payload, size_t len, size_t index, size_t total) {
        ESP_LOGI(TAG, "Received message on topic: %s", topic);

        if (strcmp(topic, MQTT_FIRMWARE_UPDATE_TOPIC) == 0) {
            ESP_LOGI(TAG, "Firmware update notification received. Triggering OTA task.");
            xTaskNotifyGive(otaTaskHandle);
            return;
        }

        if (strcmp(topic, MQTT_PRODUCT_DATA_RESPONSE_TOPIC.c_str()) == 0) {
            ESP_LOGD(TAG, "Received raw MQTT payload: %.*s", len, (char*)payload);

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
                strlcpy(message.data.errorMessage, "Zkuste znovu\nnebo\nzavolejte obsluhu...",
                        sizeof(message.data.errorMessage));
            } else {
                message.type = PRINT_PRODUCT_DATA;
                message.data.productData = response;
            }

            if (xQueueSend(printQueue, &message, pdMS_TO_TICKS(100)) != pdPASS) {
                ESP_LOGW(TAG, "Failed to send incoming display data to queue.");
            }
        }
    };
}

espMqttClientTypes::OnConnectCallback onConnectCallback(espMqttClient& client)
{
    return [&client](bool sessionPresent) {
        ESP_LOGD(TAG, "Connected to MQTT broker. Session present: %s", sessionPresent ? "true" : "false");
        client.subscribe(MQTT_PRODUCT_DATA_RESPONSE_TOPIC.c_str(), 1);
        client.subscribe(MQTT_FIRMWARE_UPDATE_TOPIC, 1);
    };
}

void networkTask(void* pvParameters)
{
    static espMqttClient mqttClient;
    const auto* params = static_cast<NetworkTaskParams*>(pvParameters);
    constexpr TickType_t wifiRestartTimeoutTicks = pdMS_TO_TICKS(30000);

    mqttClient.onMessage(onMessageCallback(params->printQueue, params->otaTaskHandle));
    mqttClient.onConnect(onConnectCallback(mqttClient));
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setClientId(MQTT_CLIENT_NAME);
    mqttClient.setCleanSession(true);
    mqttClient.setKeepAlive(15);

    ESP_LOGD(TAG, "Connecting to WiFi");
    PrintMessage statusMessage = {};
    statusMessage.type = PRINT_NETWORK_STATUS;
    statusMessage.data.networkStatus.isWifiConnected = false;
    statusMessage.data.networkStatus.isMqttConnected = false;
    strlcpy(statusMessage.data.networkStatus.ipAddressLastOctet, "", sizeof(statusMessage.data.networkStatus.ipAddressLastOctet));
    xQueueSend(params->printQueue, &statusMessage, pdMS_TO_TICKS(100));

    WiFi.persistent(false);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (!WiFi.isConnected()) {
        vTaskDelay(pdMS_TO_TICKS(300));
        ESP_LOGD(TAG, ".");
    }

    statusMessage.data.networkStatus.isWifiConnected = true;
    IPAddress ip = WiFi.localIP();
    snprintf(statusMessage.data.networkStatus.ipAddressLastOctet, sizeof(statusMessage.data.networkStatus.ipAddressLastOctet), "%d", ip[3]);
    xQueueSend(params->printQueue, &statusMessage, pdMS_TO_TICKS(100));

    ESP_LOGD(TAG, "Connecting to MQTT broker");
    mqttClient.connect();
    while (!mqttClient.connected()) {
        vTaskDelay(pdMS_TO_TICKS(300));
        ESP_LOGD(TAG, ".");
    }

    statusMessage.data.networkStatus.isMqttConnected = true;
    xQueueSend(params->printQueue, &statusMessage, pdMS_TO_TICKS(100));

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
