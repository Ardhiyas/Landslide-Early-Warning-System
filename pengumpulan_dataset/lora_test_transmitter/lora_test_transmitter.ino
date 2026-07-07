/*
  =====================================================================
  TANAALERT - PENGUJIAN KOMUNIKASI LoRa: TRANSMITTER (RTT Piggyback)
  =====================================================================
  Metode: Round Trip Time dengan piggyback delay ke paket berikutnya

  Cara kerja:
  1. Kirim paket yang berisi [seq, lastDelay, data_sensor]
     -> lastDelay = delay hasil RTT dari paket sebelumnya
     -> paket pertama, lastDelay = 0 (belum ada data)
  2. Tunggu ACK dari receiver (maks ACK_TIMEOUT_MS)
  3. Jika ACK diterima: hitung RTT, simpan sebagai lastDelay
     untuk disisipkan ke paket berikutnya
  4. Jika ACK tidak diterima: lastDelay = -1 (paket hilang)
  5. Ulangi setiap SEND_INTERVAL_MS

  Keunggulan:
  - Transmitter bisa diletakkan di lapangan TANPA laptop
  - Receiver menampilkan delay, RSSI, dan status di Serial Monitor
  - Receiver mencatat semua ke Google Sheets
  - Tidak perlu sinkronisasi jam antar board

  WIRING (hanya LoRa, sensor lain tidak perlu):
  VCC->3.3V | GND->GND | RST->GPIO14 | DIO0->GPIO2
  NSS->GPIO5 | SCK->GPIO18 | MISO->GPIO19 | MOSI->GPIO23
  =====================================================================
*/

#include <SPI.h>
#include <LoRa.h>

#define LORA_NSS      5
#define LORA_RST      14
#define LORA_DIO0     2
#define LORA_SCK      18
#define LORA_MISO     19
#define LORA_MOSI     23

#define LORA_FREQUENCY   433E6
#define LORA_SYNC_WORD   0x12
#define SEND_INTERVAL_MS 5000UL
#define ACK_TIMEOUT_MS   3000UL
#define NODE_ID          "NODE-07"

unsigned long packetSeq  = 0;
long          lastDelay  = 0;   // delay RTT/2 dari paket sebelumnya (ms)
                                 // 0  = belum ada data (paket pertama)
                                 // -1 = paket sebelumnya tidak ada ACK
unsigned long lastSendTime = 0;

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("=== TANAALERT LoRa Tester TRANSMITTER (RTT Piggyback) ===");
  Serial.println("Transmitter berjalan mandiri, tidak perlu laptop.");
  Serial.println("Semua hasil delay bisa dibaca di Serial Monitor RECEIVER.\n");

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("[ERROR] LoRa gagal! Periksa wiring.");
    while (1) delay(1000);
  }
  LoRa.setSpreadingFactor(9);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setTxPower(17);
  LoRa.setSyncWord(LORA_SYNC_WORD);

  Serial.println("[OK] LoRa siap. Mulai mengirim...");
  lastSendTime = millis();
}

void loop() {
  if (millis() - lastSendTime >= SEND_INTERVAL_MS) {
    sendPacket();
    lastSendTime = millis();
  }
}

void sendPacket() {
  packetSeq++;

  // Format paket:
  // seq,lastDelay,NODE_ID,rainADC,rainPct,rainStatus,soilADC,soilPct,
  // soilStatus,accelMag,gyroMag,pitch,roll,vibStatus
  //
  // lastDelay disisipkan di field ke-2 supaya receiver bisa parsing
  // dan menampilkannya tanpa perlu laptop di sisi transmitter.
  // Data sensor pakai nilai dummy tetap (fokus pengujian adalah LoRa,
  // bukan akurasi sensor).
  String payload = String(packetSeq) + "," +
                   String(lastDelay) + "," +
                   String(NODE_ID) + "," +
                   "4095,0,Terang," +
                   "3281,19,Kering," +
                   "1.01,3.59,3.54,-6.84,STABIL";

  unsigned long tSend = millis();

  LoRa.beginPacket();
  LoRa.print(payload);
  LoRa.endPacket();

  // Tunggu ACK dari receiver
  unsigned long deadline = millis() + ACK_TIMEOUT_MS;
  bool ackOk = false;

  while (millis() < deadline) {
    if (LoRa.parsePacket()) {
      String ack = "";
      while (LoRa.available()) ack += (char)LoRa.read();

      if (ack == "ACK," + String(packetSeq)) {
        unsigned long rtt = millis() - tSend;
        lastDelay = (long)(rtt / 2);   // delay satu arah
        ackOk = true;
        break;
      }
    }
  }

  if (!ackOk) {
    lastDelay = -1;   // -1 = tidak ada ACK, receiver akan tahu paket ini hilang
    Serial.printf("[%3lu] Tidak ada ACK (paket mungkin hilang)\n", packetSeq);
  } else {
    Serial.printf("[%3lu] ACK diterima. Delay satu arah: %ld ms\n",
                  packetSeq, lastDelay);
  }
}
