#ifndef CONFIG_H
#define CONFIG_H

// WiFi
#define WIFI_SSID "YourNetwork"
#define WIFI_PASSWORD "YourPassword"

// MQTT
#define MQTT_BROKER "192.168.1.100"
#define MQTT_PORT 1883

// Pins
#define SOIL_MOISTURE_PIN A0
#define RELAY_PIN D6
#define DHT20_SDA_PIN D4
#define DHT20_SCL_PIN D5

// Soil Calibration (MUST calibrate for your soil)
#define SOIL_DRY 600
#define SOIL_WET 250

// Thresholds
#define MOISTURE_THRESHOLD_ON 40
#define MOISTURE_THRESHOLD_OFF 70

// Read intervals (ms)
#define SENSOR_READ_INTERVAL 30000
#define MQTT_PUBLISH_INTERVAL 60000

#endif
