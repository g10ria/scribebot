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

#define STEPPER_PITCH_1 34
#define STEPPER_PITCH_2 35
#define STEPPER_PITCH_3 36
#define STEPPER_PITCH_4 37

#define STEPPER_YAW_1 38
#define STEPPER_YAW_2 39
#define STEPPER_YAW_3 40
#define STEPPER_YAW_4 41  // swap these

#define START_BUTTON_PIN 49

LiquidCrystal_I2C lcd(0x27, 16, 2);

// steppers
AccelStepper stepperLeft(AccelStepper::DRIVER, STEPPER_LEFT_STEP, STEPPER_LEFT_DIR);     // clockwise is positive
AccelStepper stepperRight(AccelStepper::DRIVER, STEPPER_RIGHT_STEP, STEPPER_RIGHT_DIR);  // clockwise is positive
AccelStepper stepperZ(AccelStepper::DRIVER, STEPPER_Z_STEP, STEPPER_Z_DIR);              // up is positive

AccelStepper stepperYaw(8, STEPPER_YAW_1, STEPPER_YAW_3, STEPPER_YAW_2, STEPPER_YAW_4);

// constants
#define MAX_Y_TRAVEL 342.9    // in mm; 13.5"
#define MAX_X_TRAVEL 174.625  // in mm; 6.875"
#define MAX_Z_TRAVEL 25.4     // in mm; 1"
#define STEPS_PER_REV 200
#define COREXY_STEPS_PER_MM 40  // for 8 microsteps; change to 80 for 16
#define Z_STEPS_PER_MM 200

// state variables
float pitch_pose = 0;
float yaw_pose = 0;

float pitch_next = 0;
float yaw_next = 0;

#define NUM_LOCS 3

// DEFLOCS
float locs[NUM_LOCS][5] = {
  { 10, 30, 15, 0, 0},
  { 10, 30, 15, 1, .25 },
  { 10, 30, 15, 0 , 0}
  // { 150, 100, 0, 0 },
  // { 100, 100, 3, 0 },
  // { 65, 65, 20, 0 }
};

int currLoc = 0;
int numLocs = NUM_LOCS;

int pitch_step = 0;
bool pitch_direction = true;
int pitch_steps_remaining = 0;

int yaw_step = 0;
bool yaw_direction = false;
int yaw_steps_remaining = 0;

unsigned long timePrevious = 0;
unsigned long timeNow = 0;

float leftDesiredSpeed = 1600.0;
float rightDesiredSpeed = 1600.0;
float zDesiredSpeed = 800.0;
float pitchDesiredSpeed = 1000.0; // aka every 1000 micros
float yawDesiredSpeed = 1000.0; // aka every 1000 micros

// x, y is all we need right now
void moveCartesian(int currLoc) {
  float x_target = locs[currLoc][0];
  float y_target = locs[currLoc][1];
  float z_target = locs[currLoc][2];
  float pitch_target = locs[currLoc][3];
  float yaw_target = locs[currLoc][4];

  Serial.print("moving cartesian to: ");
  Serial.print(x_target);
  Serial.print(" ");
  Serial.print(y_target);
  Serial.print(" ");
  Serial.print(z_target);
  Serial.print(" ");
  Serial.print(pitch_target);
  Serial.print(" ");
  Serial.println(yaw_target);

  // syncing up the speeds; this one is a doozy lmao TODO!
  float stepperLeftTarget = y_target * COREXY_STEPS_PER_MM - x_target * COREXY_STEPS_PER_MM;
  float stepperRightTarget = -y_target * COREXY_STEPS_PER_MM - x_target * COREXY_STEPS_PER_MM;
  float stepperZTarget = z_target * Z_STEPS_PER_MM;

  float stepperLeftOffset = stepperLeftTarget - stepperLeft.currentPosition();
  float stepperRightOffset = stepperRightTarget - stepperLeft.currentPosition();
  float stepperZOffset = stepperZTarget - stepperZ.currentPosition();
  Serial.println(stepperLeftOffset);
  Serial.println(stepperRightOffset);
  Serial.println(stepperZOffset);

  float maxOffset = max(abs(stepperLeftOffset), abs(stepperRightOffset));

  float scaleFactor = 1600.0/maxOffset; // making them scaled relatively, one of them being 1600
  stepperLeft.setSpeed(scaleFactor * stepperLeftOffset);
  stepperRight.setSpeed(scaleFactor * stepperRightOffset);
  stepperLeft.setMaxSpeed(scaleFactor * stepperLeftOffset);
  stepperRight.setMaxSpeed(scaleFactor * stepperRightOffset);
  Serial.println("XY STEPPER SPEEDS");
  Serial.println(scaleFactor * stepperLeftOffset);
  Serial.println(scaleFactor * stepperRightOffset);
  Serial.println();

  stepperLeft.moveTo(stepperLeftTarget);
  stepperRight.moveTo(stepperRightTarget);
  stepperZ.moveTo(stepperZTarget);

  // pitch pose is in terms of steps
  float stepperPitchTarget = pitch_target * 1300.0; // todo comment this value and verify with zeroing jig
  float stepperPitchOffset = stepperPitchTarget - pitch_pose;
  pitch_next = stepperPitchTarget;
  Serial.println("pitch target info"); Serial.println(stepperPitchTarget); Serial.println(stepperPitchOffset);
  pitch_steps_remaining = abs(stepperPitchOffset);
  pitch_direction = stepperPitchOffset > 0;
  pitch_step = 0;

  // yaw pose is in terms of steps
  float stepperYawTarget = yaw_target * 8043.0;
  float stepperYawOffset = stepperYawTarget - yaw_pose;
  yaw_next = stepperYawTarget;
  Serial.println("yaw target info"); Serial.println(stepperYawTarget); Serial.println(stepperYawOffset);
  yaw_steps_remaining = abs(stepperYawOffset);
  yaw_direction = stepperYawOffset > 0;
  yaw_step = 0;

  float leftFastestTime = abs(stepperLeftOffset) / leftDesiredSpeed;
  float rightFastestTime = abs(stepperRightOffset) / rightDesiredSpeed;
  float zFastestTime = abs(stepperZOffset) / zDesiredSpeed;
  float pitchFastestTime = abs(stepperPitchOffset) / pitchDesiredSpeed;
  float yawFastestTime = abs(stepperYawOffset) / yawDesiredSpeed;

  float maximalTimeNeeded = max(leftFastestTime, max(rightFastestTime, max(zFastestTime, max(pitchFastestTime, yawFastestTime))));
  Serial.println("Max time needed: ");
  Serial.println(maximalTimeNeeded);
  Serial.println("Proposed speeds:");
  Serial.println(stepperLeftOffset/maximalTimeNeeded);
  Serial.println(stepperRightOffset/maximalTimeNeeded);
  Serial.println(stepperZOffset/maximalTimeNeeded);
  Serial.println(stepperPitchOffset/maximalTimeNeeded);
  Serial.println(stepperYawOffset/maximalTimeNeeded);

  // TODO implement min/max speed logic
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

  // set up pitch/yaw output pins
  pinMode(STEPPER_PITCH_1, OUTPUT);
  pinMode(STEPPER_PITCH_2, OUTPUT);
  pinMode(STEPPER_PITCH_3, OUTPUT);
  pinMode(STEPPER_PITCH_4, OUTPUT);

  pinMode(STEPPER_YAW_1, OUTPUT);
  pinMode(STEPPER_YAW_2, OUTPUT);
  pinMode(STEPPER_YAW_3, OUTPUT);
  pinMode(STEPPER_YAW_4, OUTPUT);

  // set up limit switch pins
  pinMode(Y_LIMIT_SWITCH_PIN, INPUT);
  pinMode(X_LIMIT_SWITCH_PIN, INPUT);
  pinMode(Z_LIMIT_SWITCH_PIN, INPUT);

  // home the system
  // home();
  // moveCartesian(0);

  lcd.init();
  lcd.backlight();
  display("hello scribebot!", 0, true);

  Serial.println("Finished setup!");
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

void updatePitchYawState() {
  pitch_pose = pitch_next;
  yaw_pose = yaw_next;
}

void updatePitchSteps() {
  // pitch
  if (pitch_direction == 1) {
    pitch_step++;
  } else if (pitch_direction == 0) {
    pitch_step--;
  }

  if (pitch_step > 7) { pitch_step = 0; }
  if (pitch_step < 0) { pitch_step = 7; }
}

void updateYawSteps() {
  if (yaw_direction == 1) {
    yaw_step++;
  } else if (yaw_direction == 0) {
    yaw_step--;
  }

  if (yaw_step > 7) { yaw_step = 0; }
  if (yaw_step < 0) { yaw_step = 7; }
}

void loop() {
  // return;
  // runMotors();
  // return;
  // while (stepsRemaining > 0)  // steps remaining until the full circle
  // {
  //   timeNow = micros();  //write down the current time in microseconds
  //   if (timeNow - timePrevious >= 2000) {
  //     stepPitch();  //call the stepper function (check void stepper below)
  //     stepYaw();
  //     timePrevious = micros();
  //     stepsRemaining--;
  //     updateSteps();
  //   }
  // }

  // Serial.println("Wait..!");
  // delay(1000);
  // direction = !direction;  // opposite direction
  // stepsRemaining = 300;    // reset the number of steps

  // return;

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
    } else {                  // we are mid job
      if (reachedTarget()) {  // update next target location

        updatePitchYawState();  // only necessary for pitch and yaw :P

        if (simulate_buffer) {
          // READ FROM SIMULATION ---------------------
          if (currLoc < numLocs) {
            moveCartesian(currLoc);
            currLoc++;
          } else {
            display("JOB COMPLETED!", 0, true);
            delay(1000);
            mid_job = false;

            currLoc = 0;
          }
          // READ FROM SIMULATION ---------------------
        } else {
          // READ FROM SD ---------------------
          if (numLinesRead < numLines) {  // job not done, keep reading into buffer
            updateSDBuffer();
          } else {  // we are done!
            display("JOB COMPLETED!", 0, true);
            delay(1000);
            mid_job = false;
          }
          // READ FROM SD ---------------------
        }
      }

      runMotors();
    }
  }
}

void stepPitch() {
  switch (pitch_step) {
    case 0:
      digitalWrite(STEPPER_PITCH_1, LOW);
      digitalWrite(STEPPER_PITCH_2, LOW);
      digitalWrite(STEPPER_PITCH_3, LOW);
      digitalWrite(STEPPER_PITCH_4, HIGH);
      break;
    case 1:
      digitalWrite(STEPPER_PITCH_1, LOW);
      digitalWrite(STEPPER_PITCH_2, LOW);
      digitalWrite(STEPPER_PITCH_3, HIGH);
      digitalWrite(STEPPER_PITCH_4, HIGH);
      break;
    case 2:
      digitalWrite(STEPPER_PITCH_1, LOW);
      digitalWrite(STEPPER_PITCH_2, LOW);
      digitalWrite(STEPPER_PITCH_3, HIGH);
      digitalWrite(STEPPER_PITCH_4, LOW);
      break;
    case 3:
      digitalWrite(STEPPER_PITCH_1, LOW);
      digitalWrite(STEPPER_PITCH_2, HIGH);
      digitalWrite(STEPPER_PITCH_3, HIGH);
      digitalWrite(STEPPER_PITCH_4, LOW);
      break;
    case 4:
      digitalWrite(STEPPER_PITCH_1, LOW);
      digitalWrite(STEPPER_PITCH_2, HIGH);
      digitalWrite(STEPPER_PITCH_3, LOW);
      digitalWrite(STEPPER_PITCH_4, LOW);
      break;
    case 5:
      digitalWrite(STEPPER_PITCH_1, HIGH);
      digitalWrite(STEPPER_PITCH_2, HIGH);
      digitalWrite(STEPPER_PITCH_3, LOW);
      digitalWrite(STEPPER_PITCH_4, LOW);
      break;
    case 6:
      digitalWrite(STEPPER_PITCH_1, HIGH);
      digitalWrite(STEPPER_PITCH_2, LOW);
      digitalWrite(STEPPER_PITCH_3, LOW);
      digitalWrite(STEPPER_PITCH_4, LOW);
      break;
    case 7:
      digitalWrite(STEPPER_PITCH_1, HIGH);
      digitalWrite(STEPPER_PITCH_2, LOW);
      digitalWrite(STEPPER_PITCH_3, LOW);
      digitalWrite(STEPPER_PITCH_4, HIGH);
      break;
    default:
      digitalWrite(STEPPER_PITCH_1, LOW);
      digitalWrite(STEPPER_PITCH_2, LOW);
      digitalWrite(STEPPER_PITCH_3, LOW);
      digitalWrite(STEPPER_PITCH_4, LOW);
      break;
  }
}

void stepYaw() {
  switch (yaw_step) {
    case 0:
      digitalWrite(STEPPER_YAW_1, LOW);
      digitalWrite(STEPPER_YAW_2, LOW);
      digitalWrite(STEPPER_YAW_3, LOW);
      digitalWrite(STEPPER_YAW_4, HIGH);
      break;
    case 1:
      digitalWrite(STEPPER_YAW_1, LOW);
      digitalWrite(STEPPER_YAW_2, LOW);
      digitalWrite(STEPPER_YAW_3, HIGH);
      digitalWrite(STEPPER_YAW_4, HIGH);
      break;
    case 2:
      digitalWrite(STEPPER_YAW_1, LOW);
      digitalWrite(STEPPER_YAW_2, LOW);
      digitalWrite(STEPPER_YAW_3, HIGH);
      digitalWrite(STEPPER_YAW_4, LOW);
      break;
    case 3:
      digitalWrite(STEPPER_YAW_1, LOW);
      digitalWrite(STEPPER_YAW_2, HIGH);
      digitalWrite(STEPPER_YAW_3, HIGH);
      digitalWrite(STEPPER_YAW_4, LOW);
      break;
    case 4:
      digitalWrite(STEPPER_YAW_1, LOW);
      digitalWrite(STEPPER_YAW_2, HIGH);
      digitalWrite(STEPPER_YAW_3, LOW);
      digitalWrite(STEPPER_YAW_4, LOW);
      break;
    case 5:
      digitalWrite(STEPPER_YAW_1, HIGH);
      digitalWrite(STEPPER_YAW_2, HIGH);
      digitalWrite(STEPPER_YAW_3, LOW);
      digitalWrite(STEPPER_YAW_4, LOW);
      break;
    case 6:
      digitalWrite(STEPPER_YAW_1, HIGH);
      digitalWrite(STEPPER_YAW_2, LOW);
      digitalWrite(STEPPER_YAW_3, LOW);
      digitalWrite(STEPPER_YAW_4, LOW);
      break;
    case 7:
      digitalWrite(STEPPER_YAW_1, HIGH);
      digitalWrite(STEPPER_YAW_2, LOW);
      digitalWrite(STEPPER_YAW_3, LOW);
      digitalWrite(STEPPER_YAW_4, HIGH);
      break;
    default:
      digitalWrite(STEPPER_YAW_1, LOW);
      digitalWrite(STEPPER_YAW_2, LOW);
      digitalWrite(STEPPER_YAW_3, LOW);
      digitalWrite(STEPPER_YAW_4, LOW);
      break;
  }
}

void runMotors() {
  stepperLeft.run();
  stepperRight.run();
  stepperZ.run();

  timeNow = micros();
  if (timeNow - timePrevious >= 1000) {  // attempt to step pitch (and yaw later)

    if (pitch_steps_remaining > 0) {
      // Serial.println("stepping pitch");
      stepPitch();
      timePrevious = micros();
      pitch_steps_remaining--;
      updatePitchSteps();
    }

    if (yaw_steps_remaining > 0) {
      // Serial.println("stepping yaw");
      stepYaw();
      timePrevious = micros();
      yaw_steps_remaining--;
      updateYawSteps();
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
  return stepperLeft.distanceToGo() == 0 && stepperRight.distanceToGo() == 0 && stepperZ.distanceToGo() == 0 && pitch_steps_remaining == 0 && yaw_steps_remaining == 0;
}

void estop() {  // todo make this an interrupt
  sd_inserted = false;
  mid_job = false;
}

void home() {
  display("Homing XY", 1, false);
  homeXY();
  display("Homing Z ", 1, false);
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
