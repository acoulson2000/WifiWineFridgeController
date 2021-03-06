#include <SoftwareSerial.h>
#include <math.h>

#define NUMSAMPLES 10

// PWM_BASE been tuned to provide a base level of cooling which seems necessary to maintain a
// constant temperature when set to a reasonable level and external ambient temp is room temperature.
// PWM_INCREMENT is multplied by the diff between current temp and target setpoint, then added
// to PWM_BASE - so if you have a difference >= 3.4°, PWM will be running at max (255). This will
// gradually reduce as the temp difference decrease, down to a minimum of PWM_MIN.
#define PWM_BASE 150
#define PWM_INCREMENT 50
#define PWM_MIN 100         // Set to minimum PWM duty cycle - Don't make it too low, since we want the power supply fan to run at a minimum level
    
#define CW   1
#define CCW  2
#define CS_THRESHOLD 100
  
// We use serial2 to communicate with the head unit. It requires the SoftwareSerial lib and uses any two
// available pins.
SoftwareSerial serial2(10, 11); // RX, TX

int tempSetPoint = 60;
int temp = 60;
float movingAverage;
int samples[NUMSAMPLES];

/*  VNH2SP30 (Monster Moto shield) pin definitions
 xxx[0] controls '1' outputs
 xxx[1] controls '2' outputs */
int inApin[2] = {7, 4};  // INA: Clockwise input
int inBpin[2] = {8, 9}; // INB: Counter-clockwise input
int pwmpin[2] = {5, 6}; // PWM input

int THERMISTOR_PIN  = A0;                 // Analog Pin 0

int statpin = 13;

int pwm = 0;
int stat = LOW;
byte ack = 0;

// Sample code cribbed from interwebs didn't seem to produce legit results - possibly since
// I don't know what device is used as the sensor in the wine cooler. So, instead, I took
// measurements of ADC values and cross-referenced them to measured actual temps in a 
// trend-line in Excel. That gav eme a very good linear trend line, and I can use the
// slope of it to calculate reasonable temps. The values in this function are based on
// the slope of that line.
float trendBaseADC = 830; // My trend line started at ADC 830
float trendBaseTemp = 60; // My trend line started at ADC 830
float baseAdcDelta = 43.5; // An ADC delta of -43.5 resulted in a temp decrease of 12.2
float baseTempDelta = 12.2;

float Thermistor(int RawADC) {
  float calulatedTemp = trendBaseTemp - ((RawADC - trendBaseADC) / baseAdcDelta * baseTempDelta);  
  //Serial.print("RawADC: "); 
  //Serial.print(RawADC); 
  //Serial.print("calulatedTemp: "); 
  //Serial.print(calulatedTemp); 
  //Serial.println(""); 
  return calulatedTemp;                                      // Return the Temperature
}

void setup() {
  Serial.begin(115200);
  serial2.begin(115200);

  analogReference(DEFAULT);
  
  pinMode(statpin, OUTPUT);
 
  // Initialize digital pins as outputs
  for (int i=0; i<2; i++)
  {
    pinMode(inApin[i], OUTPUT);
    pinMode(inBpin[i], OUTPUT);
    pinMode(pwmpin[i], OUTPUT);
  }

  movingAverage = temp;
}

int ch;

void loop() {
  bool gotNewTemp = false;
  // Check for temp setting from head unit
  while (serial2.available() > 0) {
    ch = serial2.read();
    Serial.print("got: ");
    Serial.println(ch, DEC);
    if (ch == 0) {  // ping
      ack = pwm;
      serial2.write(ack);
      serial2.println("OK");
    } else {
      gotNewTemp = true;
    }
  }
  // If we got a temp setting and it's within reasonable bounds, use it and log it.
  if (gotNewTemp && ch > 32 && ch < 80) {
    tempSetPoint = ch;
    Serial.print("Set temp to: ");
    Serial.println(ch, DEC);
  }
  // We can also take settings over primary Serial port for debugging
  if (Serial.available() > 0) {
    Serial.print("Set temp to: ");
    int ch = Serial.read();
    Serial.println(ch, DEC);
    if (ch > 32 && ch < 80) {
      tempSetPoint = ch;
    }
  }

  // Toggle status LED
  if (stat == LOW) {
    stat = HIGH;
  } else {
    stat = LOW;
  }
  digitalWrite(statpin, stat);

  uint8_t i;
  float average;
 
  // take N samples in a row, with a slight delay
  for (i=0; i< NUMSAMPLES; i++) {
    samples[i] = Thermistor(analogRead(THERMISTOR_PIN)); 
    delay(100);
  }
 
  // average NUMSAMPLES samples out to get a moving average and smoother PWM transitions
  average = 0;
  for (i=0; i< NUMSAMPLES; i++) {
    average += samples[i];
  }
  average /= NUMSAMPLES;

  Serial.print("Latest reading°F: "); 
  Serial.print(average);
  
  movingAverage = ((movingAverage * 19) + average) / 20;
  average = movingAverage;

  temp = (int) (average + .5);	// round up for display

  Serial.print(" Smoothed°F: "); 
  Serial.print(average);
  Serial.print(" Target°F: "); 
  Serial.print(tempSetPoint);

  // Determine PWM duty cycle and set it on the two Moto PWM pins
  float diff = average - (float)tempSetPoint;
  Serial.print(" pwm adj: "); 
  Serial.print(diff);
  pwm = PWM_BASE + (PWM_INCREMENT * diff);
  if (pwm < PWM_MIN) pwm = PWM_MIN;
  if (pwm > 255) pwm = 255;
  motorGo(0, CCW, pwm);
  motorGo(1, CCW, pwm);

  Serial.print(" PWM: "); 
  Serial.println(pwm);

  serial2.write(temp);                  // send temp to ESP8266
  
  delay(3000);
}


// This is from an example for driving the Moto shield for motor control.
// We only use it in one polarity (counterclockwise), although I suppose you
// could theoretically tweak things a bit to run in reverse and it might WARM
// the fridge :-)

/* motorGo() will set a motor going in a specific direction
 the motor will continue going in that direction, at that speed
 until told to do otherwise.
 
 motor: this should be either 0 or 1, will selet which of the two
 motors to be controlled
 
 direct: Should be between 0 and 3, with the following result
 0: Brake to VCC
 1: Clockwise
 2: CounterClockwise
 3: Brake to GND
 
 pwm: should be a value between ? and 1023, higher the number, the faster
 it'll go
 */
void motorGo(uint8_t motor, uint8_t direct, uint8_t pwm)
{
  if (motor <= 1)
  {
    if (direct <=4)
    {
      // Set inA[motor]
      if (direct <=1)
        digitalWrite(inApin[motor], HIGH);
      else
        digitalWrite(inApin[motor], LOW);

      // Set inB[motor]
      if ((direct==0)||(direct==2))
        digitalWrite(inBpin[motor], HIGH);
      else
        digitalWrite(inBpin[motor], LOW);

      analogWrite(pwmpin[motor], pwm);
    }
  }
}
