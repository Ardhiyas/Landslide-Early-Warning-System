#include <WiFi.h>
#include <HTTPClient.h>

// =========================
// WiFi
// =========================
const char* ssid = "Bos Dian";
const char* password = "pakenanya";

String GAS_URL = "https://script.google.com/macros/s/AKfycbxq9-l0JvUbC_JuBWaCA7gE06s_K0CS_DGJKD8nfDvPkFlOLjsTmUx5Ort5DhXIx5pA/exec";

// =========================
// Pin Sensor
// =========================
#define RAIN_AO 34
#define RAIN_DO 26

int analogValue;
int digitalValue;
float rainIntensity;

void setup() {

  Serial.begin(115200);

  pinMode(RAIN_DO, INPUT);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");
}

void loop() {

  analogValue = analogRead(RAIN_AO);
  digitalValue = digitalRead(RAIN_DO);

  rainIntensity = map(analogValue, 4095, 0, 0, 100);

  String status;

  if (rainIntensity <= 33)
    status = "Terang";
  else if (rainIntensity <= 68)
    status = "Gerimis";
  else
    status = "Hujan";

  Serial.print("Analog : ");
  Serial.print(analogValue);

  Serial.print(" | Intensitas : ");
  Serial.print(rainIntensity);

  Serial.print("% | ");
  Serial.println(status);

  if (WiFi.status() == WL_CONNECTED) {

    HTTPClient http;

    String url = GAS_URL +
                 "?analog=" + String(analogValue) +
                 "&intensity=" + String(rainIntensity,1) +
                 "&status=" + status;

    http.begin(url);

    int httpCode = http.GET();

    Serial.print("HTTP Response : ");
    Serial.println(httpCode);

    http.end();
  }

  delay(2000);
}