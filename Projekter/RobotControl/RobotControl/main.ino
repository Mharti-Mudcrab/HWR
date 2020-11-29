// =================================== Main program ====================================
// setup state variables and define main program no other functions than setup and loop.

//Program flag variables
//----------------------
int programState = 0;
int boundryState = 0;
int returningState = 0;

//Charger return variables
int chargerSide = -1;
int wireFollowMethod = CORNER; // ZIG_ZAG STRAIGHT CORNER
unsigned long displayTimer = 0;

//Initialise the backend variables and setup pinmodes, timers and more.
//All of it is done through setup_and_funcs.cpp and performed in functions.cpp
//----------------------------------------------------------------------------
void setup() 
{
  setup_func();
}

// Program loop that calls methods in setup_and_funcs.cpp
//-------------------------------------------------------
void loop() 
{ 
    uint8_t buttons = lcd.readButtons();
    if (buttons)
    {
        if (buttons & BUTTON_LEFT)
        {
            switch (wireFollowMethod)
            {
            case ZIG_ZAG:
                wireFollowMethod = STRAIGHT;
                break;
            case STRAIGHT:
                wireFollowMethod = CORNER;
                break;
            case CORNER:
                wireFollowMethod = ZIG_ZAG;
                break;
            }
        }
    }

  
    // Battery level check
    if (!sensorRead(BATTERY))
    {
        if (programState == PROG_CUT_GRASS)
            programState = PROG_RETURNING;
        else if(boundryState == BOUNDRY_TURN)  
            startTurn(REVERSE_TURN);
    }
    
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
            // Backup slowly to accurately measure distance
            set_motors(BACKWARD, BACKWARD, MEDIUMSPEED, MEDIUMSPEED);
    
            //Back off until both boundry sensors are low
            if (!sensorRead(BOUNDRY_OR_BUMPER))
            {
                boundryState = BOUNDRY_TURN;
                // In case we are comming from charging.
                if (chargerSide != -1)
                {
                    if (chargerSide == LEFT_CHARGERSIDE)
                        startTurn(MEDIUMSPEED, -90);
                    else if (chargerSide == RIGHT_CHARGERSIDE)
                        startTurn(MEDIUMSPEED, 90);
                        
                    chargerSide = -1;
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
                boundryState = BOUNDRY_DRIVE_BACKWARDS;  // resetter flag til næste boundry møde
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
            set_motors(FORWARD, FORWARD, LOWSPEED, LOWSPEED);
            
            if (sensorRead(ANY_BOUNDRY))
            {
                returningState = RETURN_FOLLOW_WIRE;
                if (sensorRead(LEFT_BOUNDRY))
                    chargerSide = LEFT_CHARGERSIDE;
                else
                    chargerSide = RIGHT_CHARGERSIDE;
                
                if (wireFollowMethod == CORNER)
                    initFollowTurn(chargerSide);
            }
            break;
        case RETURN_FOLLOW_WIRE:
            switch (wireFollowMethod)
            {
            case ZIG_ZAG:
                // zig-zag approach. 
                if (sensorRead(LEFT_BOUNDRY, USE_OFFSET)) 
                    set_motors(FORWARD, FORWARD, LOWSPEED, MEDIUMSPEED);
                else if (sensorRead(RIGHT_BOUNDRY, USE_OFFSET))
                    set_motors(FORWARD, FORWARD, MEDIUMSPEED, LOWSPEED);
                    
                break;
            case STRAIGHT:
                if (sensorRead(LEFT_BOUNDRY, USE_OFFSET))
                    set_motors(FORWARD, FORWARD, ZEROSPEED, MEDIUMSPEED);
                else if (sensorRead(RIGHT_BOUNDRY, USE_OFFSET))
                    set_motors(FORWARD, FORWARD, MEDIUMSPEED, ZEROSPEED);
                else
                    set_motors(FORWARD, FORWARD, MEDIUMSPEED, MEDIUMSPEED); 
                break;
            case CORNER:
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
        if (sensorRead(BATTERY))
            programState = PROG_AT_BOUNDRY;
        break;
    }



    //Display
    if (displayTimer < millis())
    {
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
                lcd.print(turnRef / degreeCalcConst);
                if (digitalRead(DIR_L) == HIGH)
                    lcd.print("R     ");
                else
                    lcd.print("L     ");
                displayTimer = millis() + 1000;
                break;
            }
            break;
        case PROG_RETURNING:
            lcd.print("RETURN R");
            lcd.print(analogRead(BOUNDRYSENSOR_R));
            lcd.print(" L");
            lcd.print(analogRead(BOUNDRYSENSOR_L));
            lcd.print("    ");
            lcd.setCursor(0, 1);
            switch (returningState)
            {
            case RETURN_FIND_WIRE:
                lcd.print("FIND WIRE   V");
                lcd.print(analogRead(BATTERYSENSOR) * (5 / 1024.00) *2);
                break;
            case RETURN_FOLLOW_WIRE:
                lcd.print("FOLLOW WIRE ");
                
                switch (wireFollowMethod)
                {
                case ZIG_ZAG:
                    lcd.print("Z V");
                    break;
                case STRAIGHT:
                    lcd.print("S V");
                    break;
                case CORNER:
                    lcd.print("C V");
                    break;
                }
                lcd.print(analogRead(BATTERYSENSOR) * (5 / 1024.00) *2);
                break;
            }
            break;
        case PROG_CHARGING:
            lcd.print("CHARGING        ");
            lcd.setCursor(0, 1);
            lcd.print("BATTERY: ");
            lcd.print(analogRead(BATTERYSENSOR) * (5 / 1024.00) *2);
            lcd.print("V");
            lcd.print("      ");
            break;
        }
    }
}
