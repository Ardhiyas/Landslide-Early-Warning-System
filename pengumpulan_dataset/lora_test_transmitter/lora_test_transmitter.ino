/*
  =====================================================================
  TANAALERT - PENGUJIAN KOMUNIKASI LoRa: TRANSMITTER
  =====================================================================
  Sketch ini KHUSUS untuk pengujian Tabel 3.11 (Pengujian Komunikasi
  LoRa) -- bukan transmitter utama TANAALERT. Setiap paket dikirim
  dengan nomor urut (seq) supaya receiver bisa menghitung packet loss,
  dan timestamp millis() untuk estimasi delay.

  Mengirim data setiap 5 detik, isi paket:
  seq, sendMillis, rainADC, rainPct, rainStatus, soilADC, soilPct,
  soilStatus, accelMag, gyroMag, pitch, roll, vibStatus

  WIRING: sama seperti transmitter_sensor_node.ino
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

#define LORA_FREQUENCY 433E6

#define SEND_INTERVAL_MS 5000UL

// ====================== KALIBRASI SENSOR ======================
#define SOIL_ADC_DRY   3400
#define SOIL_ADC_WET   1200
#define RAIN_ADC_DRY   4095
#define RAIN_ADC_WET   1100

#define RAIN_PCT_GERIMIS_MIN  33
#define RAIN_PCT_HUJAN_MIN    68
#define SOIL_PCT_LEMBAB_MIN   34
#define SOIL_PCT_BASAH_MIN    68
#define GYRO_RINGAN_MIN       10.0
#define GYRO_KUAT_MIN         150.0

// ====================== ALAMAT REGISTER MPU6050 ======================
#define MPU6050_ADDR             0x68
#define MPU6050_REG_PWR_MGMT_1   0x6B
#define MPU6050_REG_ACCEL_CONFIG 0x1C
#define MPU6050_REG_GYRO_CONFIG  0x1B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define ACCEL_SENSITIVITY  8192.0
#define GYRO_SENSITIVITY   65.5

// ====================== STATE ======================
unsigned long lastSendTime = 0;
unsigned long packetSeq = 0;   // nomor urut paket, reset ke 0 setiap board di-reboot

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("=== TANAALERT - Pengujian LoRa: TRANSMITTER ===");

  analogReadResolution(12);
  pinMode(PIN_RAIN_AO, INPUT);
  pinMode(PIN_SOIL_AO, INPUT);

  Wire.begin(21, 22);
  Wire.setClock(400000);
  setupMPU6050();

  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
  LoRa.setPins(LORA_NSS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("[ERROR] LoRa gagal diinisialisasi! Periksa wiring SPI.");
    while (1) delay(1000);
  }
  LoRa.setSpreadingFactor(9);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setTxPower(17);
  Serial.println("[OK] LoRa siap mengirim.");

  lastSendTime = millis();
}

void loop() {
  if (millis() - lastSendTime >= SEND_INTERVAL_MS) {
    sendTestPacket();
    lastSendTime = millis();
  }
}

void sendTestPacket() {
  packetSeq++;

  int rainADC = analogRead(PIN_RAIN_AO);
  int rainPct = adcToPercentInverse(rainADC, RAIN_ADC_DRY, RAIN_ADC_WET);
  String rainStatus = classifyRainStatus(rainPct);

  int soilADC = analogRead(PIN_SOIL_AO);
  int soilPct = adcToPercentInverse(soilADC, SOIL_ADC_DRY, SOIL_ADC_WET);
  String soilStatus = classifySoilStatus(soilPct);

  int16_t ax_raw, ay_raw, az_raw, gx_raw, gy_raw, gz_raw;
  readMPU6050Raw(ax_raw, ay_raw, az_raw, gx_raw, gy_raw, gz_raw);

  float ax_g = ax_raw / ACCEL_SENSITIVITY;
  float ay_g = ay_raw / ACCEL_SENSITIVITY;
  float az_g = az_raw / ACCEL_SENSITIVITY;
  float gx_dps = gx_raw / GYRO_SENSITIVITY;
  float gy_dps = gy_raw / GYRO_SENSITIVITY;
  float gz_dps = gz_raw / GYRO_SENSITIVITY;

  float accelMag = sqrt(ax_g * ax_g + ay_g * ay_g + az_g * az_g);
  float gyroMag  = sqrt(gx_dps * gx_dps + gy_dps * gy_dps + gz_dps * gz_dps);
  float pitch = atan2(ay_g, sqrt(ax_g * ax_g + az_g * az_g)) * 180.0 / PI;
  float roll  = atan2(-ax_g, az_g) * 180.0 / PI;
  String vibStatus = classifyVibStatus(gyroMag);

  String payload = String(packetSeq) + "," +
                    String(millis()) + "," +
                    rainADC + "," + rainPct + "," + rainStatus + "," +
                    soilADC + "," + soilPct + "," + soilStatus + "," +
                    String(accelMag, 2) + "," +
                    String(gyroMag, 2) + "," +
                    String(pitch, 2) + "," +
                    String(roll, 2) + "," +
                    vibStatus;

  LoRa.beginPacket();
  LoRa.print(payload);
  LoRa.endPacket();

  Serial.println("[KIRIM] Seq=" + String(packetSeq) + " -> " + payload);
}

// ====================== FUNGSI BANTUAN ======================
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

void setupMPU6050() {
  writeRegister(MPU6050_ADDR, MPU6050_REG_PWR_MGMT_1, 0x00);
  delay(100);
  writeRegister(MPU6050_ADDR, MPU6050_REG_ACCEL_CONFIG, 0x08);
  writeRegister(MPU6050_ADDR, MPU6050_REG_GYRO_CONFIG, 0x08);
  delay(50);

  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(0x75);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, 1, true);
  if (Wire.available()) {
    byte whoAmI = Wire.read();
    if (whoAmI == 0x68) {
      Serial.println("[OK] MPU6050 terdeteksi.");
    } else {
      Serial.printf("[WARNING] WHO_AM_I salah (0x%02X).\n", whoAmI);
    }
  } else {
    Serial.println("[ERROR] MPU6050 tidak merespon!");
  }
}

void writeRegister(uint8_t devAddr, uint8_t regAddr, uint8_t value) {
  Wire.beginTransmission(devAddr);
  Wire.write(regAddr);
  Wire.write(value);
  Wire.endTransmission(true);
}

void readMPU6050Raw(int16_t &ax, int16_t &ay, int16_t &az,
                    int16_t &gx, int16_t &gy, int16_t &gz) {
  Wire.beginTransmission(MPU6050_ADDR);
  Wire.write(MPU6050_REG_ACCEL_XOUT_H);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU6050_ADDR, 14, true);

  ax = (Wire.read() << 8) | Wire.read();
  ay = (Wire.read() << 8) | Wire.read();
  az = (Wire.read() << 8) | Wire.read();
  Wire.read(); Wire.read();
  gx = (Wire.read() << 8) | Wire.read();
  gy = (Wire.read() << 8) | Wire.read();
  gz = (Wire.read() << 8) | Wire.read();
}
