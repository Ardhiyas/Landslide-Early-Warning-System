/*
  =====================================================================
  TANAALERT - EDGE GATEWAY (RECEIVER)
  Sistem Peringatan Dini Tanah Longsor Berbasis WSN + TinyML
  =====================================================================
  Board   : ESP32
  Modul   : LoRa SX1278, Buzzer aktif, WiFi (built-in)

  Tugas Gateway ini:
  1. Menerima paket data dari Sensor Node via LoRa
  2. Parsing paket CSV jadi nilai-nilai sensor
  3. Klasifikasi risiko longsor (AMAN / WASPADA / BAHAYA) menggunakan
     model Random Forest (9 pohon keputusan kecil hasil training,
     lihat rf_model.h) -- TANPA TensorFlow Lite Micro, karena ukuran
     dataset masih kecil (90 sampel), Random Forest ringan lebih
     cocok dan lebih mudah divalidasi.
  4. Menyalakan buzzer jika status BAHAYA
  5. Mengirim notifikasi ke Telegram jika status BAHAYA
  6. Mengirim seluruh data sensor + hasil klasifikasi ke Firebase
     Realtime Database, supaya bisa ditampilkan di website monitoring

  ---------------------------------------------------------------------
  WIRING:

  LoRa SX1278 (SPI)             Buzzer Aktif
  VCC  -> 3.3V                  VCC -> 3.3V (atau via transistor jika arus besar)
  GND  -> GND                   GND -> GND
  RST  -> GPIO14                SIG -> GPIO4
  DIO0 -> GPIO2
  NSS  -> GPIO5
  SCK  -> GPIO18
  MISO -> GPIO19
  MOSI -> GPIO23

  ---------------------------------------------------------------------
  LIBRARY YANG DIBUTUHKAN (install via Library Manager Arduino IDE):
  1. "LoRa" by Sandeep Mistry
  2. "Firebase ESP Client" by Mobizt        (untuk koneksi Firebase RTDB)
  3. "UniversalTelegramBot" by Brian Lough  (untuk notifikasi Telegram)
  4. "ArduinoJson" v6.x (dependency UniversalTelegramBot)

  File rf_model.h HARUS ada di folder yang sama dengan file .ino ini.
  =====================================================================
*/

#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include "rf_modell.h"

// ====================== KONFIGURASI WIFI ======================
#define WIFI_SSID      "Bos Dian"
#define WIFI_PASSWORD  "pakenanya"

// ====================== KONFIGURASI FIREBASE ======================
// Ambil dari Firebase Console -> Project Settings -> dan Realtime Database
#define FIREBASE_HOST       "https://dht11-19a61-default-rtdb.firebaseio.com/"
#define FIREBASE_API_KEY    "AIzaSyDBES0fKghLSVWoZSXTGxG1Q_uYLN-l6ZY"
// Gunakan email/password user yang sudah didaftarkan di Firebase Authentication
#define FIREBASE_USER_EMAIL    "landslide@gmail.com"
#define FIREBASE_USER_PASSWORD "landslide"

// Path database (disesuaikan dengan struktur yang dipakai dashboard website)
#define FIREBASE_PATH_LATEST  "/tanaalert/NODE-07/latest"
#define FIREBASE_PATH_LOG     "/tanaalert/NODE-07/log"

// ====================== KONFIGURASI TELEGRAM ======================
#define TELEGRAM_BOT_TOKEN  "8633664887:AAE73Edm6_lOlX6mISlCBTXHSrPmpBacWf8"
#define TELEGRAM_CHAT_ID    "1080840706"

// ====================== KONFIGURASI PIN ======================
#define LORA_NSS      5
#define LORA_RST      14
#define LORA_DIO0     2
#define LORA_SCK      18
#define LORA_MISO     19
#define LORA_MOSI     23
#define PIN_BUZZER    4

#define LORA_FREQUENCY 433E6   // HARUS sama dengan transmitter

// Notifikasi Telegram diulang setiap interval ini selama status tetap BAHAYA
// (supaya tidak diam jika kondisi bahaya berkepanjangan, tapi tidak spam tiap paket)
#define ALERT_REPEAT_INTERVAL_MS 5UL * 60UL * 1000UL   // 5 menit

// ====================== OBJEK GLOBAL ======================
WiFiClientSecure telegramClient;
UniversalTelegramBot bot(TELEGRAM_BOT_TOKEN, telegramClient);

FirebaseData fbdo;
FirebaseAuth fbAuth;
FirebaseConfig fbConfig;

String lastRiskStatus = "AMAN";
unsigned long lastAlertSentTime = 0;

// ====================== STRUKTUR PAKET DITERIMA ======================
struct ReceivedPacket {
  String nodeId;
  int    rainADC;
  int    rainPct;
  String rainStatus;
  int    soilADC;
  int    soilPct;
  String soilStatus;
  float  accelMag;
  float  gyroMag;
  float  pitch;
  float  roll;
  String vibStatus;
  bool   valid;
};

// ====================== SETUP ======================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("=== TANAALERT Edge Gateway (Receiver) ===");

  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  setupLoRa();
  setupWiFi();
  setupFirebase();

  telegramClient.setInsecure();  // skip sertifikat SSL (umum dipakai di ESP32 untuk Telegram)

  Serial.println("[OK] Edge Gateway siap menerima data.");
}

// ====================== LOOP ======================
void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize > 0) {
    String rawData = "";
    while (LoRa.available()) {
      rawData += (char)LoRa.read();
    }
    int rssi = LoRa.packetRssi();

    Serial.println("---- Paket LoRa Diterima ----");
    Serial.println("Raw   : " + rawData);
    Serial.println("RSSI  : " + String(rssi) + " dBm");

    ReceivedPacket pkt = parsePacket(rawData);

    if (pkt.valid) {
      processPacket(pkt, rssi);
    } else {
      Serial.println("[WARNING] Paket tidak valid / format salah, diabaikan.");
    }
    Serial.println("------------------------------\n");
  }

  // Pastikan WiFi tetap terhubung (auto-reconnect sederhana)
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WARNING] WiFi terputus, mencoba menyambung ulang...");
    setupWiFi();
  }
}

// ====================== INIT LORA ======================
void setupLoRa() {
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("[ERROR] Modul LoRa gagal diinisialisasi! Periksa wiring SPI.");
    while (1) delay(1000);
  }
  // HARUS sama dengan setting transmitter
  LoRa.setSpreadingFactor(9);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  Serial.println("[OK] LoRa siap menerima data.");
}

// ====================== INIT WIFI ======================
void setupWiFi() {
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
    Serial.println("\n[ERROR] Gagal menyambung WiFi, akan dicoba lagi di loop().");
  }
}

// ====================== INIT FIREBASE ======================
void setupFirebase() {
  fbConfig.api_key = FIREBASE_API_KEY;
  fbConfig.database_url = FIREBASE_HOST;

  fbAuth.user.email = FIREBASE_USER_EMAIL;
  fbAuth.user.password = FIREBASE_USER_PASSWORD;

  fbConfig.token_status_callback = tokenStatusCallback;  // dari addons/TokenHelper.h

  Firebase.begin(&fbConfig, &fbAuth);
  Firebase.reconnectWiFi(true);

  Serial.println("[OK] Firebase diinisialisasi.");
}

// ====================== PARSING PAKET CSV ======================
// Format: NODE_ID,rainADC,rainPct,rainStatus,soilADC,soilPct,soilStatus,
//         accelMag,gyroMag,pitch,roll,vibStatus
ReceivedPacket parsePacket(String raw) {
  ReceivedPacket pkt;
  pkt.valid = false;

  const int MAX_FIELDS = 12;
  String fields[MAX_FIELDS];
  int fieldCount = 0;
  int startIdx = 0;

  for (int i = 0; i <= raw.length(); i++) {
    if (i == raw.length() || raw[i] == ',') {
      if (fieldCount < MAX_FIELDS) {
        fields[fieldCount] = raw.substring(startIdx, i);
        fieldCount++;
      }
      startIdx = i + 1;
    }
  }

  if (fieldCount != MAX_FIELDS) {
    return pkt;  // pkt.valid tetap false
  }

  pkt.nodeId     = fields[0];
  pkt.rainADC    = fields[1].toInt();
  pkt.rainPct    = fields[2].toInt();
  pkt.rainStatus = fields[3];
  pkt.soilADC    = fields[4].toInt();
  pkt.soilPct    = fields[5].toInt();
  pkt.soilStatus = fields[6];
  pkt.accelMag   = fields[7].toFloat();
  pkt.gyroMag    = fields[8].toFloat();
  pkt.pitch      = fields[9].toFloat();
  pkt.roll       = fields[10].toFloat();
  pkt.vibStatus  = fields[11];
  pkt.valid      = true;

  return pkt;
}

// ====================== PROSES PAKET: KLASIFIKASI + AKSI ======================
void processPacket(ReceivedPacket &pkt, int rssi) {
  int riskClassIdx = classifyRiskRF(pkt.rainADC, pkt.rainPct, pkt.soilADC, pkt.soilPct,
                                     pkt.accelMag, pkt.gyroMag, pkt.pitch, pkt.roll);
  String riskStatus = riskClassToString(riskClassIdx);  // "AMAN" / "WASPADA" / "BAHAYA"

  Serial.println("Status Risiko : " + riskStatus);

  // --- Kontrol Buzzer ---
  if (riskStatus == "BAHAYA") {
    digitalWrite(PIN_BUZZER, HIGH);
  } else {
    digitalWrite(PIN_BUZZER, LOW);
  }

  // --- Kirim ke Firebase (untuk Website Monitoring) ---
  sendToFirebase(pkt, riskStatus, rssi);

  // --- Notifikasi Telegram jika BAHAYA ---
  handleTelegramAlert(pkt, riskStatus);

  lastRiskStatus = riskStatus;
}

// ====================== KIRIM DATA KE FIREBASE ======================
void sendToFirebase(ReceivedPacket &pkt, String riskStatus, int rssi) {
  if (WiFi.status() != WL_CONNECTED) return;

  FirebaseJson json;
  json.set("nodeId", pkt.nodeId);
  json.set("rainADC", pkt.rainADC);
  json.set("rainPct", pkt.rainPct);
  json.set("rainStatus", pkt.rainStatus);
  json.set("soilADC", pkt.soilADC);
  json.set("soilPct", pkt.soilPct);
  json.set("soilStatus", pkt.soilStatus);
  json.set("accelMagnitude", pkt.accelMag);
  json.set("gyroMagnitude", pkt.gyroMag);
  json.set("pitch", pkt.pitch);
  json.set("roll", pkt.roll);
  json.set("vibStatus", pkt.vibStatus);
  json.set("riskStatus", riskStatus);
  json.set("rssi", rssi);
  json.set("timestamp", (double)time(nullptr));   // pastikan NTP/time sudah di-set jika perlu format tanggal asli

  // Update data terbaru (dipakai dashboard utama / risk banner)
  if (Firebase.RTDB.setJSON(&fbdo, FIREBASE_PATH_LATEST, &json)) {
    Serial.println("[OK] Data terkirim ke Firebase (latest).");
  } else {
    Serial.println("[ERROR] Gagal kirim ke Firebase: " + fbdo.errorReason());
  }

  // Tambahkan juga ke log historis (dipakai grafik tren & tabel log di dashboard)
  if (Firebase.RTDB.pushJSON(&fbdo, FIREBASE_PATH_LOG, &json)) {
    Serial.println("[OK] Data ditambahkan ke log Firebase.");
  } else {
    Serial.println("[ERROR] Gagal menambah log Firebase: " + fbdo.errorReason());
  }
}

// ====================== NOTIFIKASI TELEGRAM ======================
void handleTelegramAlert(ReceivedPacket &pkt, String riskStatus) {
  if (WiFi.status() != WL_CONNECTED) return;

  bool justBecameDangerous = (riskStatus == "BAHAYA" && lastRiskStatus != "BAHAYA");
  bool stillDangerousAndTimeToRepeat =
      (riskStatus == "BAHAYA" &&
       (millis() - lastAlertSentTime >= ALERT_REPEAT_INTERVAL_MS));

  if (riskStatus == "BAHAYA" && (justBecameDangerous || stillDangerousAndTimeToRepeat)) {
    String message = buildAlertMessage(pkt, riskStatus);
    bool sent = bot.sendMessage(TELEGRAM_CHAT_ID, message, "");

    if (sent) {
      Serial.println("[OK] Notifikasi Telegram terkirim.");
      lastAlertSentTime = millis();
    } else {
      Serial.println("[ERROR] Gagal mengirim notifikasi Telegram.");
    }
  }
}

String buildAlertMessage(ReceivedPacket &pkt, String riskStatus) {
  String msg = "PERINGATAN BAHAYA TANAH LONGSOR\n";
  msg += "Node     : " + pkt.nodeId + "\n";
  msg += "Status   : " + riskStatus + "\n";
  msg += "------------------------------\n";
  msg += "Hujan    : " + String(pkt.rainPct) + "% (" + pkt.rainStatus + ")\n";
  msg += "Tanah    : " + String(pkt.soilPct) + "% (" + pkt.soilStatus + ")\n";
  msg += "Getaran  : " + String(pkt.gyroMag, 1) + " dps (" + pkt.vibStatus + ")\n";
  msg += "Pitch    : " + String(pkt.pitch, 1) + " derajat\n";
  msg += "Roll     : " + String(pkt.roll, 1) + " derajat\n";
  msg += "------------------------------\n";
  msg += "Segera lakukan pengecekan lapangan dan ikuti prosedur evakuasi jika diperlukan.";
  return msg;
}
