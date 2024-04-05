#include <esp_now.h>
#include <WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"

#define REPORTING_PERIOD_MS     1000

// Create a PulseOximeter object
PulseOximeter pox;

// Time at which the last beat occurred
uint32_t tsLastReport = 0;
// Data wire is plugged into port 23 on the Arduino
#define ONE_WIRE_BUS 23 // make a sure with your GPIO in ESP

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

DeviceAddress tempDeviceAddress;

int  resolution = 12;
unsigned long lastTempRequest = 0;
int  delayInMillis = 0;
float temperature = 0.0;
// Define variables to store BME280 readings to be sent
float oxygen;
float heartrate;

// REPLACE WITH THE MAC Address of your receiver 
uint8_t broadcastAddress[] = {0x40, 0x22, 0xD8, 0x3D, 0x20, 0x70};

// Variable to store if sending data was successful
String success;

//Structure example to send data
//Must match the receiver structure
typedef struct struct_message {
    float oxy;
    float bpm;
    float temp;
} struct_message;

// Create a struct_message called SensorsReadings to hold sensor readings
struct_message SensorsReadings;

esp_now_peer_info_t peerInfo;

void configureMax30100() {
  // Configure sensor to use 7.6mA for LED drive
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
  // Register a callback routine
  pox.setOnBeatDetectedCallback(onBeatDetected);
}

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status ==0){
    success = "Delivery Success :)";
  }
  else{
    success = "Delivery Fail :(";
  }
}

void sendtoesp(){
  // Set values to send
  SensorsReadings.oxy = oxygen;
  SensorsReadings.temp = temperature;
  SensorsReadings.bpm = heartrate;

    // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &SensorsReadings, sizeof(SensorsReadings));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
}

// Callback routine is executed when a pulse is detected
void onBeatDetected() {
  //Serial.println("Beat!");
  // Grab the updated heart rate and SpO2 levels
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    oxygen = pox.getSpO2();
    heartrate = pox.getHeartRate();
    tsLastReport = millis();
  }
    
    if (millis() - lastTempRequest >= 20) // waited long enough??
  {
    temperature = sensors.getTempCByIndex(0), resolution - 8;

    resolution++;
    if (resolution > 12) resolution = 9;

    sensors.setResolution(tempDeviceAddress, resolution);
    sensors.requestTemperatures();
    delayInMillis = 20 / (1 << (12 - resolution));
    lastTempRequest = millis();
  }
    Serial.println("READINGS Sensors");
    Serial.print("oxygen: ");
    Serial.println(oxygen);
    Serial.print("heartrate: ");
    Serial.println(heartrate);
    Serial.print("temperature: ");
    Serial.println(temperature);
    sendtoesp();
}

void setup(void)
{
  Serial.begin(115200);
  sensors.begin();
  sensors.getAddress(tempDeviceAddress, 0);
  sensors.setWaitForConversion(false);

  Wire.setClock(400000UL); // I tried changing the I2C_BUS_SPEED to 100Khz, it made no difference in the output values
  // Initialize sensor
  if (!pox.begin()) {
    Serial.println("FAILED");
    for (;;);
  } else {
    Serial.println("SUCCESS");
  }

  configureMax30100();

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}

void loop(void)
{
  // update data MAX100
  pox.update();
}