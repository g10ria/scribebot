#include <AccelStepper.h>
#include <SPI.h>
#include <SD.h>
#include <LiquidCrystal_I2C.h>

// defining pins
#define Y_LIMIT_SWITCH_PIN 22
#define X_LIMIT_SWITCH_PIN 23
#define Z_LIMIT_SWITCH_PIN 24

#define STEPPER_LEFT_STEP 28
#define STEPPER_LEFT_DIR 29

#define STEPPER_RIGHT_STEP 30
#define STEPPER_RIGHT_DIR 31

#define STEPPER_Z_DIR 32
#define STEPPER_Z_STEP 33

#define START_BUTTON_PIN 49 
#define ESTOP_BUTTON_PIN 1000 // fix

LiquidCrystal_I2C lcd(0x27, 16, 2); // lcd fix maybe

// steppers
AccelStepper stepperLeft(AccelStepper::DRIVER, STEPPER_LEFT_STEP, STEPPER_LEFT_DIR);   // clockwise is positive
AccelStepper stepperRight(AccelStepper::DRIVER, STEPPER_RIGHT_STEP, STEPPER_RIGHT_DIR);  // clockwise is positive
AccelStepper stepperZ(AccelStepper::DRIVER, STEPPER_Z_STEP, STEPPER_Z_DIR); // up is positive

/*
COREXY TRUTH TABLE:
Y MOVEMENT:
back: left is negative, right is positive
forward: left is positive, right is negative

X MOVEMENT:
left: both positive
right: both negative

OTHER AXES:
z: up is positive
*/

// constants
#define MAX_Y_TRAVEL 342.9    // in mm; 13.5"
#define MAX_X_TRAVEL 174.625  // in mm; 6.875"
#define MAX_Z_TRAVEL 25.4     // in mm; 1"
#define STEPS_PER_REV 200
#define COREXY_STEPS_PER_MM 40 // for 8 microsteps; change to 80 for 16
#define Z_STEPS_PER_MM 200

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

  stepperLeft.moveTo(y_target * COREXY_STEPS_PER_MM - x_target * COREXY_STEPS_PER_MM);
  stepperRight.moveTo(-y_target * COREXY_STEPS_PER_MM - x_target * COREXY_STEPS_PER_MM);

  x_next = x_target;
  y_next = y_target;
}

void setup() {
  Serial.begin(9600);

  // initialize steppers
  stepperLeft.setMaxSpeed(1600);
  stepperLeft.setAcceleration(800);

  stepperRight.setMaxSpeed(1600);
  stepperRight.setAcceleration(800);

  stepperLeft.setSpeed(1600);
  stepperRight.setSpeed(1600);

  stepperZ.setMaxSpeed(1600);
  stepperZ.setAcceleration(800);
  stepperZ.setSpeed(800);

  // set up limit switch pins
  pinMode(Y_LIMIT_SWITCH_PIN, INPUT);
  pinMode(X_LIMIT_SWITCH_PIN, INPUT);
  pinMode(Z_LIMIT_SWITCH_PIN, INPUT);

  // home the system
  home();

  // moveCartesian(20, 20);

  // todo: attach interrupt to estop button

  lcd.init();
  lcd.backlight();
  display("hello scribebot!", 0, true);
}

#define GCODE_LINE_LENGTH 41

File file;
char buf[GCODE_LINE_LENGTH]; // last two lines are carriage return + newline
int numLines = 0;
int numLinesRead = 0;

bool sd_inserted = false;
bool mid_job = false;

bool button_pressed_temp = true;

void loop() {
  stepperZ.run();
  // Serial.println(digitalRead(Z_LIMIT_SWITCH_PIN));
  // if(!sd_inserted) { // sd not inserted yet, check if inserted
  //   display("Waiting for SD...", 0, false);
  //   int sd_inserted_check = SD.begin();
  //   if (sd_inserted_check==0) { // no SD card
  //     Serial.println("no sd, waiting 1 second...");
  //     delay(1000); // wait one second before trying again
  //   } else {
  //     display("SD inserted!", 0, true);
  //     display("Press button :)", 1, false);
  //     sd_inserted = true;
  //     // add an interrupt for when the SD is removed? this can come later lol
  //   }
  // } else { // sd is now inserted

  //   if (!mid_job) { // not mid job, check for button press
  //     if (digitalRead(START_BUTTON_PIN)==HIGH) { // button is pressed; start SD buffer and start job
  //       mid_job = true;
  //       display("JOB IN PROGRESS", 0, true);
  //       display("Homing...", 1, false);
  //       home();
  //       display("Executing...", 1, false);
  //       startSDBuffer();
  //     }
  //   } else { // we are mid job
  //     if (reachedTarget()) {
  //       if (numLinesRead < numLines) { // job not done, keep reading into buffer
  //         updateSDBuffer();
  //       } else { // we are done! 
  //         display("JOB COMPLETED!", 0, true);
  //         delay(1000);
  //         mid_job = false;
  //       }
  //     }
  //     stepperLeft.runSpeedToPosition();
  //     stepperRight.runSpeedToPosition();
  //   }
  // }
}

// row is 0 or 1, max 16 chars per msg
void display(String msg, int row, bool clearScreen) {
  if (clearScreen) lcd.clear();
  lcd.setCursor(0, row);
  lcd.print(msg);
}

void startSDBuffer() { // reads first line
  file = SD.open("test.txt", FILE_READ);

  numLines = file.size()/GCODE_LINE_LENGTH;
  Serial.print(numLines);
  Serial.println(" lines");

  file.read(buf, GCODE_LINE_LENGTH);
  numLinesRead = 1;

  updateTargetFromSDBuffer();
}

void updateSDBuffer() {
  file.read(buf, GCODE_LINE_LENGTH);
  numLinesRead++;

  Serial.print("read line ");
  Serial.println(numLinesRead);
  Serial.println(buf);
}

// updates target location from SD buffer
void updateTargetFromSDBuffer() {
  // for now, just go to a random spot
}

bool reachedTarget() { // todo; rn just waits one second and returns true lmfao
  Serial.println("reached target test 1");
  delay(1000);
  Serial.println("reached target test 2");
  return true;
  // return (stepperLeft.distanceToGo() == 0);
}

void estop() { // todo make this an interrupt
  sd_inserted = false;
  mid_job = false;
}

void home() {
  display("Homing XY", 1, false);
  homeXY();
  display("Homing Z", 1, false);
  homeZ();
}

void homeZ() {
  Serial.println("Homing Z...");
  delay(1000);

  stepperZ.moveTo(-(MAX_Z_TRAVEL+50) * Z_STEPS_PER_MM);
  
  int needed_zero_count = 5;

  int z_zero_count = 0;

  while (z_zero_count < needed_zero_count) { // protection against noise
    // stepperZ.runSpeedToPosition();
    stepperZ.run(); // idk why runSpeedToPosition doesn't work here tbh

    if (digitalRead(Z_LIMIT_SWITCH_PIN) == LOW) {
      z_zero_count++;
    } else {
      z_zero_count = 0;
    }
  }
  stepperZ.stop();
  stepperZ.setCurrentPosition(0);

  Serial.println("z is homed!");
}

void homeXY() {
  Serial.print("Homing Y...");
  delay(1000);

  stepperLeft.moveTo(-(MAX_Y_TRAVEL+50) * COREXY_STEPS_PER_MM);
  stepperRight.moveTo((MAX_Y_TRAVEL+50) * COREXY_STEPS_PER_MM);

  int needed_zero_count = 5;

  int y_zero_count = 0;

  while (y_zero_count < needed_zero_count) { // protection against noise
    stepperLeft.runSpeedToPosition ();
    stepperRight.runSpeedToPosition ();

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

  stepperLeft.moveTo((MAX_X_TRAVEL+50) * COREXY_STEPS_PER_MM);
  stepperRight.moveTo((MAX_X_TRAVEL+50) * COREXY_STEPS_PER_MM);

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
