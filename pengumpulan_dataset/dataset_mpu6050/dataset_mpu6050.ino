#include <Wire.h>

#define MPU_ADDR 0x68

// Variabel sensor
int16_t accelX, accelY, accelZ;
int16_t gyroX, gyroY, gyroZ;

// Variabel hasil
float ax, ay, az;
float gx, gy, gz;

float accel_magnitude;
float gyro_magnitude;
float pitch, roll;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // SDA = 21, SCL = 22

  // Wake up MPU6050
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
}

void loop() {
  // Baca data dari MPU6050
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // mulai dari ACCEL_XOUT_H
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 14, true);

  accelX = Wire.read() << 8 | Wire.read();
  accelY = Wire.read() << 8 | Wire.read();
  accelZ = Wire.read() << 8 | Wire.read();

  Wire.read(); Wire.read(); // skip temperature

  gyroX = Wire.read() << 8 | Wire.read();
  gyroY = Wire.read() << 8 | Wire.read();
  gyroZ = Wire.read() << 8 | Wire.read();

  // Konversi ke satuan
  ax = accelX / 16384.0;
  ay = accelY / 16384.0;
  az = accelZ / 16384.0;

  gx = gyroX / 131.0;
  gy = gyroY / 131.0;
  gz = gyroZ / 131.0;

  // Hitung magnitude
  accel_magnitude = sqrt(ax * ax + ay * ay + az * az);
  gyro_magnitude  = sqrt(gx * gx + gy * gy + gz * gz);

  // Hitung pitch & roll
  pitch = atan2(ay, sqrt(ax * ax + az * az)) * 180 / PI;
  roll  = atan2(-ax, az) * 180 / PI;

  // Kategori getaran
  String kondisi;

  if (gyro_magnitude < 10) {
    kondisi = "Stabil";
  } 
  else if (gyro_magnitude < 50) {
    kondisi = "Getaran Ringan";
  } 
  else {
    kondisi = "Getaran Kuat";
  }

  // Tampilkan data
  Serial.print("Accel Mag: ");
  Serial.print(accel_magnitude);

  Serial.print(" | Gyro Mag: ");
  Serial.print(gyro_magnitude);

  Serial.print(" | Pitch: ");
  Serial.print(pitch);

  Serial.print(" | Roll: ");
  Serial.print(roll);

  Serial.print(" | Kondisi: ");
  Serial.println(kondisi);

  delay(1000);
}