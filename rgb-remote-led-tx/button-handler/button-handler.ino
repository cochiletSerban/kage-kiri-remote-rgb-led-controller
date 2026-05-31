#include <Arduino.h>
#include "ESPRotary.h"
#include "Button2.h"

// Encoder Pins (Right side of board)
const int pinA = 12;  
const int pinB = 14;  
const int pinSW = 13; 

// 3-Way Switch Pins (Safe pins on the left side of USB)
const int switchLeftPin = 4;
const int switchRightPin = 5;

// Objects
ESPRotary r = ESPRotary(pinA, pinB);
Button2 encoderBtn = Button2(pinSW);

// Treat each side of the 3-way switch as an individual debounced button
Button2 switchLeft = Button2(switchLeftPin);
Button2 switchRight = Button2(switchRightPin);

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("--- Stable Encoder + 3-Way Switch ---");

  // Setup Encoder & its Button
  r.setStepsPerClick(4); 
  r.setChangedHandler(rotate);
  encoderBtn.setPressedHandler(encoderClick);

  // Setup 3-Way Switch with Library Handlers
  switchLeft.setPressedHandler(leftPressed);
  switchLeft.setReleasedHandler(switchReturnedToCenter);

  switchRight.setPressedHandler(rightPressed);
  switchRight.setReleasedHandler(switchReturnedToCenter);
}

void loop() {
  // Keep everything updating and debouncing in the background
  r.loop();
  encoderBtn.loop();
  switchLeft.loop();
  switchRight.loop();
}

// --- HANDLER FUNCTIONS ---

void rotate(ESPRotary& re) {
  Serial.print("🔄 Position: ");
  Serial.println(re.getPosition());
}

void encoderClick(Button2& b) {
  Serial.println("🔘 Encoder Knob Clicked!");
}

// Triggered ONLY when firmly toggled Left
void leftPressed(Button2& b) {
  Serial.println("🎛️ Switch: LEFT MODE");
}

// Triggered ONLY when firmly toggled Right
void rightPressed(Button2& b) {
  Serial.println("🎛️ Switch: RIGHT MODE");
}

// Triggered when letting go of either side back to the middle
void switchReturnedToCenter(Button2& b) {
  // A tiny delay ensures both sides have fully settled before declaring center
  delay(10); 
  if (digitalRead(switchLeftPin) == HIGH && digitalRead(switchRightPin) == HIGH) {
    Serial.println("🎛️ Switch: CENTER (OFF)");
  }
}