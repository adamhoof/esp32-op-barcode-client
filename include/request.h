#pragma once

struct MqttProductDataRequest {
    char topic[128];
    char payload[256];
};
