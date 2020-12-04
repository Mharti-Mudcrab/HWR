//==============================================================================
//                                RobotControl
//        Setup project, by defineing:
//          Include Libraries
//          Defines global variables, constants, enum's, flag's, and timere
//          Defines I/O pin's
//
//        Setup utility functions:
//          setup serial communication
//          setup lcd-screen
//          setup I/O pind's input type.
//==============================================================================


//== Include Libraries =========================================================
#include <EnableInterrupt.h>
//Libs for display
#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>

//== Defines global variables, constants, enum's, flag's, and timere ===========

// Motor: Direction, ...
#define BACKWARD false
#define FORWARD true
// .. Enumlist for speed, ...
#define FULLSPEED   6
#define MEDIUMSPEED 4
#define LOWSPEED    2
#define ZEROSPEED   0
// .. encoders pins, ...
#define ENC_L 2
#define ENC_R 4
// .. control pins, Direction and PWM. OBS: NOTE(brake not used)
#define DIR_L 13
#define DIR_R 12
#define PWM_L 11
#define PWM_R 3

// Rotation: Flag for random, ...
#define RANDOM_TURN   0
// .. Flag for turn, ...
#define REVERSE_TURN  0
// .. Minimum angle, ...
#define MIN_RANDOM    90
// .. Maximum angle.
#define MAX_RANDOM    171

// State machine for program main: Enumlist.
#define PROG_CUT_GRASS  0
#define PROG_AT_BOUNDRY 1
#define PROG_RETURNING  2
#define PROG_CHARGING   3

// State machine for boundry and bumper: Enumlist, ...
#define BOUNDRY_DRIVE_BACKWARDS 0
#define BOUNDRY_TURN            1
//  .. Global bool.
#define USE_OFFSET              true

// State machine for finding and foloowing wire: Enumlist.
#define RETURN_FIND_WIRE    0
#define RETURN_FOLLOW_WIRE  1

// Charger side: Enumlist.
#define LEFT_CHARGERSIDE  0
#define RIGHT_CHARGERSIDE 1
#define UNKNOWN           2

// Following wire method: Enumlist.
#define ZIG_ZAG   0
#define STRAIGHT  1
#define CORNER    2

// Sensore: Enumliste, ...
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
// ..  Sensor pin's Analoge ...
#define BOUNDRYSENSOR_L A3
#define BOUNDRYSENSOR_R A2
#define BATTERYSENSOR A0
// .. og digitale, ...
#define BUMPERSENSOR_F 6
#define BUMPERSENSOR_B 7
// .. Sensor cutoff for Battery ...
#define BATTERY_CUTOFF 5.2
// .. Sensor cutoff for Boundry Left and Right ...
#define BOUNDRY_CUTOFF_L 180
#define BOUNDRY_CUTOFF_R 180

// Display: Constants.
#define WHITE 0x7


//== Setup method for pins and timer ===========================================
void setup_pins_and_timer(int timer1_counter)
{
  //== Setup serial for debugging.
  Serial.begin(9600);
  //== Initialise dispaly.
  Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();
  // OBS: NOTE(The shield uses the I^2C SCL and SDA pin's)

  // Display start screen.
  lcd.begin(16, 2);
  lcd.setBacklight(WHITE);
  lcd.print("Grassotron 3000");
  lcd.setCursor(0,1);
  lcd.print("Let's Cut Some G");

  //== Initialzing random func with value from unconnectted pin A1 (noise)
  int seed = analogRead(A1);
  randomSeed(seed);

  //== Pin modes for boundrySensors
  pinMode(BOUNDRYSENSOR_L, INPUT);        // sets the pin A3 to input
  digitalWrite(BOUNDRYSENSOR_L, LOW);     // use the internal pulldown resistor
  pinMode(BOUNDRYSENSOR_R, INPUT);        // sets the pin A2 to input
  digitalWrite(BOUNDRYSENSOR_L, LOW);     // use the internal pulldown resistor

  //== Pin modes for the encoders
  pinMode(ENC_L, INPUT);                  // sets the pin 2 to input
  digitalWrite(ENC_L, LOW);               // use the internal pulldown resistor
  pinMode(ENC_R, INPUT);                  // sets the pin 4 to input
  digitalWrite(ENC_R, LOW);               // use the internal pulldown resistor

  //== Setup pins for motors
  pinMode(DIR_L, OUTPUT);       // Pin controlling the direction of the motor
  pinMode(PWM_L, OUTPUT);       // Pin for controlling the speed of the motor
  digitalWrite(DIR_L, LOW);     // Setting the direction of the motor to forward
  analogWrite(PWM_L, ZEROSPEED);// Initializing the motor at speed 0
  pinMode(DIR_R, OUTPUT);
  pinMode(PWM_R, OUTPUT);
  digitalWrite(DIR_R, HIGH);
  analogWrite(PWM_R, ZEROSPEED);

  //== Initialize timer1 used for the P motor controller for the motors =======
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;

  TCNT1 = timer1_counter;   // preload timer
  TCCR1B |= (1 << CS12);    // 256 prescaler
  TIMSK1 |= (1 << TOIE1);   // enable timer overflow interrupt
  interrupts();             // enable all interrupts
}

//== Method for returning state of chosen sensor(s), with offset option. =======
bool sensorRead(int sensorRequestType, bool useOffset = false) {
    int offsetVal = 0;
    //==  Update offsetVal if useOffset depending on sensorRequestType. ========
    if (useOffset && sensorRequestType == BATTERY)
        offsetVal = 2;
    else if(useOffset && (sensorRequestType == HIGHEST_BOUNDRY_LEFT || sensorRequestType == HIGHEST_BOUNDRY_RIGHT))
        offsetVal = 10;
    else if (useOffset)
        offsetVal = 0;

    //== switch depending on sensorRequestType==================================
    switch (sensorRequestType) {

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
