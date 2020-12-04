//=================================== Main program ====================================
//setup state variables and define main program no other functions than setup and loop.



sdfskdfsdf



//Program flag variables
//----------------------
int programState = 0;
int boundryState = 0;
int returningState = 0;

//Charger return variables
int chargerSide = NO_CHARGERSIDE;
int wireFollowMethod = STRAIGHT; //ZIG_ZAG STRAIGHT CORNER

//Display and buttons variables
unsigned long displayTimer = 3000;
uint8_t buttons;

//Initialise the backend variables and setup pinmodes, timers and more.
//All of it is done through setup_and_funcs.cpp and performed in functions.cpp
//----------------------------------------------------------------------------
void setup() 
{
  setup_func();
}

//Program loop that calls methods in functions.ino and sensorRead() from RobotControl.ino
//---------------------------------------------------------------------------------------
void loop() 
{ 
    //Read button inputs
    buttons = lcd.readButtons();

    //Battery level check
    if (!sensorRead(BATTERY))
    {
        if (programState == PROG_CUT_GRASS)
            programState = PROG_RETURNING;
        else if(boundryState == BOUNDRY_TURN)  
            startTurn(REVERSE_TURN);
    }

    //Main program state machine
    switch (programState)
    {
    case PROG_CUT_GRASS:
        set_motors(FORWARD, FORWARD, MEDIUMSPEED, MEDIUMSPEED); 
                      
        if (sensorRead(BOUNDRY_OR_BUMPER))
            programState = PROG_AT_BOUNDRY;
            
        break;
    case PROG_AT_BOUNDRY:
        switch (boundryState)
        {
        case BOUNDRY_DRIVE_BACKWARDS:
            //Backup slowly to accurately measure distance
            set_motors(BACKWARD, BACKWARD, LOWSPEED, LOWSPEED);

            if (backwards_timer > 15) {
                if (sensorRead(HIGHEST_BOUNDRY_LEFT))
                    startTurn(MEDIUMSPEED, -90);
                else
                    startTurn(MEDIUMSPEED, 90);
                backwards_timer = 0;
            }
    
            //Back off until both boundry sensors are low
            if (!sensorRead(BOUNDRY_OR_BUMPER))
            {
                boundryState = BOUNDRY_TURN;
                //In case we are comming from charging.
                if (chargerSide != NO_CHARGERSIDE)
                {
                    if (chargerSide == LEFT_CHARGERSIDE)
                        startTurn(MEDIUMSPEED, -90);
                    else if (chargerSide == RIGHT_CHARGERSIDE)
                        startTurn(MEDIUMSPEED, 90);
                        
                    chargerSide = NO_CHARGERSIDE;
                }
                else
                {
                    startTurn(MEDIUMSPEED);
                }
            }
            break;
        case BOUNDRY_TURN:
            if (turnFinished())
            {
                programState = PROG_CUT_GRASS;
                boundryState = BOUNDRY_DRIVE_BACKWARDS;  //resetter flag til næste boundry møde
            }
            else if(sensorRead(BUMPER)) 
            {
                boundryState = BOUNDRY_DRIVE_BACKWARDS;
            }
            break;
        }
        break;
    case PROG_RETURNING:
        switch (returningState)
        {
        case RETURN_FIND_WIRE:
            if(turnFinished())
            {
                if(sensorRead(BUMPER))
                    set_motors(BACKWARD, BACKWARD, LOWSPEED, LOWSPEED);
                else if(sensorRead(DIRECTION_BACKWARD))
                    startTurn(LOWSPEED);
                else
                    set_motors(FORWARD, FORWARD, LOWSPEED, LOWSPEED);
                
                if (sensorRead(ANY_BOUNDRY))
                {
                    returningState = RETURN_FOLLOW_WIRE;
                    if (sensorRead(LEFT_BOUNDRY))
                        chargerSide = LEFT_CHARGERSIDE;
                    else
                        chargerSide = RIGHT_CHARGERSIDE;
                    
                    //if (wireFollowMethod == CORNER)
                    initFollowTurn(chargerSide);

                    wireFollowMethod = (wireFollowMethod +2) %3;
                    buttons = BUTTON_LEFT;
                }
            }
            break;
        case RETURN_FOLLOW_WIRE:
            switch (wireFollowMethod)
            {
            case ZIG_ZAG: //Zig-zag over the wire approach. 
                set_motors(FORWARD, FORWARD, MEDIUMSPEED, MEDIUMSPEED); 
                if (sensorRead(HIGHEST_BOUNDRY_LEFT, USE_OFFSET)) 
                    set_motors(FORWARD, FORWARD, ZEROSPEED, MEDIUMSPEED);
                else if(sensorRead(HIGHEST_BOUNDRY_RIGHT, USE_OFFSET)) 
                    set_motors(FORWARD, FORWARD, MEDIUMSPEED, ZEROSPEED);
                break;
            case STRAIGHT: //Driving straight as much as possible over wire approach.
                if (sensorRead(LEFT_BOUNDRY, USE_OFFSET))
                    set_motors(FORWARD, FORWARD, ZEROSPEED, MEDIUMSPEED);
                else if (sensorRead(RIGHT_BOUNDRY, USE_OFFSET))
                    set_motors(FORWARD, FORWARD, MEDIUMSPEED, ZEROSPEED);
                else
                    set_motors(FORWARD, FORWARD, MEDIUMSPEED, MEDIUMSPEED); 
                break;
            case CORNER: //Driving within the wire, following it with corner of the robot approach.
                if (turnFinished())
                {
                    if (chargerSide == LEFT_CHARGERSIDE) 
                    {
                        if (sensorRead(LEFT_BOUNDRY))
                            set_motors(FORWARD, FORWARD, MEDIUMSPEED, ZEROSPEED);
                        else
                            set_motors(FORWARD, FORWARD, ZEROSPEED, MEDIUMSPEED);
                    }
                    else
                    {
                        if (sensorRead(RIGHT_BOUNDRY))
                            set_motors(FORWARD, FORWARD, ZEROSPEED, MEDIUMSPEED);
                        else
                            set_motors(FORWARD, FORWARD, MEDIUMSPEED, ZEROSPEED);
                    }
                }
                break;
            }
            
            if (sensorRead(ZERO_BOUNDRY_RESPONSE))
                returningState = RETURN_FIND_WIRE;
              
            if (sensorRead(BUMPER))
            {
                set_motors(FORWARD, FORWARD, ZEROSPEED, ZEROSPEED);
                programState = PROG_CHARGING;
                returningState = RETURN_FIND_WIRE;
            }
            break;
        }
        break;
    case PROG_CHARGING:
        if (sensorRead(BATTERY, USE_OFFSET))
            programState = PROG_AT_BOUNDRY;
        break;
    }

    //Toggle and display fire follow method
    if (buttons)
    {
        if (buttons & BUTTON_LEFT)
        {
            lcd.setCursor(0, 1);
            lcd.print("FOLLOW WIRE ");
            switch (wireFollowMethod)
            {
            case ZIG_ZAG:
                wireFollowMethod = STRAIGHT;
                lcd.print("S V");
                break;
            case STRAIGHT:
                wireFollowMethod = CORNER;
                lcd.print("C V");
                break;
            case CORNER:
                wireFollowMethod = ZIG_ZAG;
                lcd.print("Z V");
                break;
            }
            lcd.print(analogRead(BATTERYSENSOR) * (5 / 1024.00) *2);
        }
    }

    //Update display every 500ms. as default
    if (displayTimer < millis())
    {
        displayTimer = millis() + 500;
        lcd.setCursor(0, 0);
        switch (programState)
        {
        case PROG_CUT_GRASS:
            lcd.print("CUTTING GRASS   ");
            lcd.setCursor(0, 1);
            lcd.print("R");
            lcd.print(analogRead(BOUNDRYSENSOR_R));
            lcd.print("      ");
            lcd.setCursor(8, 1);
            lcd.print("L");
            lcd.print(analogRead(BOUNDRYSENSOR_L));
            lcd.print("       ");
            break;
        case PROG_AT_BOUNDRY:
            switch (boundryState)
            {
            case BOUNDRY_DRIVE_BACKWARDS:
                lcd.print("AT BOUNDRY  R");
                lcd.print(analogRead(BOUNDRYSENSOR_R));
                lcd.print("  ");
                lcd.setCursor(0, 1);
                lcd.print("BACKING UP  L");
                lcd.print(analogRead(BOUNDRYSENSOR_L));
                lcd.print("  ");
                break;
            case BOUNDRY_TURN:
                lcd.print("AT BOUNDRY      ");
                lcd.setCursor(0, 1);
                lcd.print("TURNING ");
                lcd.print(static_cast<int>(turnRef / degreeCalcConst));
                if (digitalRead(DIR_L) == HIGH)
                    lcd.print("R     ");
                else
                    lcd.print("L     ");
                displayTimer = millis() + 1000;
                break;
            }
            break;
        case PROG_RETURNING:
            if(returningState != RETURN_FOLLOW_WIRE) {
                lcd.print("RETURN R");
                lcd.print(analogRead(BOUNDRYSENSOR_R));
                lcd.print("    ");
                lcd.setCursor(12, 0);
                lcd.print("L");
                lcd.print(analogRead(BOUNDRYSENSOR_L));
                lcd.print("    ");
                lcd.setCursor(0, 1);
            }
            switch (returningState)
            {
            case RETURN_FIND_WIRE:
                lcd.print("FIND WIRE   V");
                lcd.print(analogRead(BATTERYSENSOR) * (5 / 1024.00) *2);
                break;
            case RETURN_FOLLOW_WIRE:
                //Optimised to be displayed base on event.
                break;
            }
            break;
        case PROG_CHARGING:
            lcd.print("CHARGING    CS ");
            if (chargerSide == LEFT_CHARGERSIDE)
                lcd.print("L");
            else
                lcd.print("R");
            lcd.setCursor(0, 1);
            lcd.print("BATTERY: ");
            lcd.print(analogRead(BATTERYSENSOR) * (5 / 1024.00) *2);
            lcd.print("V");
            lcd.print("      ");
            break;
        }
    }
}
