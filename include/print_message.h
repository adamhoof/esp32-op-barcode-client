#pragma once

#include "response.h"

#define MAX_ERROR_MESSAGE_LENGTH 128

enum PrintMessageType {
    PRINT_PRODUCT_DATA,
    PRINT_ERROR_MESSAGE,
    PRINT_NETWORK_STATUS
};

struct NetworkStatusMessage {
    bool isWifiConnected;
    bool isMqttConnected;
};

struct PrintMessage {
    PrintMessageType type;
    union {
        MqttProductDataResponse productData;
        char errorMessage[MAX_ERROR_MESSAGE_LENGTH];
        NetworkStatusMessage networkStatus;
    } data;
};
