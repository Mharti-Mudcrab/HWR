// =================================== Setup ===================================
// Setting up global flags, pins, timer and utility functions.

#include <EnableInterrupt.h>

//Define global flags and vars
//----------------------------
//Direction
#define BACKWARD false
#define FORWARD true

//Speed values
#define FULLSPEED 9   // With 80ms timer 9.63 ticks med battery uden er den 4.75
#define MEDIUMSPEED 6
#define LOWSPEED 3
#define ZEROSPEED 0

//Random turn flag
#define RANDOM_TURN 0
#define REVERSE_TURN 0
#define MIN_RANDOM 100
#define MAX_RANDOM 180 //Maximum in random(Min, Max) is Max-1

//Program state flags
#define PROG_CUT_GRASS 0
#define PROG_AT_BOUNDRY 1
#define PROG_RETURNING 2
#define PROG_CHARGING 3

//At boundry state flags
#define BOUNDRY_DRIVE_BACKWARDS 0
#define BOUNDRY_TURN 1

//Returning to charger state flags
#define RETURN_FIND_WIRE 0
#define RETURN_FOLLOW_WIRE 1

//Charger return side
#define LEFT_CHARGERSIDE 0
#define RIGHT_CHARGERSIDE 1

//Wire follow method
#define ZIG_ZAG 0
#define STRAIGHT 1

//Sensor request type
#define LEFT_BOUNDRY 0
#define RIGHT_BOUNDRY 1
#define ANY_BOUNDRY 2
#define BUMPER 3
#define BATTERY 4

//Boundry sensor cutoff
#define BOUNDRY_CUTOFF_L 150 // venstre er for hurtig med høj hastighed.
#define BOUNDRY_CUTOFF_R 140 // Right sensor is vasly more sensitive than other.

//  Pins
//Motor encoders pins
#define ENC_L 2 
#define ENC_R 4 
//Motor control pins (brake not used)
#define DIR_L 13
#define DIR_R 12
#define PWM_L 11
#define PWM_R 3
//Boundry wire sensor pins
#define BOUNDRYSENSOR_L A2
#define BOUNDRYSENSOR_R A4
//Bumper sensor pin
#define BUMPERSENSOR 6 

//Setup method for pins and timer
void setup_pins_and_timer(int timer1_counter)
{
  Serial.begin(9600);
  
  //Initialzing random func with value from unconnectted pin A0
  int seed = analogRead(A0);
  //Serial.print("Seed = ");
  //Serial.println(seed);
  randomSeed(seed);
  
  //Pin modes for boundrySensors
  pinMode(BOUNDRYSENSOR_L, INPUT);
  digitalWrite(BOUNDRYSENSOR_L, LOW); // maybe HIGH ?
  pinMode(BOUNDRYSENSOR_R, INPUT);
  digitalWrite(BOUNDRYSENSOR_L, LOW);
  //analogReference(INTERNAL); // maybe EXTERNAL ?
  
  //Pin modes for the encoders
  pinMode(ENC_L, INPUT);     // set the pin 2 to input
  digitalWrite(ENC_L, LOW);  // use the internal pulldown resistor
  pinMode(ENC_R, INPUT);     // set the pin 4 to input
  digitalWrite(ENC_R, LOW);  // use the internal pulldown resistor

  //Setup pins or motors
  //--------------------
  pinMode(DIR_L, OUTPUT);    // Pin controlling the direction of the motor
  pinMode(PWM_L, OUTPUT);    // Pin for controlling the speed of the motor
  digitalWrite(DIR_L, LOW);  // Setting the direction of the motor to forward
  analogWrite(PWM_L, ZEROSPEED);     // Initializing the motor at speed 0

  pinMode(DIR_R, OUTPUT);    
  pinMode(PWM_R, OUTPUT);    
  digitalWrite(DIR_R, HIGH);  
  analogWrite(PWM_R, ZEROSPEED);

  //Initialize timer1 used for the P motor controller for the motors
  //-----------------------------------------------------------------
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;

  TCNT1 = timer1_counter;   // preload timer
  TCCR1B |= (1 << CS12);    // 256 prescaler
  TIMSK1 |= (1 << TOIE1);   // enable timer overflow interrupt
  interrupts();             // enable all interrupts
}

//Method for returning state of chosen sensor(s)
bool sensorRead(int sensorRequestType) 
{
    switch (sensorRequestType)
    {
    case LEFT_BOUNDRY:
        return analogRead(BOUNDRYSENSOR_L) > BOUNDRY_CUTOFF_L;
    case RIGHT_BOUNDRY:
        return analogRead(BOUNDRYSENSOR_R) > BOUNDRY_CUTOFF_R;
    case ANY_BOUNDRY:
        return analogRead(BOUNDRYSENSOR_L) > BOUNDRY_CUTOFF_L ||
               analogRead(BOUNDRYSENSOR_R) > BOUNDRY_CUTOFF_R;
    case BUMPER:
        return digitalRead(BUMPERSENSOR);
    case BATTERY:
        // TODO
        return false;
    }
}
