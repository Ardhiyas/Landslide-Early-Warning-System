/*
  =====================================================================
  TANAALERT - DATA COLLECTOR: SENSOR SOIL MOISTURE (mode otomatis)
  =====================================================================
  Sketch ini mengirim data SECARA OTOMATIS dan TERUS-MENERUS tiap
  SEND_INTERVAL_MS, tanpa perlu menekan tombol setiap kali mau ambil
  satu sampel.

  CARA PAKAI:
  1. Buka Serial Monitor (115200 baud).
  2. Ketik label sekali lalu Enter -> a = Aman | w = Waspada | b = Bahaya
  3. Sketch akan otomatis mengirim data setiap beberapa detik dengan
     label tersebut, terus-menerus, tanpa input lagi.
  4. Atur kondisi fisik tanah sesuai label yang sedang aktif (kering /
     lembab / basah), biarkan mengirim beberapa puluh sampel.
  5. Saat mau pindah kondisi, ketik label baru kapan saja (tidak perlu
     berhenti) -> label langsung berubah untuk sampel-sampel berikutnya.

  WIRING:
  Sensor Soil Moisture v2      VCC -> 3.3V, GND -> GND, AO -> GPIO35
  =====================================================================
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// ====================== KONFIGURASI WIFI ======================
#define WIFI_SSID      "Bos Dian"
#define WIFI_PASSWORD  "pakenanya"

// ====================== KONFIGURASI GOOGLE SHEETS ======================
#define SCRIPT_URL "https://script.google.com/macros/s/AKfycbznr7wbkde8IL5KFT3BuBBEvTRvlDv_0Ke8fmCFE9K4ytslsEaHC_W3qqAXrcUvPyEx9Q/exec"

// ====================== KONFIGURASI PIN & KALIBRASI ======================
#define PIN_SOIL_AO   35

#define SOIL_ADC_DRY   3400
#define SOIL_ADC_WET   1200

// Seberapa sering data dikirim otomatis (ubah sesuai kebutuhan)
#define SEND_INTERVAL_MS 2000UL

// ====================== STATE ======================
String currentLabel = "";   // kosong = belum ada label, belum mulai kirim
unsigned long lastSendTime = 0;

void setup() {
  Serial.begin(115200);
  delay(500);

  analogReadResolution(12);
  pinMode(PIN_SOIL_AO, INPUT);

  connectWiFi();

  Serial.println("\n=== TANAALERT Data Collector - SOIL MOISTURE (otomatis) ===");
  printMenu();
}

void loop() {
  checkSerialForLabelChange();

  if (currentLabel != "" && millis() - lastSendTime >= SEND_INTERVAL_MS) {
    collectAndSend(currentLabel);
    lastSendTime = millis();
  }
}

void printMenu() {
  Serial.println("Ketik label lalu Enter -> a = Aman | w = Waspada | b = Bahaya");
  Serial.println("(Boleh diganti kapan saja, pengiriman akan otomatis berlanjut)\n");
}

void checkSerialForLabelChange() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    input.toLowerCase();

    if (input == "a") currentLabel = "Aman";
    else if (input == "w") currentLabel = "Waspada";
    else if (input == "b") currentLabel = "Bahaya";
    else {
      Serial.println("[INFO] Input tidak dikenali. Ketik a / w / b lalu Enter.");
      return;
    }

    Serial.println("[LABEL AKTIF] " + currentLabel + " -> pengiriman otomatis dimulai/berlanjut.");
  }
}

void connectWiFi() {
  Serial.print("Menyambungkan ke WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\n[OK] WiFi terhubung. IP: " + WiFi.localIP().toString());
}

void collectAndSend(String label) {
  int adcValue = analogRead(PIN_SOIL_AO);
  int pct = map(adcValue, SOIL_ADC_DRY, SOIL_ADC_WET, 0, 100);
  pct = constrain(pct, 0, 100);

  Serial.printf("[KIRIM] ADC=%d  Pct=%d%%  Label=%s\n", adcValue, pct, label.c_str());

  sendToGoogleSheet(adcValue, pct, label);
}

void sendToGoogleSheet(int adcValue, int pct, String label) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[ERROR] WiFi tidak terhubung, data tidak terkirim.");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  String url = String(SCRIPT_URL) +
               "?sheet=Soil" +
               "&adc=" + String(adcValue) +
               "&pct=" + String(pct) +
               "&label=" + label;

  http.begin(client, url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    Serial.println("         -> OK. Response: " + http.getString());
  } else {
    Serial.println("         -> ERROR kirim: " + http.errorToString(httpCode));
  }
  http.end();
}
