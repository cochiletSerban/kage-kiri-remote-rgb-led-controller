#include <ESP8266WiFi.h>
#include <espnow.h>

// Broadcast MAC Address
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Using Pin 12 (GPIO 12)
const int BUTTON_PIN = 0; 

bool ledState = false;
unsigned long lastTriggerTime = 0;
const unsigned long lockoutDelay = 200; // Lock out readings for 200ms after a click

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Send Status: ");
  Serial.println(sendStatus == 0 ? "Success" : "Fail");
}
 
void setup() {
  Serial.begin(115200);
  
  // Wait for Serial Monitor on v4.0.0
  while (!Serial) {
    delay(10); 
  }
  delay(1000); 

  Serial.println("\n--- Transmitter Started ---");
  
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(OnDataSent);
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
  
  Serial.println("Ready to click!");
}
 
void loop() {
  // 1. Check if the button is pressed (LOW)
  // 2. Check if enough time has passed since the last valid click
  if (digitalRead(BUTTON_PIN) == LOW && (millis() - lastTriggerTime > lockoutDelay)) {
    
    // Update the timer instantly to lock out button bounce
    lastTriggerTime = millis();
    
    // Toggle the state
    ledState = !ledState;
    
    // Broadcast the command
    esp_now_send(broadcastAddress, (uint8_t *) &ledState, sizeof(ledState));
    
    Serial.print("Button Clicked! Sent LED State: ");
    Serial.println(ledState ? "ON" : "OFF");
  }
}