/*
  =====================================================================
  TANAALERT - PENGUJIAN KOMUNIKASI LoRa: RECEIVER
  =====================================================================
  Sketch ini KHUSUS untuk pengujian Tabel 3.11 (Pengujian Komunikasi
  LoRa) -- bukan receiver utama TANAALERT (tidak ada klasifikasi
  Random Forest, Firebase, atau Telegram di sini, supaya fokus murni
  menguji jarak & keandalan LoRa).

  Tugas:
  1. Terima paket dari lora_test_transmitter.ino
  2. Hitung "Data Terkirim" (dari nomor seq yang dibawa paket) vs
     "Data Diterima" (counter lokal paket yang benar2 sampai)
  3. Hitung RSSI dan estimasi delay
  4. Nyalakan buzzer sebentar setiap ada paket masuk
  5. Kirim baris log ke Google Sheets (tab "LoRaTest")

  CATATAN PENTING soal Delay:
  Delay dihitung dari millis() transmitter (ditempel di paket) vs
  millis() receiver saat paket tiba. Ini HANYA akurat kalau kedua
  ESP32 di-reset/nyala BERSAMAAN saat mulai pengujian, karena millis()
  masing-masing board mulai dari 0 sendiri-sendiri saat boot (tidak
  ada jam yang disinkronkan). Untuk pengujian jarak per skenario,
  nyalakan ulang (reset) KEDUA board bersamaan sebelum mulai logging
  supaya nilai delay valid secara relatif.

  WIRING:
  LoRa SX1278: sama seperti receiver_edge_gateway.ino
  Buzzer aktif: SIG -> GPIO4
  =====================================================================
*/ 

#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// ====================== KONFIGURASI WIFI ======================
#define WIFI_SSID      "Bos Dian"
#define WIFI_PASSWORD  "pakenanya"

// ====================== KONFIGURASI GOOGLE SHEETS ======================
#define SCRIPT_URL "https://script.google.com/macros/s/AKfycbyNCXKMpOk4nLQ5GqZTVD1zjk9GzryEcavwej0x3StcVZVhl-qbrlmjaK2NWm7H0kGt/exec"

// ====================== KONFIGURASI PIN ======================
#define LORA_NSS      5
#define LORA_RST      14
#define LORA_DIO0     2
#define LORA_SCK      18
#define LORA_MISO     19
#define LORA_MOSI     23
#define PIN_BUZZER    4

#define LORA_FREQUENCY 433E6   // HARUS sama dengan transmitter
#define BUZZER_BEEP_MS 300UL

// ====================== STATE ======================
unsigned long receivedCounter = 0;   // jumlah paket yang BENAR-BENAR sampai

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("=== TANAALERT - Pengujian LoRa: RECEIVER ===");

  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  setupLoRa();
  connectWiFi();

  Serial.println("[OK] Receiver siap menerima paket uji.");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize > 0) {
    String rawData = "";
    while (LoRa.available()) {
      rawData += (char)LoRa.read();
    }
    int rssi = LoRa.packetRssi();
    unsigned long receiveMillis = millis();

    handleReceivedPacket(rawData, rssi, receiveMillis);
  }

  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
}

void setupLoRa() {
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("[ERROR] LoRa gagal diinisialisasi! Periksa wiring SPI.");
    while (1) delay(1000);
  }
  LoRa.setSpreadingFactor(9);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  Serial.println("[OK] LoRa siap menerima.");
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print("Menyambungkan ke WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED && attempt < 30) {
    delay(500);
    Serial.print(".");
    attempt++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[OK] WiFi terhubung. IP: " + WiFi.localIP().toString());
  } else {
    Serial.println("\n[WARNING] WiFi belum tersambung, akan dicoba lagi.");
  }
}

void handleReceivedPacket(String rawData, int rssi, unsigned long receiveMillis) {
  receivedCounter++;

  // Field pertama = seq, field kedua = sendMillis dari transmitter
  int firstComma = rawData.indexOf(',');
  int secondComma = rawData.indexOf(',', firstComma + 1);

  if (firstComma == -1 || secondComma == -1) {
    Serial.println("[WARNING] Format paket tidak valid, dilewati: " + rawData);
    return;
  }

  unsigned long seq = rawData.substring(0, firstComma).toInt();
  unsigned long sendMillis = rawData.substring(firstComma + 1, secondComma).toInt();

  float delaySec = (receiveMillis >= sendMillis)
                      ? (receiveMillis - sendMillis) / 1000.0
                      : 0.0;  // jaga2 kalau millis() overflow/tidak sinkron

  String keterangan;
  if (seq == receivedCounter) {
    keterangan = "Tidak ada paket hilang";
  } else if (seq > receivedCounter) {
    keterangan = "Hilang " + String(seq - receivedCounter) + " paket";
  } else {
    keterangan = "Seq tidak normal (cek reset board)";
  }

  Serial.println("---- Paket Diterima ----");
  Serial.println("Seq (terkirim s/d sekarang) : " + String(seq));
  Serial.println("Diterima (kumulatif)        : " + String(receivedCounter));
  Serial.println("RSSI                        : " + String(rssi) + " dBm");
  Serial.println("Delay (estimasi)            : " + String(delaySec, 3) + " s");
  Serial.println("Keterangan                  : " + keterangan);
  Serial.println("Raw                         : " + rawData);
  Serial.println("-------------------------\n");

  // Bunyikan buzzer sebentar sebagai indikator data diterima
  digitalWrite(PIN_BUZZER, HIGH);
  delay(BUZZER_BEEP_MS);
  digitalWrite(PIN_BUZZER, LOW);

  sendToGoogleSheet(seq, receivedCounter, rssi, delaySec, keterangan);
}

void sendToGoogleSheet(unsigned long dataTerkirim, unsigned long dataDiterima,
                        int rssi, float delaySec, String keterangan) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[ERROR] WiFi tidak terhubung, log tidak terkirim ke Sheets.");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  String url = String(SCRIPT_URL) +
               "?sheet=LoRaTest" +
               "&dataTerkirim=" + String(dataTerkirim) +
               "&dataDiterima=" + String(dataDiterima) +
               "&rssi=" + String(rssi) +
               "&delay=" + String(delaySec, 3) +
               "&keterangan=" + keterangan;

  http.begin(client, url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    Serial.println("[OK] Log terkirim ke Google Sheets.");
  } else {
    Serial.println("[ERROR] Gagal kirim log: " + http.errorToString(httpCode));
  }
  http.end();
}
