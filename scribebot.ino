#include <AccelStepper.h>
#include <SPI.h>
#include <SD.h>
#include <LiquidCrystal_I2C.h>

// defining pins
#define X_LIMIT_SWITCH_PIN 22
#define Y_LIMIT_SWITCH_PIN 23
#define Z_LIMIT_SWITCH_PIN 25

#define STEPPER_LEFT_STEP 28
#define STEPPER_LEFT_DIR 29

#define STEPPER_RIGHT_STEP 30
#define STEPPER_RIGHT_DIR 31

#define STEPPER_Z_STEP 32
#define STEPPER_Z_DIR 33

#define START_BUTTON_PIN 49

LiquidCrystal_I2C lcd(0x27, 16, 2);

// steppers
AccelStepper stepperLeft(AccelStepper::DRIVER, STEPPER_LEFT_STEP, STEPPER_LEFT_DIR);     // clockwise is positive
AccelStepper stepperRight(AccelStepper::DRIVER, STEPPER_RIGHT_STEP, STEPPER_RIGHT_DIR);  // clockwise is positive
AccelStepper stepperZ(AccelStepper::DRIVER, STEPPER_Z_STEP, STEPPER_Z_DIR);              // up is positive

// AccelStepper stepperPitch(AccelStepper::FULL4WIRE, 34, 35, 36, 37);
// AccelStepper stepperPitch(8, 8,10,9,11);

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
#define COREXY_STEPS_PER_MM 40  // for 8 microsteps; change to 80 for 16
#define Z_STEPS_PER_MM 200

// state variables
float x_pose = 0;
float y_pose = 0;
float z_pose = 0;

float x_next = 0;
float y_next = 0;
float z_next = 0;

// const int stepsPerRevolution = 2038;
// Stepper myStepper = Stepper(stepsPerRevolution, 8, 10, 9, 11);

float locs[7][3] = {
  { 30, 30, 20 },
  { 100, 100, 0 },
  { 100, 150, 5 },
  { 150, 150, 10 },
  { 150, 100, 0 },
  { 100, 100, 3 },
  { 65, 65, 20 }
};

int currLoc = 0;
int numLocs = 7;

// x, y is all we need right now
void moveCartesian(float x_target, float y_target, float z_target) {  // in mm
  Serial.print("moving cartesian to: ");
  Serial.print(x_target);
  Serial.print(" ");
  Serial.print(y_target);
  Serial.print(" ");
  Serial.println(z_target);

  stepperLeft.moveTo(y_target * COREXY_STEPS_PER_MM - x_target * COREXY_STEPS_PER_MM);
  stepperRight.moveTo(-y_target * COREXY_STEPS_PER_MM - x_target * COREXY_STEPS_PER_MM);
  stepperZ.moveTo(z_target * Z_STEPS_PER_MM);

  x_next = x_target;
  y_next = y_target;
  z_next = z_target;
}

void setup() {
  Serial.begin(9600);

  // initialize steppers
  stepperLeft.setMaxSpeed(1600);
  stepperLeft.setAcceleration(10000);

  stepperRight.setMaxSpeed(1600);
  stepperRight.setAcceleration(10000);

  stepperLeft.setSpeed(1600);
  stepperRight.setSpeed(1600);

  stepperZ.setMaxSpeed(1600);
  stepperZ.setAcceleration(10000);
  stepperZ.setSpeed(800);

  // stepperRight.moveTo(400);
  // stepperLeft.moveTo(400);
  // stepperZ.moveTo(100);

  // stepperPitch.setMaxSpeed(1000000);
  // stepperPitch.setAcceleration(1000000);
  // stepperPitch.setSpeed(1000000);

  // stepperPitch.moveTo(2048);

  // set up limit switch pins
  pinMode(Y_LIMIT_SWITCH_PIN, INPUT);
  pinMode(X_LIMIT_SWITCH_PIN, INPUT);
  pinMode(Z_LIMIT_SWITCH_PIN, INPUT);

  // home the system
  // home();

  lcd.init();
  lcd.backlight();
  display("hello scribebot!", 0, true);

  Serial.println("hello!");
}

#define GCODE_LINE_LENGTH 41

File file;
char buf[GCODE_LINE_LENGTH];  // last two lines are carriage return + newline
int numLines = 0;
int numLinesRead = 0;

bool sd_inserted = false;
bool mid_job = false;

bool button_pressed_temp = true;

bool simulate_buffer = true;

void loop() {
  if (!sd_inserted) {  // sd not inserted yet, check if inserted
    Serial.println("waiting for sd");
    display("Waiting for SD...", 0, false);
    int sd_inserted_check = SD.begin();
    if (sd_inserted_check == 0) {  // no SD card
      Serial.println("no sd, waiting 1 second...");
      delay(1000);  // wait one second before trying again
    } else {
      Serial.println("sd inserted!");
      display("SD inserted!", 0, true);
      display("Press button :)", 1, false);
      sd_inserted = true;
      // add an interrupt for when the SD is removed? this can come later lol
    }
  } else {  // sd is now inserted

    if (!mid_job) {                                 // not mid job, check for button press
      if (digitalRead(START_BUTTON_PIN) == HIGH) {  // button is pressed; start SD buffer and start job
        mid_job = true;
        display("JOB IN PROGRESS", 0, true);
        display("Homing...", 1, false);
        home();
        display("Executing...", 1, false);
        if (!simulate_buffer) {
          startSDBuffer();
        }
      }
    } else {  // we are mid job
      if (reachedTarget()) {
        if (simulate_buffer) {
          // stepperLeft.setSpeed(800);
          // stepperRight.setSpeed(800);
          // stepperZ.setSpeed(800);
          if (currLoc < numLocs) {
            moveCartesian(locs[currLoc][0], locs[currLoc][1], locs[currLoc][2]);
            currLoc++;
          } else {
            display("JOB COMPLETED!", 0, true);
            delay(1000);
            mid_job = false;

            currLoc = 0;
          }
        } else {
          if (numLinesRead < numLines) {  // job not done, keep reading into buffer
            updateSDBuffer();
          } else {  // we are done!
            display("JOB COMPLETED!", 0, true);
            delay(1000);
            mid_job = false;
          }
        }
      }
      stepperLeft.run();
      stepperRight.run();
      stepperZ.run();
    }
  }
}

// row is 0 or 1, max 16 chars per msg
void display(String msg, int row, bool clearScreen) {
  if (clearScreen) lcd.clear();
  lcd.setCursor(0, row);
  lcd.print(msg);
}

void startSDBuffer() {  // reads first line
  file = SD.open("test.txt", FILE_READ);

  numLines = file.size() / GCODE_LINE_LENGTH;
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

bool reachedTarget() {
  return stepperLeft.distanceToGo() == 0 && stepperRight.distanceToGo() == 0 && stepperZ.distanceToGo() == 0;
}

void estop() {  // todo make this an interrupt
  sd_inserted = false;
  mid_job = false;
}

void home() {
  // display("Homing XY", 1, false);
  homeXY();
  Serial.println("asklfjadslk");
  // display("Homing Z", 1, false);
  homeZ();
}

void homeZ() {
  Serial.println("Homing Z...");
  delay(1000);

  stepperZ.moveTo(-(MAX_Z_TRAVEL + 50) * Z_STEPS_PER_MM);

  int needed_zero_count = 1;

  int z_zero_count = 0;

  while (z_zero_count < needed_zero_count) {  // protection against noise
    stepperZ.run();

    if (digitalRead(Z_LIMIT_SWITCH_PIN) == LOW) {
      z_zero_count++;
      // todo stop stepper z
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

  stepperLeft.moveTo(-(MAX_Y_TRAVEL + 50) * COREXY_STEPS_PER_MM);
  stepperRight.moveTo((MAX_Y_TRAVEL + 50) * COREXY_STEPS_PER_MM);

  int needed_zero_count = 5;

  int y_zero_count = 0;

  while (y_zero_count < needed_zero_count) {  // protection against noise
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

  stepperLeft.moveTo((MAX_X_TRAVEL + 50) * COREXY_STEPS_PER_MM);
  stepperRight.moveTo((MAX_X_TRAVEL + 50) * COREXY_STEPS_PER_MM);

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

  Serial.println("Reached X limit switch, zeroing motors...");

  stepperLeft.setCurrentPosition(0);
  stepperRight.setCurrentPosition(0);
  return;
}
