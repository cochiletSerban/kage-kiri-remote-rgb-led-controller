#include <ESP8266WiFi.h>
#include <espnow.h>

const int LED_PIN = 5; 

// Callback function executed when data is received
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  bool receivedState;
  
  // Copy the incoming data into our local variable
  memcpy(&receivedState, incomingData, sizeof(receivedState));
  
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("LED State Command: ");
  Serial.println(receivedState);
  
  // Update the physical LED
  digitalWrite(LED_PIN, receivedState ? HIGH : LOW);
}
 
void setup() {
  Serial.begin(115200);
  
  // Initialize the LED pin as an output
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Start with LED off

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  // Initialize ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Define role and register the receive callback
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  // Nothing to do here, the callback handles everything asynchronously!
}