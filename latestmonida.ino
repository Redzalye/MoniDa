#include <Arduino.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <WiFi.h>
#include "ThingSpeak.h"

// WiFi and ThingSpeak settings
#define SECRET_SSID "alizh2509"
#define SECRET_PASS "Alza2509"
#define SECRET_CH_ID 2341962
#define SECRET_WRITE_APIKEY "NI3SULQI9QX3TB7T"

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int keyIndex = 0;
WiFiClient client;

unsigned long myChannelNumber = SECRET_CH_ID;
const char *myWriteAPIKey = SECRET_WRITE_APIKEY;
String myStatus = "";

// Pin definitions
#define MQ135PIN 33
SoftwareSerial pmsSerial(16, 17);

// Define the analog pins for the CO and CO2 sensors
const int coAnalogPin = 32;  // CO sensor analog pin

struct pms5003data {
  uint16_t framelen;
  uint16_t pm10_standard, pm25_standard, pm100_standard;
  uint16_t pm10_env, pm25_env, pm100_env;
  uint16_t particles_03um, particles_05um, particles_10um, particles_25um, particles_50um, particles_100um;
  uint16_t unused;
  uint16_t checksum;
};

struct pms5003data data;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    delay(10);
  Serial.println("AirDust Monitoring");

  delay(1000);

  pmsSerial.begin(9600);

  WiFi.mode(WIFI_STA);
  ThingSpeak.begin(client);

}

boolean readPMSdata(Stream *s) {
  if (!s->available()) {
    return false;
  }

  if (s->peek() != 0x42) {
    s->read();
    return false;
  }

  if (s->available() < 32) {
    return false;
  }

  uint8_t buffer[32];
  uint16_t sum = 0;
  s->readBytes(buffer, 32);

  for (uint8_t i = 0; i < 30; i++) {
    sum += buffer[i];
  }

  uint16_t buffer_u16[15];
  for (uint8_t i = 0; i < 15; i++) {
    buffer_u16[i] = buffer[2 + i * 2 + 1];
    buffer_u16[i] += (buffer[2 + i * 2] << 8);
  }

  memcpy((void *)&data, (void *)buffer_u16, 30);

  if (sum != data.checksum) {
    Serial.println("Checksum failure");
    return false;
  }
  return true;
}

void loop() {
  if (readPMSdata(&pmsSerial)) {
    Serial.println();
    Serial.println("---------------------------------------");
    Serial.println("Concentration Units (standard)");
    Serial.print("PM 1.0: "); Serial.print(data.pm10_standard);
    Serial.print("\t\tPM 2.5: "); Serial.print(data.pm25_standard);
    Serial.print("\t\tPM 10: "); Serial.println(data.pm100_standard);
    Serial.println("---------------------------------------");
    Serial.println("Concentration Units (environmental)");
    Serial.print("PM 1.0: "); Serial.print(data.pm10_env);
    Serial.print("\t\tPM 2.5: "); Serial.print(data.pm25_env);
    Serial.print("\t\tPM 10: "); Serial.println(data.pm100_env);
    Serial.println("---------------------------------------");
    Serial.print("Particles > 1.0um / 0.1L air:"); Serial.println(data.particles_10um);
    Serial.print("Particles > 2.5um / 0.1L air:"); Serial.println(data.particles_25um);
    Serial.print("Particles > 10.0 um / 0.1L air:"); Serial.println(data.particles_100um);
    Serial.println("---------------------------------------");

    // MQ7 data (CO)
    int coSensorValue = analogRead(coAnalogPin);
   float coVoltage = coSensorValue * (5.0 / 1023.0);
   float coPPM = 10 * ((coVoltage - 0.22) / 0.8);
   Serial.print("CO Concentration: ");
   Serial.print(coPPM);
   Serial.println(" ppm");

    // MQ135 data (CO2)
    int CO2Value = analogRead(MQ135PIN);
    Serial.print("CO2 Value: ");
    Serial.print(CO2Value);
    Serial.println(" ppm");

    // Connect or reconnect to WiFi
    if (WiFi.status() != WL_CONNECTED) {
      Serial.print("Attempting to connect to SSID: ");
      Serial.println(SECRET_SSID);
      while (WiFi.status() != WL_CONNECTED) {
        WiFi.begin(ssid, pass);
        Serial.print(".");
        delay(5000);
      }
      Serial.println("\nConnected.");
    }

    // Set the fields with the values
    int tempValue = 0; // Replace this with actual temperature value if available
    int humidityValue = 0; // Replace this with actual humidity value if available

    // Set the fields with the values
    ThingSpeak.setField(1, coPPM);
    ThingSpeak.setField(2, CO2Value);
    ThingSpeak.setField(3, data.pm10_standard);
    ThingSpeak.setField(4, data.pm25_standard);
    ThingSpeak.setField(5, data.pm100_standard);

    // Set the status
    ThingSpeak.setStatus(myStatus);

    // Write to the ThingSpeak channel
    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if (x == 200) {
      Serial.println("Channel update successful.");
    } else {
      Serial.println("Problem updating channel. HTTP error code " + String(x));
    }
    delay(15000);
  }
}
