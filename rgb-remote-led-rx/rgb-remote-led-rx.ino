#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

// --- Hardware Pins ---
const int RED_PIN = 14;   
const int GREEN_PIN = 12; 
const int BLUE_PIN = 13;  

// --- The Data Structure (MUST match the Transmitter exactly) ---
struct RemotePacket {
  byte mode;         // 0 = Idle/Center, 1 = Brightness, 2 = Color
  int position;      // Current value of the rotary encoder
  byte clickEvent;   // 1 = Knob was clicked, 0 = No click
};

RemotePacket incomingPacket;

// --- Lighting State Variables ---
int currentBrightness = 255; // Start at full brightness
int currentHue = 0;          // Start at Red (0)
bool ledIsOn = true;         // Track if the light is toggled on or off

int lastKnownPosition[3] = {0, 0, 0}; // Tracks the remote's last encoder position per mode
const int BRIGHT_STEP = 15; // ~6% jumps (17 clicks to full brightness)
const int COLOR_STEP = 10;  // 25 clicks to sweep the entire rainbow

// --- Forward Declarations ---
void updateLED();
void setRGB(byte hue, byte brightness);

// --- The ESP-NOW Interrupt Callback ---
void onDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&incomingPacket, incomingData, sizeof(incomingPacket));

  // 1. Handle the Push Button (Toggle Power)
  if (incomingPacket.clickEvent == 1) {
    ledIsOn = !ledIsOn; 
    Serial.println(ledIsOn ? "💡 Power: ON" : "🌑 Power: OFF");
  }

  // 2. Calculate the "Delta" (How many clicks happened since the last packet?)
  // We use the mode (0, 1, or 2) as the index to grab the correct saved position
  int currentMode = incomingPacket.mode;
  int delta = incomingPacket.position - lastKnownPosition[currentMode];
  
  // Save the new position to calculate the next delta
  lastKnownPosition[currentMode] = incomingPacket.position; 

  // 3. Apply the multiplied delta to our actual lighting states
  if (currentMode == 1) {
    // ☀️ BRIGHTNESS MODE
    currentBrightness += (delta * BRIGHT_STEP);
    currentBrightness = constrain(currentBrightness, 0, 255);
    
    Serial.print("☀️ Brightness: "); Serial.println(currentBrightness);
  } 
  else if (currentMode == 2) {
    // 🎨 COLOR MODE
    currentHue += (delta * COLOR_STEP);
    
    // Wrap the color wheel cleanly around 0 and 255
    currentHue = (currentHue % 256 + 256) % 256; 
    
    Serial.print("🎨 Color Hue: "); Serial.println(currentHue);
  }

  // 4. Push the new calculated state directly to the LED pins
  updateLED();
}

void setup() {
  Serial.begin(115200);
  
  // Set pins as outputs
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  // ESP8266 PWM defaults to 1023 steps. We change it to 255 to perfectly match standard 8-bit color math.
  analogWriteRange(255); 

  // Initialize WiFi and ESP-NOW
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != 0) {
    Serial.println("⚠️ ESP-NOW Initialization Failed!");
    return;
  }
  
  // Register the receiver callback
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(onDataRecv);
  
  Serial.println("▶️ Receiver Ready. Waiting for remote commands...");
  
  // Light up the LED to the default starting state
  updateLED();
}

void loop() {
  // Loop remains completely empty! ESP-NOW handles everything via hardware interrupts.
}


// --- CORE LIGHTING LOGIC ---

void updateLED() {
  if (!ledIsOn) {
    // If the power is toggled off, kill all pins to 0V
    analogWrite(RED_PIN, 0);
    analogWrite(GREEN_PIN, 0);
    analogWrite(BLUE_PIN, 0);
  } else {
    // Feed the current hue and brightness into the color wheel math
    setRGB(currentHue, currentBrightness);
  }
}

// Lightweight RGB Color Wheel Math (Bypasses the need for heavy libraries)
void setRGB(byte wheelPos, byte brightness) {
  wheelPos = 255 - wheelPos;
  int r, g, b;
  
  // Sector 1: Red to Green
  if (wheelPos < 85) {
    r = 255 - wheelPos * 3;
    g = 0;
    b = wheelPos * 3;
  } 
  // Sector 2: Green to Blue
  else if (wheelPos < 170) {
    wheelPos -= 85;
    r = 0;
    g = wheelPos * 3;
    b = 255 - wheelPos * 3;
  } 
  // Sector 3: Blue to Red
  else {
    wheelPos -= 170;
    r = wheelPos * 3;
    g = 255 - wheelPos * 3;
    b = 0;
  }

  // Scale the raw colors down by the global brightness multiplier
  r = (r * brightness) / 255;
  g = (g * brightness) / 255;
  b = (b * brightness) / 255;

  // Output PWM signals to the pins
  analogWrite(RED_PIN, r);
  analogWrite(GREEN_PIN, g);
  analogWrite(BLUE_PIN, b);
}