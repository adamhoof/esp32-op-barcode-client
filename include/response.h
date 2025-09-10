#pragma once

#include <cstdint>

struct MqttProductDataResponse {
    char name[100];
    double price;
    uint16_t stock;
    char unitOfMeasure[20];
    double unitOfMeasureKoef;
};
