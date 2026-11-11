
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";
AsyncWebServer server(80);
Adafruit_BME280 bme;
float temperature = NAN, humidity = NAN, pressure = NAN;
unsigned long lastRead = 0;
const unsigned long READ_INTERVAL = 2000;
void updateSensor() {
  if (millis() - lastRead < READ_INTERVAL) return;
  lastRead = millis();
  temperature = bme.readTemperature();
  humidity = bme.readHumidity();
  pressure = bme.readPressure() / 100.0F;
  Serial.printf("Temperature: %.2f C | Humidity: %.2f %% | Pressure: %.2f hPa\n", temperature, humidity, pressure);
}
void sendReading(AsyncWebServerRequest *request, float value) {
  if (!isfinite(value)) {
    request->send(503, "text/plain", "Sensor unavailable");
    return;
  }
  request->send(200, "text/plain", String(value, 2));
}
void setup() {
  Serial.begin(115200);
  if (!bme.begin(0x76) && !bme.begin(0x77)) {
    Serial.println("BME280 not found!");
    while (true) delay(1000);
  }
  if (!LittleFS.begin(false)) {
    Serial.println("LittleFS mounting failed!");
    while (true) delay(1000);
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.println(WiFi.localIP());
  updateSensor();
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", "text/html");
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request) {
    sendReading(request, temperature);
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request) {
    sendReading(request, humidity);
  });
  server.on("/pressure", HTTP_GET, [](AsyncWebServerRequest *request) {
    sendReading(request, pressure);
  });
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Page not found");
  });
  server.begin();
  Serial.println("Web server started!");
}
void loop() {
  updateSensor();
}