#include "../../include/tasks/print_task.h"
#include <Arduino.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_GFX.h>
#include "config.h"
#include "response.h"

void printProductData(Adafruit_ILI9341 &display, const MqttProductDataResponse& productData) {
    display.fillScreen(ILI9341_BLACK);
    display.setCursor(0, 20);
    display.setTextSize(2);
    display.setTextColor(ILI9341_WHITE);
    display.printf("\n%s\n\n", productData.name);

    display.setTextSize(3);
    display.setTextColor(ILI9341_GREEN);
    display.printf("Cena: %.2f kc\n", productData.price);

    display.setTextSize(2);
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
    display.fillScreen(ILI9341_BLACK);
    display.setTextColor(ILI9341_WHITE);
    display.setTextSize(2);
    display.setCursor(10, 10);
    display.println("Waiting for scan...");
    Serial.println("Print task started");

    for (;;) {
        MqttProductDataResponse receivedData{};
        if (xQueueReceive(params->incomingQueue, &receivedData, portMAX_DELAY)) {
            Serial.printf("Received product data for display: %s\n", receivedData.name);
            printProductData(display, receivedData);
        }
    }
}
