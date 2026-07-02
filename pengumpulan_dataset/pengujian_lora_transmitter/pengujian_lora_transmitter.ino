/****************************************************
 * TRANSMITTER NODE
 * ESP32 + LoRa SX1278 + MPU6050 +
 * Soil Moisture + Rain Sensor
 *
 * Pengiriman setiap 5 detik
 ****************************************************/

#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>



//====================================================
// LoRa Pin
//====================================================
#define LORA_SCK     18
#define LORA_MISO    19
#define LORA_MOSI    23
#define LORA_SS      5
#define LORA_RST     14
#define LORA_DIO0    2

#define LORA_FREQ 433E6



//====================================================
// Soil Moisture
//====================================================
#define SOIL_PIN 35



//====================================================
// Rain Sensor
//====================================================
#define RAIN_AO 34
#define RAIN_DO 26



//====================================================
// MPU6050
//====================================================
#define MPU_ADDR 0x68

#define ACCEL_SENSITIVITY 8192.0
#define GYRO_SENSITIVITY 65.5

#define GYRO_RINGAN_MIN 10.0
#define GYRO_KUAT_MIN 150.0



//====================================================
// Variabel Soil
//====================================================
int soilAnalog;
float soilPercent;
String soilStatus;



//====================================================
// Variabel Rain
//====================================================
int rainAnalog;
int rainDigital;

float rainPercent;
String rainStatus;



//====================================================
// Raw MPU
//====================================================
int16_t ax_raw;
int16_t ay_raw;
int16_t az_raw;

int16_t gx_raw;
int16_t gy_raw;
int16_t gz_raw;



//====================================================
// Hasil MPU
//====================================================
float accelMagnitude;
float gyroMagnitude;

float pitch;
float roll;

String vibStatus;



//====================================================
// Variabel Packet
//====================================================
String packetData;



unsigned long previousMillis = 0;

const long interval = 5000;
//====================================================
// Status Getaran
//====================================================
String classifyVibStatus(float gyroMag)
{

  if (gyroMag < GYRO_RINGAN_MIN)
    return "STABIL";

  if (gyroMag < GYRO_KUAT_MIN)
    return "GETARAN_RINGAN";

  return "GETARAN_KUAT";

}



//====================================================
// Status Soil
//====================================================
String classifySoil(float percent)
{

  if(percent<=34)
      return "Kering";

  if(percent<=68)
      return "Lembab";

  return "Basah";

}



//====================================================
// Status Rain
//====================================================
String classifyRain(float percent)
{

  if(percent<=33)
      return "Terang";

  if(percent<=68)
      return "Gerimis";

  return "Hujan";

}
void setup()
{

  Serial.begin(115200);

  Wire.begin(21,22);

  pinMode(RAIN_DO,INPUT);



  //-------------------------------------
  // Wake MPU6050
  //-------------------------------------
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);



  //-------------------------------------
  // Accel ±4g
  //-------------------------------------
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1C);
  Wire.write(0x08);
  Wire.endTransmission(true);



  //-------------------------------------
  // Gyro ±500 dps
  //-------------------------------------
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1B);
  Wire.write(0x08);
  Wire.endTransmission(true);



  //-------------------------------------
  // LoRa
  //-------------------------------------
  SPI.begin(LORA_SCK,LORA_MISO,LORA_MOSI,LORA_SS);

  LoRa.setPins(LORA_SS,LORA_RST,LORA_DIO0);

  Serial.println("LoRa Initializing...");

  if(!LoRa.begin(LORA_FREQ))
  {

      Serial.println("LoRa Failed");

      while(true);

  }

  Serial.println("LoRa Ready");

}
//====================================================
// Membaca Soil Moisture
//====================================================
void readSoil()
{
    soilAnalog = analogRead(SOIL_PIN);

    soilPercent = map(soilAnalog, 4095, 0, 0, 100);

    soilStatus = classifySoil(soilPercent);
}



//====================================================
// Membaca Rain Sensor
//====================================================
void readRain()
{
    rainAnalog = analogRead(RAIN_AO);

    rainDigital = digitalRead(RAIN_DO);

    rainPercent = map(rainAnalog, 4095, 0, 0, 100);

    rainStatus = classifyRain(rainPercent);
}



//====================================================
// Membaca MPU6050
//====================================================
void readMPU()
{

    Wire.beginTransmission(MPU_ADDR);

    Wire.write(0x3B);

    Wire.endTransmission(false);

    Wire.requestFrom(MPU_ADDR,14,true);



    ax_raw = Wire.read()<<8 | Wire.read();

    ay_raw = Wire.read()<<8 | Wire.read();

    az_raw = Wire.read()<<8 | Wire.read();



    Wire.read();
    Wire.read();



    gx_raw = Wire.read()<<8 | Wire.read();

    gy_raw = Wire.read()<<8 | Wire.read();

    gz_raw = Wire.read()<<8 | Wire.read();



    float ax = ax_raw / ACCEL_SENSITIVITY;

    float ay = ay_raw / ACCEL_SENSITIVITY;

    float az = az_raw / ACCEL_SENSITIVITY;



    float gx = gx_raw / GYRO_SENSITIVITY;

    float gy = gy_raw / GYRO_SENSITIVITY;

    float gz = gz_raw / GYRO_SENSITIVITY;



    accelMagnitude =
    sqrt(
    ax*ax+
    ay*ay+
    az*az
    );



    gyroMagnitude =
    sqrt(
    gx*gx+
    gy*gy+
    gz*gz
    );



    pitch =
    atan2(
    ay,
    sqrt(ax*ax+az*az)
    )*180.0/PI;



    roll =
    atan2(
    -ax,
    az
    )*180.0/PI;



    vibStatus =
    classifyVibStatus(gyroMagnitude);

}



//====================================================
// Membuat Paket
//====================================================
void createPacket()
{

    packetData = "";

    packetData += String(millis());

    packetData += ",";

    packetData += String(soilAnalog);

    packetData += ",";

    packetData += String(soilPercent,1);

    packetData += ",";

    packetData += soilStatus;

    packetData += ",";

    packetData += String(rainAnalog);

    packetData += ",";

    packetData += String(rainPercent,1);

    packetData += ",";

    packetData += rainStatus;

    packetData += ",";

    packetData += String(accelMagnitude,3);

    packetData += ",";

    packetData += String(gyroMagnitude,2);

    packetData += ",";

    packetData += String(pitch,2);

    packetData += ",";

    packetData += String(roll,2);

    packetData += ",";

    packetData += vibStatus;

}



//====================================================
// Mengirim Paket LoRa
//====================================================
void sendPacket()
{

    LoRa.beginPacket();

    LoRa.print(packetData);

    LoRa.endPacket();

}
void loop()
{

    unsigned long currentMillis = millis();

    if(currentMillis-previousMillis>=interval)
    {

        previousMillis=currentMillis;

        //----------------------------
        // Baca Sensor
        //----------------------------
        readSoil();

        readRain();

        readMPU();



        //----------------------------
        // Buat Paket
        //----------------------------
        createPacket();



        //----------------------------
        // Kirim LoRa
        //----------------------------
        sendPacket();



        //----------------------------
        // Serial Monitor
        //----------------------------
        Serial.println("--------------------------------------");

        Serial.println(packetData);

        Serial.println("--------------------------------------");

    }

}