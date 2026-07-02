#include <WiFi.h>
#include <HTTPClient.h>

#define SOIL_AO 35

const char* ssid = "Bos Dian";
const char* password = "pakenanya";

String GAS_URL = "https://script.google.com/macros/s/AKfycbz8u3nm9f3id046dKDSufH8n5vMpmmlxmafUaD8YrMdXirGMq7k8AI54EvhYeD-DPCsbw/exec";

int analogValue;
float moisturePercent;

void setup() {

  Serial.begin(115200);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");
}

void loop() {

  analogValue = analogRead(SOIL_AO);

  moisturePercent = map(analogValue, 4095, 0, 0, 100);

  String status;

  if (moisturePercent <= 34)
      status = "Kering";
  else if (moisturePercent <= 68)
      status = "Lembab";
  else
      status = "Basah";

  Serial.print("Analog : ");
  Serial.print(analogValue);

  Serial.print(" | Moisture : ");
  Serial.print(moisturePercent);

  Serial.print("% | ");
  Serial.println(status);

  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;

    String url = GAS_URL +
                 "?analog=" + String(analogValue) +
                 "&moisture=" + String(moisturePercent,1) +
                 "&status=" + status;

    http.begin(url);

    int httpCode = http.GET();

    Serial.print("HTTP Code : ");
    Serial.println(httpCode);

    http.end();
  }

  delay(2000);
}