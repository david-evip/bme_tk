#pragma region "includes"
#include <MPU9250_asukiaaa.h>
#include "arduinoFFT.h"
#pragma endregion

#pragma region "defines"
#define SAMPLES 128
#define SAMPLING_FREQUENCY 8000
#pragma endregion

#pragma region "variables"
MPU9250_asukiaaa mySensor;
arduinoFFT FFT = arduinoFFT();

float aX, aY, aZ;
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

void setup() {
  Serial.begin(115200);
  
  Wire.begin();
  mySensor.setWire(&Wire);
  mySensor.beginAccel();
}

void loop() {
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
    //Serial.println(String(vRealAcc[i]));
    } else {
      vRealAcc[i] = 0.0;
    }
    while(micros() < (microSeconds + samplingPeriod)) {
        //do nothing
      }
  }
  double VibrationMajorPeak = fft(vRealAcc, vImagAcc);
  double AudioMajorPeak = fft(vRealAudio, vImagAudio);
  
  for (int i = 1; i < SAMPLES/2; i++) {
    vRealAcc[i] = vRealAcc[i] / 8;
    Serial.print((String)((i * 1.0 * SAMPLING_FREQUENCY) / SAMPLES)+"Hz ");
    Serial.println(vRealAcc[i], 2);
  }
  Serial.println("Vibration major peak: "+String(VibrationMajorPeak/8)+" Hz");
  Serial.println("Audio major peak: "+String(AudioMajorPeak/8)+"Hz");
  delay(3000);
}
