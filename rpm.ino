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

void rpm_fun() {
  rpmcount++;
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
  // Bemenetnek állítjuk be a pint, amelyen érkezik a jel a megszakításhoz
  pinMode(RpmInputPin, INPUT);
  rpmcount = 0;
  rpm = 0;
  timeold = 0;
}

void loop() {
  int rpm = getRPM();
  Serial.println("RPM: "+(String)rpm);
}
