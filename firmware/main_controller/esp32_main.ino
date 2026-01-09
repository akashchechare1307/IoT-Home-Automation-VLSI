/*
 * IoT-Based Home Automation System
 * ESP32 Main Controller Firmware
 * 
 * Features:
 * - Multi-sensor data acquisition
 * - Wireless control via WiFi/Bluetooth
 * - Real-time device management
 * - Power optimization
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

// WiFi Configuration
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";
const char* mqtt_server = "mqtt.example.com";

// Pin Definitions
#define DHT_PIN 4
#define DHT_TYPE DHT22
#define PIR_PIN 5
#define LDR_PIN 34
#define RELAY_LIGHT 12
#define RELAY_FAN 13
#define RELAY_PUMP 14
#define STATUS_LED 2

// Sensor Objects
DHT dht(DHT_PIN, DHT_TYPE);
WiFiClient espClient;
PubSubClient client(espClient);

// Global Variables
float temperature = 0.0;
float humidity = 0.0;
bool motionDetected = false;
int lightIntensity = 0;

// Function Declarations
void setup();
void loop();
void initWiFi();
void initMQTT();
void readSensors();
void updateActuators();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void reconnect();

void setup() {
  Serial.begin(115200);
  delay(100);
  
  // Pin Configuration
  pinMode(PIR_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(RELAY_LIGHT, OUTPUT);
  pinMode(RELAY_FAN, OUTPUT);
  pinMode(RELAY_PUMP, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);
  
  // Initialize Sensors
  dht.begin();
  
  // Initialize WiFi and MQTT
  initWiFi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);
  
  digitalWrite(STATUS_LED, HIGH);
  Serial.println("System initialized successfully");
}

void loop() {
  // Check WiFi Connection
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  // Read sensor values
  readSensors();
  
  // Update actuators based on sensor input and MQTT commands
  updateActuators();
  
  // Publish sensor data every 5 seconds
  delay(5000);
  
  // Publish MQTT data
  char tempStr[8];
  char humStr[8];
  sprintf(tempStr, "%.1f", temperature);
  sprintf(humStr, "%.1f", humidity);
  
  client.publish("home/sensors/temperature", tempStr);
  client.publish("home/sensors/humidity", humStr);
  client.publish("home/sensors/motion", motionDetected ? "detected" : "clear");
}

void initWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi");
  }
}

void readSensors() {
  // Read DHT22 Temperature and Humidity
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();
  
  // Read PIR Motion Sensor
  motionDetected = digitalRead(PIR_PIN) == HIGH;
  
  // Read LDR Light Sensor
  lightIntensity = analogRead(LDR_PIN);
  
  // Print sensor data to Serial
  Serial.print("Temp: ");
  Serial.print(temperature);
  Serial.print("C, Humidity: ");
  Serial.print(humidity);
  Serial.print("%,  Motion: ");
  Serial.print(motionDetected ? "Detected" : "Clear");
  Serial.print(", Light: ");
  Serial.println(lightIntensity);
}

void updateActuators() {
  // Smart light control based on motion and light intensity
  if (motionDetected && lightIntensity < 500) {
    digitalWrite(RELAY_LIGHT, HIGH);
  } else if (!motionDetected) {
    digitalWrite(RELAY_LIGHT, LOW);
  }
  
  // Smart fan control based on temperature
  if (temperature > 28.0) {
    digitalWrite(RELAY_FAN, HIGH);
  } else if (temperature < 24.0) {
    digitalWrite(RELAY_FAN, LOW);
  }
  
  // Pump control can be triggered via MQTT
  // Default: OFF
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String command = "";
  for (int i = 0; i < length; i++) {
    command += (char)payload[i];
  }
  
  // Parse and execute commands
  if (String(topic) == "home/control/light") {
    digitalWrite(RELAY_LIGHT, command == "ON" ? HIGH : LOW);
  } else if (String(topic) == "home/control/fan") {
    digitalWrite(RELAY_FAN, command == "ON" ? HIGH : LOW);
  } else if (String(topic) == "home/control/pump") {
    digitalWrite(RELAY_PUMP, command == "ON" ? HIGH : LOW);
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      client.subscribe("home/control/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
