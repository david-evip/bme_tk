#pragma region "includes"
#include "Adafruit_MLX90614.h"
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include "credentials.h"
#pragma ends

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
int analogPin = A0; // Analóg bemenet
int max_val;
int new_val;
int old_val = 540;
float IRMS;
bool meres_kezdheto = false;
int digitPin1 = D1;
int digitPin2 = D2;
//Hőmérőszenzor
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
long temptime;
long timer;
long timer2;
#pragma end

void connectToMQTT() {
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

void sendTemp() {
  double temperature = mlx.readObjectTempC();
  String temp = "200,Temperature,temperature," + (String)temperature;
  char payload[temp.length() + 1];
  temp.toCharArray(payload, temp.length() + 1);
  client.publish("s/us", payload);
}

void sendIRMS(float IRMS) {
  int watt = 240*IRMS;
  String temp = "200,Ammeter,IRMS," + String(IRMS) + "\n200,Ammeter,Watt," + (String)watt;
  char payload[temp.length() + 1];
  temp.toCharArray(payload, temp.length() + 1);
  client.publish("s/us", payload);
  delay(100);
}

// Az alábbi függvény megvizsgálja, hogy pozitív félhullámot mintavételezünk-e
bool emelkedik() {
  bool emelkedes;
  analogReference(EXTERNAL);
  int new_val1 = analogRead(analogPin);
  delayMicroseconds(50);
  analogReference(EXTERNAL);
  int new_val2 = analogRead(analogPin);
  if(new_val2 > new_val1 && new_val2 > 552){
    emelkedes =true;
  }
  else {
    emelkedes =false;
  }
  return emelkedes;
}
void Tempprint(){
  Serial.print("AmbientTemp = "); 
  Serial.print(mlx.readAmbientTempC());
  Serial.println("*C");
  Serial.print("ObjectTemp = "); 
  Serial.print(mlx.readObjectTempC());
  Serial.println("*C");
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
  connectToMQTT();
  
  pinMode(A0,INPUT);
  pinMode(D1,INPUT);
  pinMode(D2,INPUT);
  mlx.begin();
  temptime=millis();
  timer=millis();
  timer2=millis();
}

// Megnézzük, hogy pozitív félhullámot vizsgálunk-e, és megkeressük a maximum értéket
// Amint megvan a maximum érték, kiértékeljük a mért eredményt, és kiíratjuk.
void loop() {
  if(millis()-temptime > 1000){
    temptime = millis();
  }
  meres_kezdheto = emelkedik();
  while(meres_kezdheto){
  analogReference(EXTERNAL);
  new_val = analogRead(analogPin);
  if(new_val > old_val){
    old_val = new_val;
  }
  else {
    delayMicroseconds(50);
    analogReference(EXTERNAL);
    new_val =analogRead(analogPin);
    if(new_val < old_val) {
      max_val =old_val;
      IRMS =(((max_val*4.79/1024)-2.395)/200)*1000/1.41;
      old_val =512;
      meres_kezdheto =false;
     }
   }
 }
 if(millis()-timer>1000){
  timer=millis();
  Serial.print("IRMS: ");
  Serial.println((String)IRMS+"A");
  Serial.print("PowerConsumption: ");
  Serial.println((String)(240*IRMS)+"W");
  sendIRMS(IRMS);
 }
 if(millis()-timer2>5000){
  timer2=millis();
  Tempprint();
  sendTemp();
 }
}
