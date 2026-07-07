/*
  =====================================================================
  TANAALERT - PENGUJIAN KOMUNIKASI LoRa: RECEIVER (RTT Piggyback)
  =====================================================================
  Receiver menampilkan delay, RSSI, Data Terkirim, Data Diterima,
  dan keterangan -- semua tanpa transmitter perlu terhubung ke laptop.

  Delay yang ditampilkan = delay satu arah paket SEBELUMNYA (hasil RTT
  yang dihitung transmitter dan disisipkan ke paket berikutnya).
  Paket pertama delay = 0 (normal, belum ada data sebelumnya).

  YANG DITAMPILKAN DI SERIAL MONITOR:
  Seq | Data Terkirim | Data Diterima | RSSI    | Delay    | Keterangan
    1 |             1 |             1 | -58 dBm |      0ms | Data Masuk (delay perdana)
    2 |             2 |             2 | -61 dBm |    175ms | Data Masuk
    3 |             3 |             3 | -59 dBm |    182ms | Data Masuk
    5 |             5 |             4 | -63 dBm |    190ms | Hilang 1 paket

  WIRING:
  LoRa SX1278: VCC->3.3V | GND->GND | RST->GPIO14 | DIO0->GPIO2
               NSS->GPIO5 | SCK->GPIO18 | MISO->GPIO19 | MOSI->GPIO23
  Buzzer aktif: SIG->GPIO4 | GND->GND
  =====================================================================
*/

#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// ====================== KONFIGURASI ======================
#define WIFI_SSID      "Bos Dian"
#define WIFI_PASSWORD  "pakenanya"
#define SCRIPT_URL     "https://script.google.com/macros/s/AKfycby9efyYNIt6U36lNuK4zmRe63SPbhTjs1Mn3Hsf31WSP9nXjrW10rMoIZAgY85xLlnm/exec"

#define LORA_NSS      5
#define LORA_RST      14
#define LORA_DIO0     2
#define LORA_SCK      18
#define LORA_MISO     19
#define LORA_MOSI     23
#define PIN_BUZZER    4

#define LORA_FREQUENCY  433E6
#define LORA_SYNC_WORD  0x12
#define BUZZER_BEEP_MS  150UL

// ====================== STATE ======================
unsigned long receivedCounter = 0;
bool wifiReady = false;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("=== TANAALERT LoRa Tester RECEIVER (RTT Piggyback) ===");
  Serial.println("Delay, RSSI, dan status paket ditampilkan di sini.");
  Serial.println("Transmitter tidak perlu terhubung ke laptop.\n");

  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  // LoRa DULU sebelum WiFi
  setupLoRa();

  // WiFi non-blocking
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("[WiFi] Menyambungkan di background...\n");

  printTableHeader();
}

void loop() {
  // Cek WiFi non-blocking
  if (!wifiReady && WiFi.status() == WL_CONNECTED) {
    wifiReady = true;
    Serial.println("\n[WiFi] Terhubung: " + WiFi.localIP().toString() +
                   " -> log akan masuk ke Google Sheets\n");
    printTableHeader();
  }

  // Cek paket LoRa
  int packetSize = LoRa.parsePacket();
  if (packetSize > 0) {
    String raw = "";
    while (LoRa.available()) raw += (char)LoRa.read();
    int rssi = LoRa.packetRssi();

    handlePacket(raw, rssi);
  }
}

// ====================== INIT LORA ======================
void setupLoRa() {
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("[ERROR] LoRa gagal! Periksa wiring.");
    while (1) delay(1000);
  }
  LoRa.setSpreadingFactor(9);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setSyncWord(LORA_SYNC_WORD);
  Serial.println("[OK] LoRa aktif.");
}

// ====================== HEADER TABEL SERIAL ======================
void printTableHeader() {
  Serial.println("  Seq | Terkirim | Diterima | RSSI     | Delay    | Keterangan");
  Serial.println("------------------------------------------------------------------");
}

// ====================== PROSES PAKET ======================
void handlePacket(String raw, int rssi) {
  // Format paket: seq,lastDelay,NODE_ID,rainADC,...
  //               [0] [1]       [2]     [3]
  int c1 = raw.indexOf(',');
  int c2 = raw.indexOf(',', c1 + 1);

  if (c1 == -1 || c2 == -1) {
    Serial.println("[WARNING] Format paket tidak valid: " + raw);
    return;
  }

  unsigned long seq      = raw.substring(0, c1).toInt();
  long          delayMs  = raw.substring(c1 + 1, c2).toInt();
                           // delay satu arah dari paket sebelumnya
                           // 0  = paket pertama (normal)
                           // -1 = paket sebelumnya tidak ada ACK

  receivedCounter++;

  // ═══════════════════════════════════════════════
  // KIRIM ACK SEGERA sebelum proses apapun
  // supaya RTT di transmitter tidak terpengaruh
  // waktu proses Serial.print atau HTTP request
  // ═══════════════════════════════════════════════
  LoRa.beginPacket();
  LoRa.print("ACK," + String(seq));
  LoRa.endPacket();
  // ═══════════════════════════════════════════════

  // Keterangan paket loss
  String keterangan;
  if (seq == receivedCounter) {
    keterangan = "Data Masuk";
    if (delayMs == 0 && seq == 1) keterangan = "Data Masuk (delay perdana)";
  } else if (seq > receivedCounter) {
    keterangan = "Hilang " + String(seq - receivedCounter) + " paket";
  } else {
    keterangan = "Cek reset board";
  }

  // Tampilan delay
  String delayStr;
  if (delayMs < 0) {
    delayStr = "  NO_ACK";   // transmitter tidak dapat ACK paket sebelumnya
  } else {
    delayStr = String(delayMs) + " ms";
  }

  // Tampilkan di Serial Monitor receiver
  Serial.printf("  %3lu | %8lu | %8lu | %4d dBm | %8s | %s\n",
                seq, seq, receivedCounter, rssi,
                delayStr.c_str(), keterangan.c_str());

  // Buzzer
  digitalWrite(PIN_BUZZER, HIGH);
  delay(BUZZER_BEEP_MS);
  digitalWrite(PIN_BUZZER, LOW);

  // Log ke Google Sheets
  if (wifiReady) {
    sendToSheets(seq, receivedCounter, rssi, delayMs, keterangan);
  } else {
    Serial.println("        [INFO] WiFi belum siap, log belum dikirim ke Sheets.");
  }
}

// ====================== KIRIM KE GOOGLE SHEETS ======================
void sendToSheets(unsigned long dataTerkirim, unsigned long dataDiterima,
                  int rssi, long delayMs, String keterangan) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  String url = String(SCRIPT_URL) +
               "?sheet=LoRaTest" +
               "&dataTerkirim=" + String(dataTerkirim) +
               "&dataDiterima=" + String(dataDiterima) +
               "&rssi="         + String(rssi) +
               "&delay="        + String(delayMs) +
               "&keterangan="   + keterangan;

  http.begin(client, url);
  int httpCode = http.GET();

  if (httpCode > 0) {
    Serial.println("        [OK] -> Google Sheets.");
  } else {
    Serial.printf("        [ERROR] %s\n", http.errorToString(httpCode).c_str());
  }
  http.end();
}
