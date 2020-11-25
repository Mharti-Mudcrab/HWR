#include "TimerOne.h"

float outputValue;

void setup() 
{
  // set a timer of length 100.000 microseconds 
  // (or 0.1 sec - or 10Hz => the led will blink 5 times, 5 cycles of on-and-off, per second)
  // Mil -> kHz calc => https://www.unitjuggler.com/convert-frequency-from-%C2%B5s(p)-to-kHz.html?val=64
  Timer1.initialize(64); // Microsecond period to kHz => 15.625
  Timer1.pwm(9, 512);

  Serial.begin(9600);

}
 
void loop()
{
  /*
  outputValue = analogRead(9);
  Serial.println(outputValue);
  Serial.print(" ");

  delay(20000);*/
}
 
