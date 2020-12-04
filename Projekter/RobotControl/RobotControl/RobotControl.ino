//=================================== Setup ===================================
//Setting up global flags, pins, timer and utility functions.

//Include Libraries
//----------------------------
//Interrupt Lib
#include <EnableInterrupt.h>
//Libs for display
#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>

//Define global flags and vars
//----------------------------
//Display color
#define WHITE 0x7

//Direction
#define BACKWARD false
#define FORWARD true

//Speed values
#define FULLSPEED   6
#define MEDIUMSPEED 4
#define LOWSPEED    2
#define ZEROSPEED   0

//Random turn flag
#define RANDOM_TURN   0
#define REVERSE_TURN  0
#define MIN_RANDOM    90
#define MAX_RANDOM    171

//Program state flags
#define PROG_CUT_GRASS  0
#define PROG_AT_BOUNDRY 1
#define PROG_RETURNING  2
#define PROG_CHARGING   3

//At boundry state flags
#define BOUNDRY_DRIVE_BACKWARDS 0
#define BOUNDRY_TURN            1
#define USE_OFFSET              true

//Returning to charger state flags
#define RETURN_FIND_WIRE    0
#define RETURN_FOLLOW_WIRE  1

//Charger return side
#define LEFT_CHARGERSIDE  0
#define RIGHT_CHARGERSIDE 1
#define NO_CHARGERSIDE    2

//Wire follow method
#define ZIG_ZAG   0
#define STRAIGHT  1
#define CORNER    2

//Sensor request type
#define LEFT_BOUNDRY          0
#define RIGHT_BOUNDRY         1
#define ANY_BOUNDRY           2
#define HIGHEST_BOUNDRY_LEFT  3
#define HIGHEST_BOUNDRY_RIGHT 4
#define ZERO_BOUNDRY_RESPONSE 5
#define BUMPER                6
#define BOUNDRY_OR_BUMPER     7
#define BATTERY               8
#define DIRECTION_FORWARD     9
#define DIRECTION_BACKWARD    10

//Sufficient battery level cutoff
#define BATTERY_CUTOFF 5.2

//Boundry sensor cutoff
#define BOUNDRY_CUTOFF_L 180 // venstre er for hurtig med høj hastighed.
#define BOUNDRY_CUTOFF_R 180 // Right sensor is vasly more sensitive than other.

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
#define BOUNDRYSENSOR_L A3
#define BOUNDRYSENSOR_R A2
//Battery sensor pin
#define BATTERYSENSOR A0 
//Bumper sensor pin
#define BUMPERSENSOR_F 6
#define BUMPERSENSOR_B 7

// Initialise lcd which controls the display trough I^2C
// The shield uses the I^2C SCL and SDA pins
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

//Setup method for pins and timer
void setup_pins_and_timer(int timer1_counter)
{
  //Setup serial for debugging.
  Serial.begin(9600);

  //Setup display with LCD's number of columns and rows:
  lcd.begin(16, 2);
  lcd.setBacklight(WHITE);
  lcd.print("Grassotron 3000");
  lcd.setCursor(0,1);
  lcd.print("Let's Cut Some G");
  
  //Initialzing random func with value from unconnectted pin A0
  int seed = analogRead(A1);
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
bool sensorRead(int sensorRequestType, bool useOffset = false) 
{   
    int offsetVal = 0;
    if (useOffset && sensorRequestType == BATTERY)
        offsetVal = 2;
    else if(useOffset && (sensorRequestType == HIGHEST_BOUNDRY_LEFT || sensorRequestType == HIGHEST_BOUNDRY_RIGHT))
        offsetVal = 10;
    else if (useOffset)
        offsetVal = 0;
    
        
    switch (sensorRequestType)
    {
    case LEFT_BOUNDRY:
        return analogRead(BOUNDRYSENSOR_L) > (BOUNDRY_CUTOFF_L + offsetVal) || lcd.readButtons() & BUTTON_UP;
    case RIGHT_BOUNDRY:
        return analogRead(BOUNDRYSENSOR_R) > (BOUNDRY_CUTOFF_R + offsetVal) || lcd.readButtons() & BUTTON_UP;
    case ANY_BOUNDRY:
        return sensorRead(LEFT_BOUNDRY) || sensorRead(RIGHT_BOUNDRY);
    case HIGHEST_BOUNDRY_LEFT:
        return analogRead(BOUNDRYSENSOR_L) > analogRead(BOUNDRYSENSOR_R) + offsetVal;
    case HIGHEST_BOUNDRY_RIGHT:
        return analogRead(BOUNDRYSENSOR_R) > analogRead(BOUNDRYSENSOR_L) + offsetVal;
    case ZERO_BOUNDRY_RESPONSE:
        return analogRead(BOUNDRYSENSOR_R) == 0 && analogRead(BOUNDRYSENSOR_L) == 0;
    case BUMPER:
        return !(digitalRead(BUMPERSENSOR_F) && digitalRead(BUMPERSENSOR_B));
    case BOUNDRY_OR_BUMPER: //Can be activated with SELECT button on the display
        return sensorRead(ANY_BOUNDRY) || sensorRead(BUMPER) || lcd.readButtons() & BUTTON_SELECT; 
    case BATTERY:           //Can be activated with RIGTH button on the display
        return analogRead(BATTERYSENSOR) * (5 / 1024.00) *2 > BATTERY_CUTOFF + offsetVal && !(lcd.readButtons() & BUTTON_RIGHT);
    case DIRECTION_FORWARD:
        return digitalRead(DIR_L) == HIGH && !(digitalRead(DIR_R) == HIGH);
    case DIRECTION_BACKWARD:
        return digitalRead(DIR_L) == LOW && !(digitalRead(DIR_R) == LOW);
    }
}
