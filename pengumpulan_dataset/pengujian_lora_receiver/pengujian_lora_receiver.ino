/******************************************************
 * RECEIVER NODE
 * ESP32 + LoRa SX1278 + WiFi + Google Sheets
 *
 * Menerima data LoRa
 * Menghitung Delay
 * Menghitung RSSI
 * Mengirim ke Google Sheets
 * Menyalakan buzzer saat paket diterima
 ******************************************************/

#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <HTTPClient.h>
//==================================================
// LoRa
//==================================================
#define LORA_SCK    18
#define LORA_MISO   19
#define LORA_MOSI   23
#define LORA_SS     5
#define LORA_RST    14
#define LORA_DIO0   2

#define LORA_FREQ 433E6
#define BUZZER_PIN 4
const char* ssid     = "Bos Dian";
const char* password = "pakenanya";

String GAS_URL =
"https://script.google.com/macros/s/XXXXXXXXXXXXXXXXXXXXXXXXXXXX/exec";
String receivedPacket;

String txMillis;

String soilAnalog;
String soilPercent;
String soilStatus;

String rainAnalog;
String rainPercent;
String rainStatus;

String accelMag;
String gyroMag;

String pitch;
String roll;

String vibStatus;



long delayTime;

int rssi;

float snr;
void setup()
{

  Serial.begin(115200);

  pinMode(BUZZER_PIN,OUTPUT);

  digitalWrite(BUZZER_PIN,LOW);



  //----------------------------------
  // WiFi
  //----------------------------------
  WiFi.begin(ssid,password);

  Serial.print("Connecting WiFi");

  while(WiFi.status()!=WL_CONNECTED)
  {

      delay(500);

      Serial.print(".");

  }

  Serial.println();

  Serial.println("WiFi Connected");



  //----------------------------------
  // LoRa
  //----------------------------------
  SPI.begin(
      LORA_SCK,
      LORA_MISO,
      LORA_MOSI,
      LORA_SS);

  LoRa.setPins(
      LORA_SS,
      LORA_RST,
      LORA_DIO0);

  if(!LoRa.begin(LORA_FREQ))
  {

      Serial.println("LoRa Failed");

      while(true);

  }

  Serial.println("LoRa Receiver Ready");

}
void beep()
{

    digitalWrite(BUZZER_PIN,HIGH);

    delay(200);

    digitalWrite(BUZZER_PIN,LOW);

}
void parsePacket(String data)
{

    int index=0;

    String item[12];



    while(data.length()>0)
    {

        int comma=data.indexOf(',');

        if(comma==-1)
        {

            item[index++]=data;

            break;

        }

        item[index++]=data.substring(0,comma);

        data=data.substring(comma+1);

    }



    txMillis=item[0];

    soilAnalog=item[1];

    soilPercent=item[2];

    soilStatus=item[3];

    rainAnalog=item[4];

    rainPercent=item[5];

    rainStatus=item[6];

    accelMag=item[7];

    gyroMag=item[8];

    pitch=item[9];

    roll=item[10];

    vibStatus=item[11];

}