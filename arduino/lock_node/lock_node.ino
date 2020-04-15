/*
 * Arduino Nano slave that controls multiple doors
 * Uses a single pin r/g LED to indicate permissions
 *   - Red (LOW) indicates locked; Green (HIGH) indicates unlocked; Blinking R/G indicates in use
 * Uses a SG90 servo for door locking mechanism
 * Uses a simple contact switch for door open/closed indicator
 * 
 * Receives instructions from master as to which doors to unlock
 * Does not have internal knowledge of what its doors do, only their state and usage metrics
 * Compiles metrics on last used and number of uses on EEPROM; reports and clears metrics every X minutes
 */

#include <Wire.h>
#include <Servo.h>

#define LED_PIN 2
#define I2C_ADDRESS 8
#define SERVO_PIN 4
#define DOOR_SWITCH 3

#define LOCK_POSITION LOW
#define UNLOCK_POSITION HIGH

byte lockState = LOW; // 0 Locked, 1 unlocked
byte doorClosed = LOW; // 0 Closed, 1 open
Servo lockMotor;


/**
 * On Boot
 * 1. Initialize with master
 * 2. Wait for list of tools from master (bytes == num of servos on node)
 * 
 */
void setup() {
  Serial.begin(9600);           /* start serial for debug */
  
  Wire.begin(I2C_ADDRESS);     /* join i2c bus with address */
  Wire.onReceive(receiveEvent); /* register receive event */
//  Wire.onRequest(requestEvent); /* register request event */

  pinMode(LED_PIN, OUTPUT); /* Initialize LED indicator */
  pinMode(DOOR_SWITCH, INPUT); /* Initialize Door indicator */
   
  lockMotor.attach(SERVO_PIN);/* Initialize Lock */
  lock();
}

void loop() {
  doorClosed = digitalRead(DOOR_SWITCH);
  
  if (lockState == HIGH) { // Unlock if told to
    unlock();
  } else if (doorClosed == LOW) { // Lock if door closed
    lock();
  } else {
//    while (digitalRead(DOOR_SWITCH) == HIGH) { // If door is left open, blink LED
//      digitalWrite(LED_PIN, LOW);
//      delay(500);
//      digitalWrite(LED_PIN, HIGH);
//      delay(500);
//    }
  }
  delay(100);
}

// function that executes whenever data is received from master
void receiveEvent(int numBytes) {
  lockState = Wire.read(); // Read just one byte 
}

// function that executes whenever data is requested from master
//void requestEvent() {
// Wire.write("Hello Master");  /*send current position on request */
//}

void lock() {
  digitalWrite(LED_PIN, LOW);
  lockMotor.write(0);
}

void unlock() {
  digitalWrite(LED_PIN, HIGH); // Unlock door for 10s
  lockMotor.write(90);
}
