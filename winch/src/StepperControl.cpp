#include "StepperControl.h"

StepperControl::StepperControl(LoRaComm& loraComm) : lora(loraComm) {}

void StepperControl::setup() {
    M5.begin();
    Serial.begin(115200);  // Initialize serial monitor

    // Configure the PWM settings
    ledcSetup(pwmChannel, pwmFrequency, pwmResolution);
    ledcAttachPin(pwmPin, pwmChannel);
    ledcWrite(pwmChannel, pwmValue);  // Set initial PWM duty cycle

    // Setup other GPIO pins
    pinMode(runStepperPin, OUTPUT);
    pinMode(dirStepperPin, OUTPUT);
    pinMode(limitSwitchPin, INPUT_PULLUP);
}

void StepperControl::runStepper() {
    if (motorState != RUNNING) {
        motorState = RUNNING;
        motorRunning = true;
        digitalWrite(runStepperPin, HIGH);
    }

    if (checkLimitSwitch()) {
        toggleMotorDirection();
    }
}

void StepperControl::stopAndCenter() {
    if (motorRunning) {
        motorRunning = false;
        motorState = CENTERING_TO_LIMIT;
        digitalWrite(runStepperPin, HIGH);  // Ensure motor is running
    }
}

void StepperControl::update() {
   // Serial.print("MotorState: ");
    //Serial.println(motorState);

    // Declare the variables outside the switch statement
    unsigned long currentTime = millis();
    unsigned long elapsedTime = 0;

    switch (motorState) {
        case RUNNING:
            if (checkLimitSwitch()) {
               // Serial.println("Limit switch hit in RUNNING state");
                toggleMotorDirection();
            }
            break;

        case CENTERING_TO_LIMIT:
            if (checkLimitSwitch()) {
               // Serial.println("Limit switch hit in CENTERING_TO_LIMIT state");
                toggleMotorDirection();
                motorState = CENTERING_BACK;
                centeringStartTime = millis();
                //Serial.print("Centering Start Time: ");
                //Serial.println(centeringStartTime);
            }
            break;

        case CENTERING_BACK:
            elapsedTime = currentTime - centeringStartTime;
           // Serial.print("ElapsedTime: ");
           // Serial.println(elapsedTime);
            if (elapsedTime < additionalRunTime) {
                if (checkLimitSwitch()) {
                    //Serial.println("Limit switch hit in CENTERING_BACK state");
                    toggleMotorDirection();
                }
            } else {
                motorState = IDLE;
                //Serial.println("Centering completed, setting motor to IDLE");
                digitalWrite(runStepperPin, LOW);
                motorRunning = false;
            }
            break;

        case IDLE:
        default:
            // Do nothing
            break;
    }
}


void StepperControl::updatePWM(int value) {
    pwmValue = value;
    ledcWrite(pwmChannel, pwmValue);
}

void StepperControl::setPwmValue(int value) {
    updatePWM(value);
}

void StepperControl::toggleMotorDirection() {
    motorDirection = !motorDirection;
    digitalWrite(dirStepperPin, motorDirection ? HIGH : LOW);
   // Serial.print("Motor direction toggled to: ");
   // Serial.println(motorDirection ? "Forward" : "Backward");
}

bool StepperControl::checkLimitSwitch() {
    static bool lastSwitchState = HIGH;
    bool currentSwitchState = digitalRead(limitSwitchPin);
    
    if (currentSwitchState != lastSwitchState && currentSwitchState == LOW) {
        if (millis() - lastDebounceTime > debounceDelay) {
            lastDebounceTime = millis();
            lastSwitchState = currentSwitchState;
           // Serial.println("Limit switch triggered");
            return true;
        }
    }
    lastSwitchState = currentSwitchState;
    return false;
}
