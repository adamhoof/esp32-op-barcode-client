#pragma once

#include "response.h"

#define MAX_ERROR_MESSAGE_LENGTH 128

enum PrintMessageType {
    PRINT_PRODUCT_DATA,
    PRINT_ERROR_MESSAGE,
};

struct PrintMessage {
    PrintMessageType type;
    union {
        MqttProductDataResponse productData;
        char errorMessage[MAX_ERROR_MESSAGE_LENGTH];
    } data;
};
