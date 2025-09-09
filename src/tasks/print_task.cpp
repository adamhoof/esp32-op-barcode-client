#include "tasks/print_task.h"
#include <Arduino.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_GFX.h>
#include <esp_log.h>
#include "config.h"
#include "response.h"
#include "arial.h"

static const char* TAG = "PRINT_TASK";

void printErrorMessage(Adafruit_ILI9341& display, const char* erroMessage)
{
    display.fillScreen(ILI9341_BLACK);
    display.setCursor(0, 30);
    display.setTextSize(2);
    display.setTextColor(ILI9341_RED);
    display.print(erroMessage);
}

void printProductData(Adafruit_ILI9341 &display, const MqttProductDataResponse& productData) {
    if (strlen(productData.name) == 0 || productData.price == 0) {
        printErrorMessage(display, "Zkuste znovu\nnebo\nzavolejte obsluhu...");
        return;
    }
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

    for (;;) {
        MqttProductDataResponse receivedData{};
        if (xQueueReceive(params->incomingQueue, &receivedData, portMAX_DELAY)) {
            ESP_LOGD(TAG, "Received product data for display: %s", receivedData.name);
            printProductData(display, receivedData);
        }
    }
}

