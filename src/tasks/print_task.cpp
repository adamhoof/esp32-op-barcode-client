#include "tasks/print_task.h"
#include <Arduino.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_GFX.h>
#include <esp_log.h>
#include "config.h"
#include "response.h"
#include "arial.h"
#include "print_message.h"

static const char* TAG = "PRINT_TASK";

void printNetworkStatusMessage(Adafruit_ILI9341& display, const NetworkStatusMessage& status)
{
    display.fillScreen(ILI9341_BLACK);
    display.setTextSize(1);
    display.setCursor(0, 20);
    display.setTextColor(ILI9341_WHITE);

    display.print("WiFi: ");
    if (status.isWifiConnected) {
        display.setTextColor(ILI9341_GREEN);
        display.print("connected ");
        display.setTextColor(ILI9341_WHITE);
        display.println(status.ipAddressLastOctet);
    } else {
        display.setTextColor(ILI9341_RED);
        display.println("disconnected");
    }
    display.setTextColor(ILI9341_WHITE);
    display.setCursor(0, 50);
    display.print("MQTT: ");
    if (status.isMqttConnected) {
        display.setTextColor(ILI9341_GREEN);
        display.println("connected");
    } else {
        display.setTextColor(ILI9341_RED);
        display.println("disconnected");
    }
}

void printErrorMessage(Adafruit_ILI9341& display, const char* erroMessage)
{
    display.fillScreen(ILI9341_BLACK);
    display.setCursor(0, 30);
    display.setTextSize(2);
    display.setTextColor(ILI9341_RED);
    display.print(erroMessage);
}

void printProductData(Adafruit_ILI9341 &display, const MqttProductDataResponse& productData) {
    display.fillScreen(ILI9341_BLACK);
    display.setCursor(0, 20);
    display.setTextSize(1);
    display.setTextColor(ILI9341_WHITE);
    display.printf("\n%s\n\n", productData.name);

    display.setTextSize(2);
    display.setTextColor(ILI9341_GREEN);
    display.printf("Cena: %.2f kc\n", productData.price);

    display.setTextSize(1);
    display.setTextColor(ILI9341_WHITE);
    if (strlen(productData.unitOfMeasure) > 0) {
        display.printf("Cena za %s: %.2f kc\n\n",
                       productData.unitOfMeasure,
                       productData.price * productData.unitOfMeasureKoef);
    }

    display.printf("Skladem: %hu", productData.stock);
}

void printTask(void* pvParameters) {
    const auto* params = static_cast<const PrintTaskParams*>(pvParameters);
    Adafruit_ILI9341 display = Adafruit_ILI9341(DISPLAY_CS_PIN, DISPLAY_DC_PIN, DISPLAY_RST_PIN);
    display.begin();
    display.setRotation(1);
    display.setFont(&ariblk9pt8b);
    display.fillScreen(ILI9341_BLACK);
    display.setTextColor(ILI9341_WHITE);
    display.setTextSize(2);
    display.setCursor(10, 30);
    display.println("^__^");
    ESP_LOGD(TAG, "Print task started");

    for (;;)
    {
        PrintMessage receivedMessage{};
        if (xQueueReceive(params->printQueue, &receivedMessage, portMAX_DELAY)) {
            switch (receivedMessage.type) {
                case PRINT_PRODUCT_DATA:
                    ESP_LOGD(TAG, "Received product data for display: %s", receivedMessage.data.productData.name);
                    printProductData(display, receivedMessage.data.productData);
                    break;
                case PRINT_ERROR_MESSAGE:
                    ESP_LOGD(TAG, "Received error message for display: %s", receivedMessage.data.errorMessage);
                    printErrorMessage(display, receivedMessage.data.errorMessage);
                    break;
                case PRINT_NETWORK_STATUS:
                    ESP_LOGD(TAG, "Received network status for display. WiFi: %d, MQTT: %d", receivedMessage.data.networkStatus.isWifiConnected, receivedMessage.data.networkStatus.isMqttConnected);
                    printNetworkStatusMessage(display, receivedMessage.data.networkStatus);
                    break;
            }
        }
    }
}
