#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT20.h"
#include <ArduinoJson.h>
#include "config.h"

DHT20 dht20;
WiFiClient espClient;
PubSubClient client(espClient);

unsigned long lastSensorRead = 0;
unsigned long lastMQTTPublish = 0;
bool pumpRunning = false;
int soilMoisture = 0;
float temperature = 0, humidity = 0;

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n=== Smart Farm ESP32 ===");
  
pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  
  Wire.begin(DHT20_SDA_PIN, DHT20_SCL_PIN);
  if (!dht20.begin()) {
    Serial.println("ERROR: DHT20 not found");
    while (1) delay(1000);
  }
  
  connectWiFi();
  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setCallback(onMQTTMessage);
}

void loop() {
  if (!client.connected()) reconnectMQTT();
  client.loop();
  
  unsigned long now = millis();
  
  if (now - lastSensorRead >= SENSOR_READ_INTERVAL) {
    readSensors();
    lastSensorRead = now;
  }
  
  if (now - lastMQTTPublish >= MQTT_PUBLISH_INTERVAL) {
    publishData();
    lastMQTTPublish = now;
  }
  
  delay(100);
}

void readSensors() {
  int raw = analogRead(SOIL_MOISTURE_PIN);
  soilMoisture = map(raw, SOIL_DRY, SOIL_WET, 0, 100);
  soilMoisture = constrain(soilMoisture, 0, 100);
  
  if (dht20.read() == 0) {
    temperature = dht20.getTemperature();
    humidity = dht20.getHumidity();
  }
  
  Serial.printf("Soil: %d%% | Temp: %.1f°C | Humidity: %.1f%%\n", 
                soilMoisture, temperature, humidity);
}

void publishData() {
  StaticJsonDocument<200> doc;
  doc["moisture"] = soilMoisture;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["pump"] = pumpRunning ? "ON" : "OFF";
  
  char buffer[256];
  serializeJson(doc, buffer);
  client.publish("farm/data", buffer);
  
  controlPump();
}

void controlPump() {
  if (soilMoisture < MOISTURE_THRESHOLD_ON && !pumpRunning) {
    digitalWrite(RELAY_PIN, HIGH);
    pumpRunning = true;
    client.publish("farm/pump/status", "ON");
    Serial.println("💧 PUMP ON");
  }
  else if (soilMoisture > MOISTURE_THRESHOLD_OFF && pumpRunning) {
    digitalWrite(RELAY_PIN, LOW);
    pumpRunning = false;
    client.publish("farm/pump/status", "OFF");
    Serial.println("🛑 PUMP OFF");
  }
}

void onMQTTMessage(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (int i = 0; i < length; i++) msg += (char)payload[i];
  
  if (String(topic) == "farm/pump/command") {
    if (msg == "ON") {
      digitalWrite(RELAY_PIN, HIGH);
      pumpRunning = true;
    } else if (msg == "OFF") {
      digitalWrite(RELAY_PIN, LOW);
      pumpRunning = false;
    }
  }
}

void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\n✓ WiFi connected: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n✗ WiFi failed");
  }
}

void reconnectMQTT() {
  static unsigned long lastAttempt = 0;
  if (millis() - lastAttempt < 5000) return;
  lastAttempt = millis();
  
  if (client.connect("SmartFarmESP32")) {
    Serial.println("✓ MQTT connected");
    client.subscribe("farm/pump/command");
  } else {
    Serial.print("✗ MQTT failed: ");
    Serial.println(client.state());
  }
}