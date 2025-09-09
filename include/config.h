#pragma once

#include <string>

const char* const WIFI_SSID = "Medunka";
const char* const WIFI_PASS = "med1974unka";

const char* const MQTT_CLIENT_NAME = "op-barcode-esp32-back-corner";
const char* const MQTT_SERVER = "10.0.0.138";
constexpr int MQTT_PORT = 1883;

const char* const MQTT_PRODUCT_DATA_REQUEST_TOPIC = "/get_product_data";
const std::string MQTT_PRODUCT_DATA_RESPONSE_TOPIC = std::string(MQTT_CLIENT_NAME) + MQTT_PRODUCT_DATA_REQUEST_TOPIC;
const char* const MQTT_LIGHT_COMMAND_TOPIC = "/light";
const char* const MQTT_FIRMWARE_UPDATE_TOPIC = "/firmware_update";

constexpr int BARCODE_TX_PIN = 15;
constexpr int BARCODE_RX_PIN = 13;
constexpr int BARCODE_BAUD_RATE = 9600;
constexpr char BARCODE_DELIMITER = '\r';
constexpr int BARCODE_BUFFER_SIZE = 30;

constexpr int DISPLAY_CS_PIN = 32;
constexpr int DISPLAY_DC_PIN = 26;
constexpr int DISPLAY_RST_PIN = 25;
