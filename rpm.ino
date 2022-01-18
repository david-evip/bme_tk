#pragma region "includes"
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "credentials.h"
#pragma endregion

#pragma region "variables"
WiFiClient espClient;
PubSubClient client(espClient);

const char ssid[] = WIFI_SSID;
const char password[] = WIFI_PASSWORD;

const char mqtt_server[] = MQTT_SERVER;
const int mqtt_port = MQTT_PORT;
const char mqtt_user[] = MQTT_USER;
const char mqtt_password[] = MQTT_PASSWORD;

const char client_id[] = CLIENT_ID;

const int hallSensorPin = 2;
const unsigned long sampleTime = 1000;
const int maxRPM = 1260;
int rpmMaximum = 0;

const byte RpmInputPin = 5;

volatile byte rpmcount;
unsigned int rpm;
unsigned long timeold;

unsigned long first_interrupt=0;
unsigned long next_interrupt=0;
#pragma endregion

void reconnect() {
  while (!client.connected()) {
     Serial.print("Attempting MQTT connection ...");
     if (client.connect(client_id, mqtt_user, mqtt_password)) {
        Serial.println("Connected");
        client.subscribe("s/us");
      }
      else {
        Serial.print("Failed, rc= ");
        Serial.print(client.state());
        Serial.println("Try again in 5 seconds");
        delay(5000);
      } 
    }
}

void sendMessage(int rpm) {
  String temp = "200,RPM,rpm," + String(rpm);
  char payload[temp.length() + 1];
  temp.toCharArray(payload, temp.length() + 1);
  client.publish("s/us", payload);
  delay(100);
}

int getRPM() {
  int count = 0;
  boolean countFlag = LOW;
  unsigned long currentTime = 0;
  unsigned long startTime = millis();
  while (currentTime <= sampleTime)
  {
    if (digitalRead(RpmInputPin) == HIGH)
    {
      countFlag = HIGH;
    }
    if (digitalRead(RpmInputPin) == LOW && countFlag == HIGH)
    {
      count++;
      countFlag=LOW;
    }
    currentTime = millis() - startTime;
  }
  int countRpm = int(60000/float(sampleTime))*count;
  return countRpm/6;
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi ...");
  }
  Serial.println("Connected to WiFi network");

  client.setServer(mqtt_server,  mqtt_port);
  reconnect();
  
  // Bemenetnek állítjuk be a pint, amelyen érkezik a jel a megszakításhoz
  pinMode(RpmInputPin, INPUT);
  rpmcount = 0;
  rpm = 0;
  timeold = 0;
}

void loop() {
  client.loop();
  int rpm = getRPM();
  reconnect();
  sendMessage(rpm);
  Serial.println("RPM: "+(String)rpm);
}
