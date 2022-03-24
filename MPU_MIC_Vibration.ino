#include "mpu9250.h"
#include "arduinoFFT.h"
#include <PubSubClient.h>
#include <WiFiManager.h>
#include "credentials.h"

#define Mic_pin 36
arduinoFFT FFT = arduinoFFT();
arduinoFFT FFT_Audio = arduinoFFT();
/* Mpu9250 object, SPI bus, CS on pin 10 */
bfs::Mpu9250 imu(&SPI, 5);
WiFiClient espClient;
PubSubClient client(espClient);

float x_offset, y_offset, z_offset;

const uint16_t samples = 256; //This value MUST ALWAYS be a power of 2
const double samplingFrequency = 2000; //Hz, must be less than 10000 due to ADC

unsigned int sampling_period_us;
unsigned long microseconds;

double vReal[samples];
double vImag[samples];

double vReal_Audio[samples];
double vImag_Audio[samples];

char ssid[] = WIFI_SSID;
const char password[] = WIFI_PASSWORD;

const char mqtt_server[] = MQTT_SERVER;
const int mqtt_port = MQTT_PORT;
const char mqtt_user[] = MQTT_USER;
const char mqtt_password[] = MQTT_PASSWORD;

const char client_id[] = CLIENT_ID;

struct TOP10_Value{
  double value;
  int hz;
};
TOP10_Value Top10_MPU[10];
TOP10_Value Top10_MIC[10];

void getTop10(double * vData, TOP10_Value *List){
  TOP10_Value temp;
  int counter=0;
  for(int i=0; i < 10; i++){
    temp.value=0;
    for(int j=2; j < samples/2; j++){
      if(vData[j]>temp.value){
        temp.value=vData[j];
        temp.hz=((j * 1.0 * samplingFrequency) / samples);
        counter=j;
      }
    }
    vData[counter]=0;
    List[i]=temp;
  }
}

void set_offset(){
  float x=0;
  float y=0;
  float z=0;
  for(int i=0; i<10; i++){
    imu.Read();
    x+=imu.accel_x_mps2();
    y+=imu.accel_y_mps2();
    z+=imu.accel_z_mps2();
}
  x_offset=x/10;
  y_offset=y/10;
  z_offset=z/10;
}
void PrintVector(double *vData, uint16_t bufferSize)
{
  for (uint16_t i = 0; i < bufferSize; i++)
  {
    double abscissa = ((i * 1.0 * samplingFrequency) / samples);
    
    Serial.print(abscissa, 6);
    Serial.print("Hz");
    Serial.print(" ");
    Serial.println(vData[i], 4);
  }
  Serial.println();
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
        Serial.println("Try again in 2 seconds");
        delay(2000);
      } 
    }
}
void sendMessage(TOP10_Value *List, double majorpeak, String Name) {
  String temp = "";
  for (int i = 0; i < 10; i++) {
    temp = "200," + Name + ",Top" + String(i+1) + "Hz," + String(List[i].hz) + "\n" +
            "200," + Name + ",Top" + String(i+1) + "Value," + String(List[i].value) + "\n";
    char payload[temp.length() + 1];
    temp.toCharArray(payload, temp.length() + 1);
    client.publish("s/us", payload);
  }
  temp = "200," + Name + ",major_peak," + String(majorpeak);
  char payload[temp.length() + 1];
  temp.toCharArray(payload, temp.length() + 1);
  client.publish("s/us", payload);
}
void setup() {
  Serial.begin(115200);
  while(!Serial) {}
  SPI.begin();
  /* Initialize and configure IMU */
  if (!imu.Begin()) {
    Serial.println("Error initializing communication with IMU");
    while(1) {}
  }
  set_offset();
  sampling_period_us = round(1000000*(1.0/samplingFrequency));
  /*----------------------------------------------WIFI--------------------------------------------------*/
   WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
    
    //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wm;

    // reset settings - wipe stored credentials for testing
    // these are stored by the esp library
    //wm.resetSettings();

    // Automatically connect using saved credentials,
    // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
    // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
    // then goes into a blocking loop awaiting configuration and will return success result

    bool res;
    res = wm.autoConnect(ssid, password); // password protected ap

    if(!res) {
        Serial.println("Failed to connect");
           //ESP.restart();
    } 
    else {
        //if you get here you have connected to the WiFi    
        Serial.println("connected...yeey :)");
    }
    /*----------------------------------------------------------------------------------------------*/
    client.setServer(mqtt_server,  mqtt_port);
    reconnect();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  
  client.loop();
  
  microseconds = micros();
  if(imu.Read()){
  for(int i=0; i<samples; i++)
  {
      imu.Read();
      vReal[i] = (imu.accel_x_mps2()-x_offset)+(imu.accel_y_mps2()-y_offset)+(imu.accel_z_mps2()-z_offset);
      vImag[i] = 0;

      vReal_Audio[i] = analogRead(Mic_pin);
      vImag_Audio[i] = 0;
      while(micros() - microseconds < sampling_period_us){
        //empty loop
      }
      microseconds += sampling_period_us;
  }
 }
  FFT.Windowing(vReal, samples, FFT_WIN_TYP_HAMMING, FFT_FORWARD); /* Weigh data */
  FFT.Compute(vReal, vImag, samples, FFT_FORWARD); /* Compute FFT */
  FFT.ComplexToMagnitude(vReal, vImag, samples); /* Compute magnitudes */
  Serial.print("MPU_All hz: ");
  //PrintVector(vReal, (samples >> 1));
  double majorpeak_mpu = FFT.MajorPeak(vReal, samples, samplingFrequency);
  Serial.print(majorpeak_mpu, 0); 
  Serial.println(" Hz");//Print out what frequency is the most dominant.
  Serial.println("TOP10:");
  getTop10(vReal, Top10_MPU);
  for(int i=0; i<10; i++){
    Serial.println((String)Top10_MPU[i].hz +"Hz: "+ (String)Top10_MPU[i].value);
  } 

  FFT_Audio.Windowing(vReal_Audio, samples, FFT_WIN_TYP_HAMMING, FFT_FORWARD); /* Weigh data */
  FFT_Audio.Compute(vReal_Audio, vImag_Audio, samples, FFT_FORWARD); /* Compute FFT */
  FFT_Audio.ComplexToMagnitude(vReal_Audio, vImag_Audio, samples); /* Compute magnitudes */
  Serial.print("MIC_All hz: ");
  //PrintVector(vReal_Audio, (samples >> 1));
  double majorpeak_mic = FFT_Audio.MajorPeak(vReal_Audio, samples, samplingFrequency);
  Serial.print(majorpeak_mic, 0); 
  Serial.println(" Hz");//Print out what frequency is the most dominant.
  Serial.println("TOP10:");
  getTop10(vReal_Audio, Top10_MIC);
  for(int i=0; i<10; i++){
    Serial.println((String)Top10_MIC[i].hz +"Hz: "+ (String)Top10_MIC[i].value);
  }
  sendMessage(Top10_MPU,majorpeak_mpu,"MPUVibration");
  sendMessage(Top10_MIC,majorpeak_mic,"MICVibration");
  delay(5000);

}
