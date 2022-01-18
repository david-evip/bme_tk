#pragma region "includes"
#include <MPU9250_asukiaaa.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include "arduinoFFT.h"
#include "credentials.h"
#pragma endregion

#pragma region "defines"
#define SAMPLES 128
#define SAMPLING_FREQUENCY 8000
#pragma endregion

#pragma region "variables"
MPU9250_asukiaaa mySensor;
arduinoFFT FFT = arduinoFFT();
WiFiClient espClient;
PubSubClient client(espClient);

const char ssid[] = WIFI_SSID;
const char password[] = WIFI_PASSWORD;

const char mqtt_server[] = MQTT_SERVER;
const int mqtt_port = MQTT_PORT;
const char mqtt_user[] = MQTT_USER;
const char mqtt_password[] = MQTT_PASSWORD;

const char client_id[] = CLIENT_ID;

double vRealAcc[SAMPLES];
double vImagAcc[SAMPLES];

double vRealAudio[SAMPLES];
double vImagAudio[SAMPLES];

unsigned int samplingPeriod;
unsigned long microSeconds;
#pragma endregion

double fft(double *vReal, double *vImag) {
  FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);
  FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);
  
  return FFT.MajorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY);
}

void sendMessage(double AudioMajorPeak, double VibrationMajorPeak) {
  String temp = "200,MajorPeaks,microphone," + String(AudioMajorPeak/8,2) + "\n200,MajorPeaks,accelerometer," + String(VibrationMajorPeak/8, 2);
  char payload[temp.length() + 1];
  temp.toCharArray(payload, temp.length() + 1);
  client.publish("s/us", payload);
  delay(1000);
}

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
  
  Wire.begin();
  mySensor.setWire(&Wire);
  mySensor.beginAccel();
}

void loop() {
  client.loop();
  Serial.flush();
  samplingPeriod = round(1000000*(1.0/SAMPLING_FREQUENCY));
  long startTime = micros();

  for (int i = 0; i < SAMPLES; i++) {
    microSeconds = micros();
    vImagAcc[i] = 0.0;
    vImagAudio[i] = 0.0;

    vRealAudio[i] = analogRead(36);

    if (mySensor.accelUpdate() == 0) {
    vRealAcc[i] = mySensor.accelX() + mySensor.accelY() + mySensor.accelZ();
    } else {
      vRealAcc[i] = 0.0;
    }
    while(micros() < (microSeconds + samplingPeriod)) {
        //do nothing
      }
  }
  double VibrationMajorPeak = fft(vRealAcc, vImagAcc);
  double AudioMajorPeak = fft(vRealAudio, vImagAudio);
  
  // Print out how dominant each frequency (vibration) 
  for (int i = 1; i < SAMPLES/2; i++) {
    vRealAcc[i] = vRealAcc[i] / 8;
    Serial.print((String)((i * 1.0 * SAMPLING_FREQUENCY) / SAMPLES)+"Hz ");
    Serial.println(vRealAcc[i], 2);
  }
  
  Serial.println("Vibration major peak: "+String(VibrationMajorPeak/8)+" Hz");
  Serial.println("Audio major peak: "+String(AudioMajorPeak/8)+"Hz");
  reconnect();
  sendMessage(AudioMajorPeak, VibrationMajorPeak);
  delay(3000);
}
