#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>
#include "ESPRotary.h"
#include "Button2.h"

// --- Hardware Pins ---
const int pinA = 12;  
const int pinB = 14;  
const int pinSW = 13; 
const int switchLeftPin = 4;
const int switchRightPin = 5;

// --- Architecture Configuration ---
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Memory array to remember knob positions:
// [1] = Left Mode (Brightness), [2] = Right Mode (Color Wheel)
// (Index 0 is ignored since center mode has no logic)
int lastSavedEncoderPositions[3] = {0, 0, 0}; 

// --- The Simplified Data Structure ---
struct RemotePacket {
  byte mode;         // 0 = Idle/Center, 1 = Brightness (Left), 2 = Color (Right)
  int position;      // Current value of the rotary encoder
  byte clickEvent;   // 1 = Knob was clicked, 0 = No click
};

RemotePacket packet;

// --- Core Objects ---
ESPRotary r = ESPRotary(pinA, pinB);
Button2 encoderBtn = Button2(pinSW);
Button2 switchLeft = Button2(switchLeftPin);
Button2 switchRight = Button2(switchRightPin);

// --- Forward Declarations ---
void sendData();
void rotate(ESPRotary& re);
void encoderClick(Button2& b);
void leftPressed(Button2& b);
void rightPressed(Button2& b);
void switchReturnedToCenter(Button2& b);

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("--- Remote: Brightness & Color Mode ---");

  // Initialize ESP-NOW Protocol
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != 0) {
    Serial.println("⚠️ ESP-NOW Initialization Failed!");
    return;
  }
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);

  // Default System State (Booting into Idle/Center)
  packet.mode = 0;        
  packet.position = 0;

  // Setup Hardware Input Handlers
  r.setStepsPerClick(4); 
  r.setChangedHandler(rotate);
  encoderBtn.setPressedHandler(encoderClick);

  switchLeft.setPressedHandler(leftPressed);
  switchLeft.setReleasedHandler(switchReturnedToCenter);
  switchRight.setPressedHandler(rightPressed);
  switchRight.setReleasedHandler(switchReturnedToCenter);

  Serial.println("▶️ System Ready. Switch is in IDLE.");
}

void loop() {
  r.loop();
  encoderBtn.loop();
  switchLeft.loop();
  switchRight.loop();
}

// --- HELPER TRANSMIT FUNCTION ---
void sendData() {
  esp_now_send(broadcastAddress, (uint8_t *) &packet, sizeof(packet));
  packet.clickEvent = 0; // Clear immediately after sending
}

// --- ROTARY ENCODER EXECUTION ---
void rotate(ESPRotary& re) {
  // Only broadcast data if we are NOT in the center idle mode
  if (packet.mode != 0) {
    packet.position = re.getPosition();
    sendData();
    
    if (packet.mode == 1) Serial.print("☀️ Brightness Adjust: ");
    if (packet.mode == 2) Serial.print("🎨 Color Adjust: ");
    Serial.println(packet.position);
  } else {
    Serial.println("🔒 Remote is locked in Center Mode. Ignoring rotation.");
  }
}

// --- PUSH BUTTON INTERRUPT ---
void encoderClick(Button2& b) {
  packet.clickEvent = 1;
  sendData();
  Serial.println("🔘 Action Event Broadcasted!");
}

// --- 3-WAY TOGGLE SWITCH EXECUTIONS ---

void leftPressed(Button2& b) { 
  packet.mode = 1; 
  r.resetPosition(lastSavedEncoderPositions[1]);  // Recall previous Brightness value
  packet.position = r.getPosition();
  sendData();
  Serial.println("🎛️ Switched to: BRIGHTNESS MODE");
}

void rightPressed(Button2& b) { 
  packet.mode = 2; 
  r.resetPosition(lastSavedEncoderPositions[2]);  // Recall previous Color value
  packet.position = r.getPosition();
  sendData();
  Serial.println("🎛️ Switched to: COLOR MODE");
}

void switchReturnedToCenter(Button2& b) {
  delay(10); // Settle electrical bounce
  if (digitalRead(switchLeftPin) == HIGH && digitalRead(switchRightPin) == HIGH) {
    
    // Cache the values before entering the idle state
    if (packet.mode == 1) lastSavedEncoderPositions[1] = r.getPosition();
    if (packet.mode == 2) lastSavedEncoderPositions[2] = r.getPosition();

    packet.mode = 0; // Enter Idle Mode
    
    // We don't send data when returning to center, to keep the network quiet.
    Serial.println("🎛️ Switched to: CENTER (IDLE / LOCKED)");
  }
}