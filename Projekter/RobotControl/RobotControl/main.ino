// =================================== Main program ====================================
// setup state variables and define main program no other functions than setup and loop.

//Program flag variables
//----------------------
int programState = 0;
int boundryState = 0;
int returningState = 0;

//Charger return variables
int chargerSide = -1;
int wireFollowMethod = ZIG_ZAG;

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
    Serial.print("leftSens = ");
    Serial.print(analogRead(BOUNDRYSENSOR_L));
    Serial.print("\trightSens = ");
    Serial.print(analogRead(BOUNDRYSENSOR_R));

    /*Serial.print("SpeedL, ");
    Serial.print(current_speed_L);
    Serial.print(", SpeedR, ");
    Serial.print(current_speed_R);
    Serial.print(", refL, ");
    Serial.print(desired_speed_L);
    Serial.print(", refR, ");
    Serial.print(desired_speed_R);*/
    
    Serial.print("\tposval_L = ");
    Serial.print(posVal_L);
    Serial.print("\t posval_R = ");
    Serial.print(posVal_R);
    Serial.print("\t P_L = ");
    Serial.print(abs(P_L));
    Serial.print("\t P_R = ");
    Serial.print(abs(P_R));
    Serial.print("\t err_L = ");
    Serial.print(errorVal_L);
    Serial.print("\t err_R = ");
    Serial.print(errorVal_R);
    Serial.print("\t Intergal_L = ");
    Serial.print(errorIntSum_L);
    Serial.print("\t R = ");
    Serial.print(errorIntSum_R);
    Serial.print("\t cur_speed_L = ");
    Serial.print(current_speed_L);
    Serial.print("\t cur_speed_R = ");
    Serial.print(current_speed_R);
    Serial.print("\t desired = ");
    Serial.print(desired_speed_L);

    /*Serial.print("desired_speed_L ");
    Serial.print(desired_speed_L);
    Serial.print("\t errorVal_L ");
    Serial.print(errorVal_L);
    Serial.print("\t current_speed_L ");
    Serial.println(current_speed_L);
    /*Serial.print("\t errorVal_R ");
    Serial.print(errorVal_R);
    Serial.print("\t current_speed_R ");
    Serial.println(current_speed_R);*/
    
    //delay(1000);
    
    Serial.print("\tProgState = ");
    Serial.print(programState);
    Serial.print("\tBoundState = ");
    Serial.println(boundryState);
    /*Serial.print("\tisMidtTurn = ");
    Serial.print(isMidtTurn);
    Serial.print("\tstartTurn_L = ");
    Serial.print(turnStart_L);
    Serial.print("\tturnRef = ");
    Serial.println(turnRef);
    //Serial.print("\tReturnState = ");
    //Serial.println(returningState);
    */
  
    /* Battery lvl not yet supported
    if (sensorRead(BATTERY) == LAV)
    {
        if (boundryState == BOUNDRY_DRIVE_BACKWARDS)
            programState = PROG_RETURNING
        else  
            startTurn(REVERSE_TURN);
    }
    */  
    
    switch (programState)
    {
    case PROG_CUT_GRASS:
        set_motors(FORWARD, FORWARD, MEDIUMSPEED, MEDIUMSPEED); 
        
        if (sensorRead(BOUNDRY_OR_BUMPER));
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
            set_motors(FORWARD, FORWARD, MEDIUMSPEED, MEDIUMSPEED);
            
            if (sensorRead(ANY_BOUNDRY))
            {
                returningState = RETURN_FOLLOW_WIRE;
                if (sensorRead(LEFT_BOUNDRY))
                    chargerSide = LEFT_CHARGERSIDE;
                else
                    chargerSide = RIGHT_CHARGERSIDE;
            }
            break;
        case RETURN_FOLLOW_WIRE:
            switch (wireFollowMethod)
            {
            case ZIG_ZAG:
                // zig-zag approach. 
                if (sensorRead(LEFT_BOUNDRY)) 
                    set_motors(FORWARD, FORWARD, LOWSPEED, MEDIUMSPEED);
                else if (sensorRead(RIGHT_BOUNDRY))
                    set_motors(FORWARD, FORWARD, MEDIUMSPEED, LOWSPEED);
                    
                break;
            case STRAIGHT:
                if (sensorRead(LEFT_BOUNDRY)) 
                    set_motors(FORWARD, FORWARD, ZEROSPEED, MEDIUMSPEED);
                else if (sensorRead(RIGHT_BOUNDRY))
                    set_motors(FORWARD, FORWARD, MEDIUMSPEED, ZEROSPEED);
                else
                    set_motors(FORWARD, FORWARD, MEDIUMSPEED, MEDIUMSPEED); 
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
    lcd.setCursor(0, 0);
    switch (programState)
    {
    case PROG_CUT_GRASS:
        lcd.print("CUTTING GRASS   ");
        break;
    case PROG_AT_BOUNDRY:
        lcd.print("AT BOUNDRY      ");
        lcd.setCursor(0, 1);
        switch (boundryState)
        {
        case BOUNDRY_DRIVE_BACKWARDS:
            lcd.print("BACKING UP      ");
            break;
        case BOUNDRY_TURN:
            lcd.print("TURNING         ");
            lcd.print(turnRef / degreeCalcConst);
            if (digitalRead(DIR_L) == HIGH)
                lcd.print("R");
            else
                lcd.print("L");
            break;
        }
        break;
    case PROG_RETURNING:
        lcd.print("RETURN TO CHARGE");
        lcd.setCursor(0, 1);
        switch (returningState)
        {
        case RETURN_FIND_WIRE:
            lcd.print("FINDING WIRE    ");
            break;
        case RETURN_FOLLOW_WIRE:
            lcd.print("FOLLOWING WIRE ");
            switch (wireFollowMethod)
            {
            case ZIG_ZAG:
                lcd.print("Z");
                break;
            case STRAIGHT:
                lcd.print("S");
                break;
            }
            break;
        }
        break;
    case PROG_CHARGING:
        lcd.print("CHARGING        ");
        lcd.setCursor(0, 1);
        lcd.print("BATTERY: ");
        lcd.print(analogRead(BATTERYSENSOR) * (5 / 1024.00) *2);
        lcd.print("V");
        break;
    }
}
