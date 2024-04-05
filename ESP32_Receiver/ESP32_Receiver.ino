#include <esp_now.h>
#include <WiFi.h>

// Define variables to store incoming readings
float incomingOxy;
float incomingTemp;

//Structure example to send data
//Must match the sender structure
typedef struct struct_message {
    float oxy;
    float temp;
} struct_message;

// Create a struct_message to hold incoming sensor readings
struct_message incomingReadings;

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  Serial.print("Bytes received: ");
  Serial.println(len);
  incomingOxy = incomingReadings.oxy;
  incomingTemp = incomingReadings.temp;
  updateDisplay();
}
 
void setup() {
  // Init Serial Monitor
  Serial.begin(115200);

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
}
 
void loop() {
  // Do nothing in the loop, all actions are performed in the callback function
  //updateDisplay();
  //delay(10000);
}

void updateDisplay(){  
  // Display Readings in Serial Monitor
  Serial.println("INCOMING READINGS");
  Serial.print("oxygen: ");
  Serial.println(incomingReadings.oxy);
  Serial.print("Temperature: ");
  Serial.println(incomingReadings.temp);
  Serial.println();
}
