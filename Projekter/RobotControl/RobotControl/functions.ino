//=================================== Functions ================================
//
//
//
//
//==============================================================================
//Provide general functionality for main program. setup backend variables and functions

//== Variables for motorController =============================================
int current_speed_L = 0;            // Variable to hold the speed value
int desired_speed_L = 0;            // Variable to hold the decired speed
int last_Pos_L = 0;                 // Variable to hold the last possition
int current_speed_R = 0;
int desired_speed_R = 0;
int last_Pos_R = 0;

//== General P controller values ===============================================
double errorVal_L = 0.0;            // The direct error value for L
double errorVal_R = 0.0;            // The direct error value for R
double last_errorVal_L = 0.0;
double last_errorVal_R = 0.0;
double errorIntSum_L = 0.0;         // Integral sum L
double errorIntSum_R = 0.0;         // Integral sum R
double Kp_L = 15.0;                 // Proportional Kp - Left
double Kp_R = 15.0;                 // Proportional Kp - Right
double Ki_L = 8.0;                  // Integral Ki - Left
double Ki_R = 8.0;                  // Integral Ki - Right
double deltaT = 0.08;
int P_L = 0;                        // P output control value for motor L
int P_R = 0;                        // P output control value for motor R

//== Variables for tick counter on motor =======================================
int posVal_L = 0;
int posVal_R = 0;

//== Variables for the value where the timer will interrupt ====================
int timer1_counter = 60536;         // preload timer 65536-16MHz/256/100Hz(10ms). 59286(100ms), 60536(80ms).
int backwards_cyclics = 0;

//== Turn variables ============================================================
double degreeCalcConst = 20/180.0;  // Wheel ticks 50pr wheel, for 180 degree.    Update 20.
int turnStart_L = 0;
int turnStart_R = 0;
int turnRef = 0;
bool isMidtTurn = false;
bool isReverseTurning = false;

//== Interrupt routine handler for motor L, when encoder ticks =================
void enc_Int_Motor_L() {
    noInterrupts();                 // Disables interrupts
    posVal_L++;
    interrupts();                   // Enable interrupts
}
//== Interrupt routine handler for motor R, when encoder ticks =================
void enc_Int_Motor_R() {
    noInterrupts();                 // Disables interrupts
    posVal_R++;
    interrupts();                   // Enable interrupts
}

//== Interrupt service routine for handling the timer interrupt ================
ISR(TIMER1_OVF_vect) {
    TCNT1 = timer1_counter;         // preload timer
    motorControl();
}

//== seteup funcktion ==========================================================
void setup_func() {
    //== Setup pins for motors and sensors and initialise timer. ===============
    setup_pins_and_timer(timer1_counter);

    //== Enable interrupts on pin 2 and 4. =====================================
    enableInterrupt(ENC_L, enc_Int_Motor_L, CHANGE);
    enableInterrupt(ENC_R, enc_Int_Motor_R, CHANGE);
}

//============================== P controller ==================================
// This controller is set so that it assures constand speed on the motors
//==============================================================================
void motorControl() {
    //== Is turning check ======================================================
    if (isMidtTurn) {
      if (posVal_L >= turnRef + turnStart_L &&
          posVal_R >= turnRef + turnStart_R) {
              isMidtTurn = false;
              isReverseTurning = false;
          }
    }

    //== Calculating curent motor speed for weels given the ticks ==============
    current_speed_L = posVal_L - last_Pos_L;
    current_speed_R = posVal_R - last_Pos_R;

    //== Saving current weel position(tiks) for next calculation ===============
    last_Pos_L = posVal_L;
    last_Pos_R = posVal_R;

    //== Calculating error values ==============================================
    errorVal_L = desired_speed_L - current_speed_L; //Speed error
    errorVal_R = desired_speed_R - current_speed_R;

    //== Calculating intergral error from error values =========================
    errorIntSum_L += errorVal_L * deltaT;           //Integral
    errorIntSum_R += errorVal_R * deltaT;

    //== Calculating motor PWM from Proportional error plus intergral error ====
    P_L = static_cast<int>(errorVal_L*Kp_L   +   errorIntSum_L*Ki_L);
    P_R = static_cast<int>(errorVal_R*Kp_R   +   errorIntSum_R*Ki_R);

    //== Limits PWM output to the maximum and minimum signal ===================
    if (P_L > 255)
        P_L = 255;
    else if (P_L < 0)
        P_L = 0;
    if (P_R > 255)
        P_R = 255;
    else if (P_R < 0)
        P_R = 0;

    //== Makes sure to stop motors if disired speed is zerospeed ===============
    if (desired_speed_L == ZEROSPEED)
        P_L = 0;
    if (desired_speed_R == ZEROSPEED)
        P_R = 0;
    //== outputs PI controllers PWM signal on pind's ===========================
    analogWrite(PWM_L, abs(P_L));
    analogWrite(PWM_R, abs(P_R));
}

//== Utillyti function for easy setting motor direction and spped ==============
void set_motors(bool forward_L, bool forward_R, int speed_L, int speed_R) {
    if (!isMidtTurn){
        desired_speed_L = speed_L;
        desired_speed_R = speed_R;

        if (forward_L)
            digitalWrite(DIR_L, HIGH);
        else
            digitalWrite(DIR_L, LOW);

        // OBS NOTE:
        //The right motor was fliped on robot chassis one so forword is backward
        if (!forward_R)
            digitalWrite(DIR_R, HIGH);
        else
            digitalWrite(DIR_R, LOW);
        //== If the robot is backing strait back (Not turning) increment cyclics
        if (!forward_L && forward_R)
            backwards_cyclics++;
    }
}

//== Method initiates turn, controlled in the controller by the isMidtTurn flag=
//    Tels the robot to perform a turn on the spot with a determined angle
//    If turn_degrees == RANDOM_TURN, calculate random angle for turn_degrees
//if turn_speed == REVERSE_TURN, reverse the turning process to return to starting point
void startTurn(int turn_speed, int turn_degrees = RANDOM_TURN) {
    if (turn_speed == REVERSE_TURN && !isReverseTurning) {
        // Updating turnref from average turn completed by both wheels so far ==
        turnRef = (posVal_L - turnStart_L + posVal_R - turnStart_R) /2;
        turnStart_L = posVal_L;
        turnStart_R = posVal_R;
        //If desired_speed_L is negative, it should be positive and vice versa.
        set_motors(desired_speed_L < 0, desired_speed_R < 0, MEDIUMSPEED, MEDIUMSPEED);
        isReverseTurning = true;
    }
    else if (!isMidtTurn) {
        //If turn_degrees is set to RANDOM_TURN, a random turn angle is chosen
        if (turn_degrees == RANDOM_TURN)
            if(random(2) == 1)
                turn_degrees = random(MIN_RANDOM, MAX_RANDOM);
            else
                turn_degrees = -random(MIN_RANDOM, MAX_RANDOM);

        turnRef = abs(turn_degrees) * degreeCalcConst;
        turnStart_L = posVal_L;
        turnStart_R = posVal_R;

        if (turn_degrees > 0)
            set_motors(BACKWARD, FORWARD, turn_speed, turn_speed);
        else
            set_motors(FORWARD, BACKWARD, turn_speed, turn_speed);
    }
    isMidtTurn = true;
}

//== Check if turn is finished =================================================
bool turnFinished() {
    return !isMidtTurn;
}

//== When init wire follow, start by turning a little to orient robot better ===
void initFollowTurn(int chargerSide) {
    float sensor_val = 0;
    int turn_degree = 0;
    if (chargerSide == LEFT_CHARGERSIDE) {
        sensor_val = analogRead(BOUNDRYSENSOR_R);
        turn_degree = (sensor_val / BOUNDRY_CUTOFF_R) * -90;
    } else {
        sensor_val = analogRead(BOUNDRYSENSOR_L);
        turn_degree = (sensor_val / BOUNDRY_CUTOFF_L) * 90;
    }

    if (turn_degree > 20)
        startTurn(MEDIUMSPEED, turn_degree);
}
