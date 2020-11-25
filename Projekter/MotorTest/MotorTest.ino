#include "TimerOne.h"

#define A_DIRECTION 12
#define A_PWM 3
#define A_BRAKE 9
#define B_DIRECTION 13
#define B_PWM 11
#define B_BRAKE 8

void timerForStop();
void setPWMOfPin(int pin, long pwm);
void stopAllMotor();
void setPinout(int pin, int lvl);

bool reset = false;
 
void setup() 
{
  // set a timer of length 100.000 microseconds 
  // (or 0.1 sec - or 10Hz => the led will blink 5 times, 5 cycles of on-and-off, per second)
  Timer1.initialize(300000);
  Timer1.attachInterrupt(timerForStop); // attach the service routine here
  
  Serial.begin(9600);
  Serial.println("========================= NEW RUN =========================");

  pinMode(A_DIRECTION, OUTPUT);
  pinMode(A_BRAKE, OUTPUT);
  pinMode(B_DIRECTION, OUTPUT);
  pinMode(B_BRAKE, OUTPUT);
  pinMode(A_PWM, OUTPUT);
  pinMode(B_PWM, OUTPUT);

  analogWrite(A_PWM, 0);
  analogWrite(B_PWM, 0);
  digitalWrite(A_BRAKE, LOW);
  digitalWrite(B_BRAKE, LOW);
  digitalWrite(A_DIRECTION, LOW);
  digitalWrite(B_DIRECTION, LOW);

  Timer1.pwm(9, 512);
}
 
void loop()
{

    /*
  if(Serial.available() > 0){   //check if any data was received
    long firstNum = Serial.parseInt();
    long secondNum = Serial.parseInt();
        
    Serial.print("Data is: ");
    Serial.print(firstNum, DEC);
    Serial.print(", ");
    Serial.println(secondNum, DEC);
    
    Serial.read(); // Read to clear rest of input i.e. "\n"

    switch(firstNum){
      case 12:  // A_DIRECTION 
      case 13:  // B_DIRECTION
      case 9:   // A_BRAKE 
      case 8:   // B_BRAKE
        setPinout(firstNum, secondNum);
        break;
        
      case 3:   // A_PWM
      case 11:  // B_PWM 
        setPWMOfPin(firstNum, secondNum);
        break;
        
      default:  // Set PWM's to zero
        stopAllMotor();
    }

    if (firstNum != 0)
      reset = true;
      Timer1.start();
      Serial.println("Timer started at time ");
      Serial.println(Timer1.);
  }*/
}
 
//--------------------------
//Custom ISR Timer Routine
//--------------------------
void timerForStop()
{
  if (reset) {
    Serial.println("timerForStop() called");
    // stop all motorrelated
    stopAllMotor();
    reset = false;
  }
  // Timer1.stop();
}

void setPWMOfPin(int pin, long pwm)
{
  if (pwm <= 100 && pwm >= 0)
  {
      pwm = 255 * pwm / 100; // 6-bit
      analogWrite(pin, pwm); // Sæt ny PWM.
      Serial.print("Setting pin: ");
      Serial.print(pin, DEC);
      Serial.print(", with new PWM: ");
      Serial.println(pwm, DEC);
  }
  else
  {
    Serial.print("Could not set PWM of: ");
    Serial.println(pwm, DEC);
  }
  /*
  if (pwm <= 100 && pwm >= 0)
  {
      pwm = 1024 * pwm / 100; // 6-bit
      Timer1.pwm(pin, pwm); // Sæt ny PWM.
      Serial.print("Setting pin: ");
      Serial.print(pin, DEC);
      Serial.print(", with new PWM: ");
      Serial.println(pwm, DEC);
  }
  else
  {
    Serial.print("Could not set PWM of: ");
    Serial.println(pwm, DEC);
  }*/
}

void stopAllMotor()
{
  //Timer1.pwm(A_PWM, 0);
  //Timer1.pwm(B_PWM, 0);
  analogWrite(A_PWM, 0);
  analogWrite(B_PWM, 0);
  
  digitalWrite(A_BRAKE, LOW);
  digitalWrite(B_BRAKE, LOW);
  Serial.println("stopAllMotor() called");
}

void setPinout(int pin, int lvl)
{
  if (pin == A_DIRECTION || pin == B_DIRECTION)
  {
    if (lvl == 0 | lvl == 1)
    {
      digitalWrite(pin, lvl);
      Serial.print("pin: ");
      Serial.print(pin, DEC);
      Serial.print(" is set to: ");
      if (lvl == 1)
        Serial.println("HIGH");
      else
        Serial.println("LOW");          
    }
  }
}
