#include <AccelStepper.h>

// defining pins
#define STEPPER_LEFT_STEP = 4;
#define STEPPER_LEFT_DIR = 5;

#define STEPPER_RIGHT_STEP = 2;
#define STEPPER_RIGHT_DIR = 3;

// #define STEPPER_Z_STEP = 6;
// #define STEPPER_Z_DIR = 7;

#define Y_LIMIT_SWITCH_PIN 8
#define X_LIMIT_SWITCH_PIN 12

// steppers
AccelStepper stepperLeft(AccelStepper::DRIVER, STEPPER_LEFT_STEP, STEPPER_LEFT_DIR);   // clockwise is positive
AccelStepper stepperRight(AccelStepper::DRIVER, STEPPER_RIGHT_STEP, STEPPER_RIGHT_DIR);  // clockwise is positive

/*
COREXY TRUTH TABLE:
Y MOVEMENT:
back: left is negative, right is positive
forward: left is positive, right is negative

X MOVEMENT:
left: both positive
right: both negative
*/

// constants
#define MAX_Y_TRAVEL 342.9    // in mm; 13.5"
#define MAX_X_TRAVEL 174.625  // in mm; 6.875"
#define STEPS_PER_REV 200
#define STEPS_PER_MM 40       // for xy, for 8 microsteps; change to 80 for 16 (change to 16)

// state variables
float x_pose = 0;
float y_pose = 0;

float x_next = 0;
float y_next = 0;

// x, y is all we need right now
void moveCartesian(float x_target, float y_target) {  // in mm
  Serial.print("moving cartesian to: ");
  Serial.print(x_target);
  Serial.print(" ");
  Serial.println(y_target);

  stepperLeft.moveTo(y_target * STEPS_PER_MM - x_target * STEPS_PER_MM);
  stepperRight.moveTo(-y_target * STEPS_PER_MM - x_target * STEPS_PER_MM);

  x_next = x_target;
  y_next = y_target;
}

void setup() {
  Serial.begin(9600);

  // initialize steppers
  stepperLeft.setMaxSpeed(1000);
  stepperLeft.setAcceleration(800);

  stepperRight.setMaxSpeed(1000);
  stepperRight.setAcceleration(800);

  stepperLeft.setSpeed(1000);
  stepperRight.setSpeed(1000);

  Serial.println(stepperLeft.currentPosition());
  Serial.println(stepperRight.currentPosition());

  // set up limit switch pins
  pinMode(Y_LIMIT_SWITCH_PIN, INPUT);
  pinMode(X_LIMIT_SWITCH_PIN, INPUT);

  // home the system
  homeXY();

  // moveCartesian(20, 20);
}


void loop() {
  stepperLeft.run();
  stepperRight.run();
}

// void homeZ() {
//   Serial.print("Homing Z...");
//   delay(1000);
//   stepperZ.setSpeed(1000);
//   // etc
// }

void homeXY() {
  Serial.print("Homing Y...");
  delay(1000);

  stepperLeft.moveTo(-MAX_Y_TRAVEL * STEPS_PER_MM);
  stepperRight.moveTo(MAX_Y_TRAVEL * STEPS_PER_MM);

  int needed_zero_count = 5;

  int y_zero_count = 0;

  while (y_zero_count < needed_zero_count) { // protection against noise
    stepperLeft.run();
    stepperRight.run();

    if (digitalRead(Y_LIMIT_SWITCH_PIN) == LOW) {
      y_zero_count++;
    } else {
      y_zero_count = 0;
    }
  }
  stepperLeft.stop();
  stepperRight.stop();

  Serial.print("Reached Y limit switch, now homing X...");
  stepperLeft.setCurrentPosition(0);
  stepperRight.setCurrentPosition(0);
  delay(1000);

  stepperLeft.moveTo(MAX_X_TRAVEL * STEPS_PER_MM);
  stepperRight.moveTo(MAX_X_TRAVEL * STEPS_PER_MM);

  int x_zero_count = 0;
  
  while (x_zero_count < needed_zero_count) {
    stepperLeft.run();
    stepperRight.run();

    if (digitalRead(X_LIMIT_SWITCH_PIN) == HIGH) {
      x_zero_count = 0;
    } else {
      x_zero_count++;
    }
  }
  stepperLeft.stop();
  stepperRight.stop();

  Serial.print("Reached X limit switch, zeroing motors...");

  stepperLeft.setCurrentPosition(0);
  stepperRight.setCurrentPosition(0);
}
