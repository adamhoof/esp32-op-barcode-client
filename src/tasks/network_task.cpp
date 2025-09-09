#include "tasks/network_task.h"
#include <Arduino.h>
#include <espMqttClient.h>
#include <ArduinoJson.h>
#include "config.h"
#include "request.h"
#include "response.h"

espMqttClientTypes::OnMessageCallback onMessageCallback(QueueHandle_t incomingQueue)
{
    return [incomingQueue](const espMqttClientTypes::MessageProperties& properties, const char* topic,
                           const uint8_t* payload, size_t len, size_t index, size_t total) {
        MqttProductDataResponse response{};
        StaticJsonDocument<350> doc;
        const DeserializationError error = deserializeJson(doc, payload, len);
        if (error) {
            return;
        }
        strlcpy(response.name, doc["name"] | "", sizeof(response.name));
        response.price = doc["price"] | 0.0;
        response.stock = doc["stock"] | 0;
        strlcpy(response.unitOfMeasure, doc["unitOfMeasure"] | "", sizeof(response.unitOfMeasure));
        response.unitOfMeasureKoef = doc["unitOfMeasureCoef"] | 0.0;
        xQueueSend(incomingQueue, &response, pdMS_TO_TICKS(100));
    };
}

espMqttClientTypes::OnConnectCallback onConnectCallback(espMqttClient& client)
{
    return [&client](bool sessionPresent) {
        client.subscribe(MQTT_PRODUCT_DATA_RESPONSE_TOPIC.c_str(), 1);
    };
}

void networkTask(void* pvParameters)
{
    static espMqttClient mqttClient;
    const auto* params = static_cast<NetworkTaskParams*>(pvParameters);
    constexpr TickType_t wifiRestartTimeoutTicks = pdMS_TO_TICKS(30000);

    mqttClient.onMessage(onMessageCallback(params->incomingQueue));
    mqttClient.onConnect(onConnectCallback(mqttClient));
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setClientId(MQTT_CLIENT_NAME);

    while (true) {
        mqttClient.loop();

        if (WiFi.isConnected()) {

            if (!mqttClient.connected()) {
                mqttClient.connect();
            }
            else {
                MqttProductDataRequest receivedRequest{};
                if (xQueueReceive(params->outgoingQueue, &receivedRequest, pdMS_TO_TICKS(1000)) == pdPASS) {
                    mqttClient.publish(receivedRequest.topic, 1, false, receivedRequest.payload);
                }
            }
        }
        else { // WiFi is disconnected, auto-retry will try to reconnect
            const TickType_t startTime = xTaskGetTickCount();
            while (!WiFi.isConnected() && (xTaskGetTickCount() - startTime < wifiRestartTimeoutTicks)) {
                vTaskDelay(pdMS_TO_TICKS(300));
            }
            if (!WiFi.isConnected()) {
                ESP.restart();
            }
        }
    }
}
