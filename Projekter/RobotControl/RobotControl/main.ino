//==============================================================================
//                                Main Program
//        Setup state variables and it's init state.
//        Calls Arduino setup() and loop().
//        setup() cals setup_func() in functions.ino their set's up:
//                pin's, timer's, enum varibels, and interupts handler.
//        loop() contains "AI's" State machine, and Display handler
//               It's use method's from functions.ino and RobotControl.ino
//==============================================================================

//== Define Init state of state machine varibels ===============================
int programState = 0;
int boundryState = 0;
int returningState = 0;

//== Defines default method and varibels for following wire and charching ======
int wireFollowMethod = STRAIGHT; //Other options is: ZIG_ZAG , STRAIGHT , CORNER
int chargerSide = UNKNOWN;

//== LCD screen variables ======================================================
unsigned long displayTimer = 3000;
uint8_t buttons; // Binary container for the butten pressed on display

//== Initialise ================================================================
void setup() {
  setup_func(); // set's up pin's, timer's, enum varibels, and interupts handler.
}

//== Program loop  State machine controller  ===================================
void loop() {

    buttons = lcd.readButtons(); // Read button input's

//== Battery Low Check handler =================================================
    if (!sensorRead(BATTERY)) {
        if (programState == PROG_CUT_GRASS)
            programState = PROG_RETURNING;
        else if(boundryState == BOUNDRY_TURN)
            startTurn(REVERSE_TURN);
    }

//== Main Program state machine  ===============================================
    switch (programState) {

    case PROG_CUT_GRASS: // Drive forward if no boundr or bumper
        set_motors(FORWARD, FORWARD, MEDIUMSPEED, MEDIUMSPEED);
        if (sensorRead(BOUNDRY_OR_BUMPER))
            programState = PROG_AT_BOUNDRY;
        break;

    case PROG_AT_BOUNDRY:
    //== Sub state machine boundry and bumper handler ==========================
        switch (boundryState) {

        case BOUNDRY_DRIVE_BACKWARDS:
            // Drive backword max 15 cyclics if boundry bumper not low
            set_motors(BACKWARD, BACKWARD, LOWSPEED, LOWSPEED);
            if (backwards_cyclics > 15) {
                if (sensorRead(HIGHEST_BOUNDRY_LEFT))
                    startTurn(MEDIUMSPEED, -90);
                else
                    startTurn(MEDIUMSPEED, 90);
                backwards_cyclics = 0;
            }
            // Drive backword until both boundry sensors are low
            if (!sensorRead(BOUNDRY_OR_BUMPER)) {
                boundryState = BOUNDRY_TURN;
                startTurn(MEDIUMSPEED);
            }
            // In case we are comming from charging, and bumper is free.
            else if (!sensorRead(BUMPER) && chargerSide != UNKNOWN) {
                if (chargerSide == LEFT_CHARGERSIDE)
                    startTurn(MEDIUMSPEED, -90);
                else if (chargerSide == RIGHT_CHARGERSIDE)
                    startTurn(MEDIUMSPEED, 90);
                chargerSide = UNKNOWN;
                boundryState = BOUNDRY_TURN;
            }

            break;

        case BOUNDRY_TURN:
            if (turnFinished()) {
                // Return to main state mashine
                programState = PROG_CUT_GRASS;
                //Reset sub state mashine for next boundry bumper.
                boundryState = BOUNDRY_DRIVE_BACKWARDS;
            }
            else if(sensorRead(BUMPER) {
                //== ROOM for IMPROVEMENT ======================================
                boundryState = BOUNDRY_DRIVE_BACKWARDS;
            }
            break;
        }
        break;
        //== End of sub state machine boundry and bumper handler ===============

    case PROG_RETURNING:
        //== Sub state machine returning to charge station (low battery) =======
        switch (returningState) {

        case RETURN_FIND_WIRE:
            // Avoiding a obstacle.
            if(sensorRead(BUMPER))
                set_motors(BACKWARD, BACKWARD, LOWSPEED, LOWSPEED);
            else if(sensorRead(DIRECTION_BACKWARD))
                startTurn(LOWSPEED);
            else
                set_motors(FORWARD, FORWARD, LOWSPEED, LOWSPEED);

            // Correcting orientation towards boundry wire.
            if (sensorRead(ANY_BOUNDRY) && turnFinished() ) {
                returningState = RETURN_FOLLOW_WIRE;
                if (sensorRead(LEFT_BOUNDRY))
                    chargerSide = LEFT_CHARGERSIDE;
                else
                    chargerSide = RIGHT_CHARGERSIDE;

                initFollowTurn(chargerSide);

                buttons = BUTTON_LEFT; // Toggle follow wire dispaly,
                wireFollowMethod = (wireFollowMethod +2) %3;
                // Correcting toggle method change, coused by dispaly toogle.
            }
            break;

        case RETURN_FOLLOW_WIRE:
            //== Sub state machine depending follow wire method ================
            switch (wireFollowMethod) {

            case ZIG_ZAG: // Driving Zig-zag, above wire approach.
                set_motors(FORWARD, FORWARD, MEDIUMSPEED, MEDIUMSPEED);
                if (sensorRead(HIGHEST_BOUNDRY_LEFT, USE_OFFSET))
                    set_motors(FORWARD, FORWARD, ZEROSPEED, MEDIUMSPEED);
                else if(sensorRead(HIGHEST_BOUNDRY_RIGHT, USE_OFFSET))
                    set_motors(FORWARD, FORWARD, MEDIUMSPEED, ZEROSPEED);
                break;
            case STRAIGHT: // Driving Straight if possible, above wire approach.
                if (sensorRead(LEFT_BOUNDRY, USE_OFFSET))
                    set_motors(FORWARD, FORWARD, ZEROSPEED, MEDIUMSPEED);
                else if (sensorRead(RIGHT_BOUNDRY, USE_OFFSET))
                    set_motors(FORWARD, FORWARD, MEDIUMSPEED, ZEROSPEED);
                else
                    set_motors(FORWARD, FORWARD, MEDIUMSPEED, MEDIUMSPEED);
                break;
            case CORNER: // Driving inside the wire, using Corner sensor.
                if (chargerSide == LEFT_CHARGERSIDE) {
                    if (sensorRead(LEFT_BOUNDRY))
                        set_motors(FORWARD, FORWARD, MEDIUMSPEED, ZEROSPEED);
                    else
                        set_motors(FORWARD, FORWARD, ZEROSPEED, MEDIUMSPEED);
                } else {
                    if (sensorRead(RIGHT_BOUNDRY))
                        set_motors(FORWARD, FORWARD, ZEROSPEED, MEDIUMSPEED);
                    else
                        set_motors(FORWARD, FORWARD, MEDIUMSPEED, ZEROSPEED);
                }
                break;
            }

            // if sense of wire is complety gone, locate wire again
            if (sensorRead(ZERO_BOUNDRY_RESPONSE))
                returningState = RETURN_FIND_WIRE;

            // if charger sation is found.
            if (sensorRead(BUMPER)) {
                set_motors(FORWARD, FORWARD, ZEROSPEED, ZEROSPEED);
                programState = PROG_CHARGING;
                returningState = RETURN_FIND_WIRE;
            }
            break;
        }
        break;
    case PROG_CHARGING:
        // if Charging is done, navigate into grass field.
        if (sensorRead(BATTERY, USE_OFFSET))
            programState = PROG_AT_BOUNDRY;
        break;
    }

//== Toggle and display follow wire method, show voltage on display ============
    if (buttons) {
        if (buttons & BUTTON_LEFT)
        {
            lcd.setCursor(0, 0);
            lcd.print("RETURN          ");
            lcd.setCursor(0, 1);
            lcd.print("FOLLOW WIRE ");
            //== Toggle between diffrent follow wire methods ===================
            switch (wireFollowMethod) {

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

//== Update display every 500ms, depending on state machine's ==================
    if (displayTimer < millis())
    {
        displayTimer = millis() + 500;
        lcd.setCursor(0, 0);
        //== Display  according to main state machine ==========================
        switch (programState) {
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
            switch (boundryState) {
            //== Display according to sub state machine boundry and bumper =====
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
            //== End Display according to sub state machine boundry and bumper =

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
            switch (returningState) {
            //== Display according to sub state machine returning to station ===
            case RETURN_FIND_WIRE:
                lcd.print("FIND WIRE   V");
                lcd.print(analogRead(BATTERYSENSOR) * (5 / 1024.00) *2);
                break;
            case RETURN_FOLLOW_WIRE:
                // Optimised to be displayed base on event,
                // as Toggle and display follow wire method...
                break;
            }
            break;
            //== End Display according to sub state machine returning to station

        case PROG_CHARGING:
            lcd.print("CHARGING SIDE ");
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
