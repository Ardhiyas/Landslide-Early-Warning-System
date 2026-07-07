/*
  =====================================================================
  TANAALERT - SENSOR NODE (TRANSMITTER)  -- Single Node Version
  Sistem Peringatan Dini Tanah Longsor Berbasis WSN + TinyML
  =====================================================================
  Board   : ESP32
  Modul   : LoRa SX1278, Soil Moisture Sensor v2, Rain Sensor MH-RD,
            IMU MPU6050 (dibaca manual via register I2C, TANPA library
            Adafruit_MPU6050 - hanya pakai Wire.h bawaan Arduino)

  Tugas Node ini:
  1. Membaca sensor kelembapan tanah (ADC) -> dikonversi ke %
  2. Membaca sensor hujan (ADC)            -> dikonversi ke %
  3. Membaca MPU6050 langsung dari register I2C (accel & gyro 3 sumbu)
     -> dihitung magnitude, pitch, dan roll
  4. Mengemas semua nilai jadi satu paket CSV ringkas
  5. Mengirim paket via LoRa ke Edge Gateway (Receiver) setiap
     SEND_INTERVAL_MS

  ---------------------------------------------------------------------
  WIRING:

  Sensor Hujan (MH-RD)        Sensor Soil Moisture v2
  VCC  -> 3.3V                 VCC -> 3.3V
  GND  -> GND                  GND -> GND
  AO   -> GPIO34                AO -> GPIO35

  MPU6050 (I2C)                LoRa SX1278 (SPI)
  VCC -> 3.3V                  VCC  -> 3.3V
  GND -> GND                   GND  -> GND
  SDA -> GPIO21                RST  -> GPIO14
  SCL -> GPIO22                DIO0 -> GPIO2
                                NSS  -> GPIO5
                                SCK  -> GPIO18
                                MISO -> GPIO19
                                MOSI -> GPIO23

  ---------------------------------------------------------------------
  LIBRARY YANG DIBUTUHKAN:
  1. "LoRa" by Sandeep Mistry  (untuk modul SX1278)
  -> MPU6050 TIDAK butuh library tambahan, hanya Wire.h (sudah bawaan
     Arduino core ESP32).
  =====================================================================
*/

#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <math.h>

// ====================== KONFIGURASI PIN ======================
#define PIN_RAIN_AO   34
#define PIN_SOIL_AO   35

#define LORA_NSS      5
#define LORA_RST      14
#define LORA_DIO0     2
#define LORA_SCK      18
#define LORA_MISO     19
#define LORA_MOSI     23

// Frekuensi LoRa: sesuaikan dengan regulasi ISM band di Indonesia
// (433 MHz atau 920-923 MHz).
#define LORA_FREQUENCY 433E6

// ====================== KONFIGURASI NODE ======================
// Karena hanya 1 sensor node, NODE_ID tetap dicantumkan di paket
// supaya format tetap konsisten dan mudah dikembangkan ke multi-node
// di kemudian hari tanpa mengubah struktur paket / kode receiver.
#define NODE_ID          "NODE-07"
#define SEND_INTERVAL_MS 10000UL   // kirim data setiap 5 detik

// ====================== ALAMAT REGISTER MPU6050 ======================
#define MPU6050_ADDR             0x68   // alamat default (AD0 -> GND)
#define MPU6050_REG_PWR_MGMT_1   0x6B
#define MPU6050_REG_ACCEL_CONFIG 0x1C
#define MPU6050_REG_GYRO_CONFIG  0x1B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B  // accel & gyro dibaca berurutan dari sini

// Skala sesuai konfigurasi range di bawah (lihat setupMPU6050())
// Accel range +-4g       -> sensitivity 8192 LSB/g
// Gyro  range +-500 dps  -> sensitivity 65.5 LSB/(deg/s)
#define ACCEL_SENSITIVITY  8192.0
#define GYRO_SENSITIVITY   65.5

// ====================== KALIBRASI SENSOR ======================
// WAJIB dikalibrasi ulang sesuai sensor & tanah lokasi pemasangan NODE-07.
#define SOIL_ADC_DRY   3400   // ADC saat tanah benar-benar kering
#define SOIL_ADC_WET   1200   // ADC saat tanah benar-benar jenuh air

#define RAIN_ADC_DRY   4095   // ADC saat sensor benar-benar kering (tidak hujan)
#define RAIN_ADC_WET   1100   // ADC saat sensor basah penuh (hujan deras)

// Batas klasifikasi status tampilan (boleh disesuaikan)
#define RAIN_PCT_GERIMIS_MIN  33
#define RAIN_PCT_HUJAN_MIN    68
#define SOIL_PCT_LEMBAB_MIN   34
#define SOIL_PCT_BASAH_MIN    68
#define GYRO_RINGAN_MIN       10.0   // dps, batas getaran ringan
#define GYRO_KUAT_MIN         150.0  // dps, batas getaran kuat

// ====================== OBJEK GLOBAL ======================
unsigned long lastSendTime = 0;

// ====================== STRUKTUR DATA PAKET ======================
struct SensorPacket {
  int   rainADC;
  int   rainPct;
  String rainStatus;
  int   soilADC;
  int   soilPct;
  String soilStatus;
  float accelMagnitude;
  float gyroMagnitude;
  float pitch;
  float roll;
  String vibStatus;
};

// ====================== SETUP ======================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("=== TANAALERT Sensor Node (Transmitter) ===");

  // --- Init ADC pin sensor analog ---
  analogReadResolution(12);   // ESP32 ADC 12-bit (0-4095)
  pinMode(PIN_RAIN_AO, INPUT);
  pinMode(PIN_SOIL_AO, INPUT);

  // --- Init I2C & MPU6050 (manual, tanpa library) ---
  Wire.begin(21, 22);  // SDA, SCL
  Wire.setClock(400000);
  setupMPU6050();

  // --- Init LoRa SX1278 ---
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("[ERROR] Modul LoRa gagal diinisialisasi! Periksa wiring SPI.");
    while (1) delay(1000);
  }
  // Opsional: tuning parameter LoRa (HARUS sama dengan receiver!)
  LoRa.setSpreadingFactor(9);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setTxPower(17);
  Serial.println("[OK] LoRa siap mengirim data.");

  lastSendTime = millis();
}

// ====================== LOOP ======================
void loop() {
  if (millis() - lastSendTime >= SEND_INTERVAL_MS) {
    SensorPacket pkt = readAllSensors();
    sendPacketViaLoRa(pkt);
    printPacketToSerial(pkt);
    lastSendTime = millis();
  }
}

// ====================== MPU6050: INIT MANUAL ======================
void setupMPU6050() {
  // Wake up sensor (keluar dari sleep mode bawaan)
  writeRegister(MPU6050_ADDR, MPU6050_REG_PWR_MGMT_1, 0x00);
  delay(100);

  // Set accel range ke +-4g  -> nilai 0x08 di bit [4:3]
  writeRegister(MPU6050_ADDR, MPU6050_REG_ACCEL_CONFIG, 0x08);

  // Set gyro range ke +-500 dps -> nilai 0x08 di bit [4:3]
  writeRegister(MPU6050_ADDR, MPU6050_REG_GYRO_CONFIG, 0x08);

  delay(50);

  // Cek koneksi dengan membaca WHO_AM_I (register 0x75, harus 0x68)
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x75);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, 1, true);
  if (Wire.available()) {
    byte whoAmI = Wire.read();
    if (whoAmI == 0x68) {
      Serial.println("[OK] MPU6050 terdeteksi (WHO_AM_I = 0x68).");
    } else {
      Serial.printf("[WARNING] WHO_AM_I tidak sesuai (0x%02X). Cek wiring I2C.\n", whoAmI);
    }
  } else {
    Serial.println("[ERROR] MPU6050 tidak merespon! Periksa wiring SDA/SCL.");
  }
}

void writeRegister(uint8_t devAddr, uint8_t regAddr, uint8_t value) {
  Wire.beginTransmission(devAddr);
  Wire.write(regAddr);
  Wire.write(value);
  Wire.endTransmission(true);
}

// ====================== MPU6050: BACA RAW DATA ======================
// Membaca 14 byte berurutan mulai dari ACCEL_XOUT_H:
// AX,AY,AZ,TEMP,GX,GY,GZ (masing-masing 2 byte, big-endian)
void readMPU6050Raw(int16_t &ax, int16_t &ay, int16_t &az,
                    int16_t &gx, int16_t &gy, int16_t &gz) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(MPU6050_REG_ACCEL_XOUT_H);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, 14, true);

  ax = (Wire.read() << 8) | Wire.read();
  ay = (Wire.read() << 8) | Wire.read();
  az = (Wire.read() << 8) | Wire.read();
  Wire.read(); Wire.read();          // lewati 2 byte data temperatur
  gx = (Wire.read() << 8) | Wire.read();
  gy = (Wire.read() << 8) | Wire.read();
  gz = (Wire.read() << 8) | Wire.read();
}

// ====================== BACA SEMUA SENSOR ======================
SensorPacket readAllSensors() {
  SensorPacket pkt;

  // --- Sensor Hujan ---
  pkt.rainADC = analogRead(PIN_RAIN_AO);
  pkt.rainPct = adcToPercentInverse(pkt.rainADC, RAIN_ADC_DRY, RAIN_ADC_WET);
  pkt.rainStatus = classifyRainStatus(pkt.rainPct);

  // --- Sensor Soil Moisture ---
  pkt.soilADC = analogRead(PIN_SOIL_AO);
  pkt.soilPct = adcToPercentInverse(pkt.soilADC, SOIL_ADC_DRY, SOIL_ADC_WET);
  pkt.soilStatus = classifySoilStatus(pkt.soilPct);

  // --- Sensor MPU6050 (manual I2C) ---
  int16_t ax_raw, ay_raw, az_raw, gx_raw, gy_raw, gz_raw;
  readMPU6050Raw(ax_raw, ay_raw, az_raw, gx_raw, gy_raw, gz_raw);

  // Konversi raw -> satuan g (accel) dan dps (gyro)
  float ax_g = ax_raw / ACCEL_SENSITIVITY;
  float ay_g = ay_raw / ACCEL_SENSITIVITY;
  float az_g = az_raw / ACCEL_SENSITIVITY;

  float gx_dps = gx_raw / GYRO_SENSITIVITY;
  float gy_dps = gy_raw / GYRO_SENSITIVITY;
  float gz_dps = gz_raw / GYRO_SENSITIVITY;

  pkt.accelMagnitude = sqrt(ax_g * ax_g + ay_g * ay_g + az_g * az_g);
  pkt.gyroMagnitude  = sqrt(gx_dps * gx_dps + gy_dps * gy_dps + gz_dps * gz_dps);

  // Pitch & Roll dihitung dari accelerometer (derajat)
  pkt.pitch = atan2(ay_g, sqrt(ax_g * ax_g + az_g * az_g)) * 180.0 / PI;
  pkt.roll  = atan2(-ax_g, az_g) * 180.0 / PI;

  pkt.vibStatus = classifyVibStatus(pkt.gyroMagnitude);

  return pkt;
}

// ====================== FUNGSI KONVERSI & KLASIFIKASI ======================

// Konversi ADC ke persen, dengan arah terbalik:
// ADC tinggi = kering/tidak hujan (0%), ADC rendah = basah/hujan deras (100%)
int adcToPercentInverse(int adcValue, int adcDry, int adcWet) {
  int pct = map(adcValue, adcDry, adcWet, 0, 100);
  return constrain(pct, 0, 100);
}

String classifyRainStatus(int rainPct) {
  if (rainPct < RAIN_PCT_GERIMIS_MIN) return "Terang";
  if (rainPct < RAIN_PCT_HUJAN_MIN)   return "Gerimis";
  return "Hujan";
}

String classifySoilStatus(int soilPct) {
  if (soilPct < SOIL_PCT_LEMBAB_MIN) return "Kering";
  if (soilPct < SOIL_PCT_BASAH_MIN)  return "Lembab";
  return "Basah";
}

String classifyVibStatus(float gyroMag) {
  if (gyroMag < GYRO_RINGAN_MIN)  return "STABIL";
  if (gyroMag < GYRO_KUAT_MIN)    return "GETARAN_RINGAN";
  return "GETARAN_KUAT";
}

// ====================== KIRIM PAKET VIA LORA ======================
// Format paket CSV (ringkas, hemat airtime LoRa):
// NODE_ID,rainADC,rainPct,rainStatus,soilADC,soilPct,soilStatus,
// accelMag,gyroMag,pitch,roll,vibStatus
void sendPacketViaLoRa(SensorPacket &pkt) {
  String payload = String(NODE_ID) + "," +
                   pkt.rainADC + "," + pkt.rainPct + "," + pkt.rainStatus + "," +
                   pkt.soilADC + "," + pkt.soilPct + "," + pkt.soilStatus + "," +
                   String(pkt.accelMagnitude, 2) + "," +
                   String(pkt.gyroMagnitude, 2) + "," +
                   String(pkt.pitch, 2) + "," +
                   String(pkt.roll, 2) + "," +
                   pkt.vibStatus;

  LoRa.beginPacket();
  LoRa.print(payload);
  LoRa.endPacket();
}

// ====================== DEBUG SERIAL MONITOR ======================
void printPacketToSerial(SensorPacket &pkt) {
  Serial.println("---- Paket Terkirim ----");
  Serial.printf("Rain  : ADC=%d  Pct=%d%%  Status=%s\n",
                 pkt.rainADC, pkt.rainPct, pkt.rainStatus.c_str());
  Serial.printf("Soil  : ADC=%d  Pct=%d%%  Status=%s\n",
                 pkt.soilADC, pkt.soilPct, pkt.soilStatus.c_str());
  Serial.printf("IMU   : AccelMag=%.2f g  GyroMag=%.2f dps  Pitch=%.2f  Roll=%.2f  Status=%s\n",
                 pkt.accelMagnitude, pkt.gyroMagnitude, pkt.pitch, pkt.roll, pkt.vibStatus.c_str());
  Serial.println("------------------------\n");
}
