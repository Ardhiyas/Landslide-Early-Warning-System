/*
  =====================================================================
  TANAALERT - DATA COLLECTOR: SENSOR MPU6050 (mode otomatis)
  =====================================================================
  Sketch ini mengirim data SECARA OTOMATIS dan TERUS-MENERUS tiap
  SEND_INTERVAL_MS, tanpa perlu menekan tombol setiap kali mau ambil
  satu sampel. MPU6050 dibaca manual via register I2C (TANPA library
  Adafruit_MPU6050), sama seperti pada transmitter utama.

  CARA PAKAI:
  1. Buka Serial Monitor (115200 baud).
  2. Ketik label sekali lalu Enter -> a = Aman | w = Waspada | b = Bahaya
  3. Sketch akan otomatis mengirim data setiap beberapa detik dengan
     label tersebut, terus-menerus, tanpa input lagi.
  4. Atur kondisi fisik sensor sesuai label yang sedang aktif (diam
     stabil / digoyang ringan / digoyang kuat / dimiringkan), biarkan
     mengirim beberapa puluh sampel.
  5. Saat mau pindah kondisi, ketik label baru kapan saja (tidak perlu
     berhenti) -> label langsung berubah untuk sampel-sampel berikutnya.

  WIRING:
  MPU6050      VCC -> 3.3V, GND -> GND, SDA -> GPIO21, SCL -> GPIO22
  =====================================================================
*/

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <math.h>

// ====================== KONFIGURASI WIFI ======================
#define WIFI_SSID      "NAMA_WIFI_ANDA"
#define WIFI_PASSWORD  "PASSWORD_WIFI_ANDA"

// ====================== KONFIGURASI GOOGLE SHEETS ======================
#define SCRIPT_URL "https://script.google.com/macros/s/XXXXXXXXXXXXXXXXXXXXXXXXXX/exec"

// ====================== ALAMAT REGISTER MPU6050 ======================
#define MPU6050_ADDR             0x68
#define MPU6050_REG_PWR_MGMT_1   0x6B
#define MPU6050_REG_ACCEL_CONFIG 0x1C
#define MPU6050_REG_GYRO_CONFIG  0x1B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B

#define ACCEL_SENSITIVITY  8192.0
#define GYRO_SENSITIVITY   65.5

#define SEND_INTERVAL_MS 1000UL   // getaran berubah cepat, interval lebih singkat dari sensor lain

// ====================== STATE ======================
String currentLabel = "";
unsigned long lastSendTime = 0;

void setup() {
  Serial.begin(115200);
  delay(500);

  Wire.begin(21, 22);
  Wire.setClock(400000);
  setupMPU6050();

  connectWiFi();

  Serial.println("\n=== TANAALERT Data Collector - MPU6050 (otomatis) ===");
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

// ====================== MPU6050: INIT MANUAL ======================
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

void collectAndSend(String label) {
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

  Serial.printf(
    "[KIRIM] accelMag=%.2fg  gyroMag=%.2fdps  pitch=%.2f  roll=%.2f  Label=%s\n",
    accelMag, gyroMag, pitch, roll, label.c_str()
  );

  sendToGoogleSheet(ax_g, ay_g, az_g, gx_dps, gy_dps, gz_dps,
                     accelMag, gyroMag, pitch, roll, label);
}

void sendToGoogleSheet(float ax, float ay, float az, float gx, float gy, float gz,
                        float accelMag, float gyroMag, float pitch, float roll,
                        String label) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[ERROR] WiFi tidak terhubung, data tidak terkirim.");
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;

  String url = String(SCRIPT_URL) +
               "?sheet=MPU6050" +
               "&ax=" + String(ax, 3) +
               "&ay=" + String(ay, 3) +
               "&az=" + String(az, 3) +
               "&gx=" + String(gx, 3) +
               "&gy=" + String(gy, 3) +
               "&gz=" + String(gz, 3) +
               "&accelMag=" + String(accelMag, 3) +
               "&gyroMag=" + String(gyroMag, 3) +
               "&pitch=" + String(pitch, 2) +
               "&roll=" + String(roll, 2) +
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
