#include <esp_now.h>
#include <WiFi.h>
#include "time.h"
#include <ESP_Google_Sheet_Client.h>

// For SD/SD_MMC mounting helper
#include <GS_SDHelper.h>

#define WIFI_SSID "wifi rumah"
#define WIFI_PASSWORD "12345678"

// Google Project ID
#define PROJECT_ID "agam-418710"

// Service Account's client email
#define CLIENT_EMAIL "agam-datalogging@agam-418710.iam.gserviceaccount.com"

// Service Account's private key
const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----\nMIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQCx4NiXFqG3Vhqj\nGd3ognEcolCQDuFgQjxhj3PVEEJ9JT7uMXbkZSfmOgJgw0uqe5rnfFstCYZwnXUv\nVGvzRvWWgNvdzYB7AL6UPVmflYNCIiSeQhE8DXBrYMWmk86jJqHRNKMwUhj2rv2y\nfdS5Xr+CY7fuq7wZk0I1wosn+qghlrm/qxjLKOIq23ee48KLZm4c9epscXQ4bTkR\n3oOjMHhQtZbV4r7/3h8oXmMm9D2KW9KUGXe/cWYMoUyj/dUE6TzPEOKEwJJchXdn\nL9wmgD3ALIWg9VdLI61zTEYmurbkA5uDXk8QXlfukWlKGjLGCxwmKPmxlmmAYkR8\nesUayNGdAgMBAAECggEACf8an6fCMrE4Vy0I41GnWoZbCYmIjCC/eh/Le5sfEso7\n0TuUP1gu/UQ1JivQ42Bwb2zLRp/tKgQzVdOvAGb0aNROPEFvDxqW20e1CHVTgS01\ninOoj8vVOEz3NqTCXWQPqS1F3t088WoasFlcJzHGXfp4LQXXkPFiwcOF5LCJxTS2\nM89lNo/OiavEMlAoVbpytDMdpGHTbRQjmcYV+YHYvVW1shpUT7l0YiCTYI4e3NIx\nEvNH/XdWZG4os+G5kBcG8768FwtgAhPHHHAUd+hrk6LZPF0QCW+dkKic+UHw0wkS\nUCbPEuMczgU9pTMsOjMlGAyW7vaVwFuq8VQ02N+qcQKBgQDxodKtO0KcKg/ZXZqs\nYMumO6PpUjY+0c96105wYUY2wQZ3a4hQJ4Jmi1443GYtmEk6G36sATZawlXj/aj8\nAMgRXqh/3mqVhwZ2VxeufFcRw/AvPdL5qMnFa+tfI9sr+d7eOvdMC7XX0Ms86ruN\ntA/JNUuMoLPo3RfdKMeKgZ9NMQKBgQC8dIxuFyuYDUYfMk+jkCniC4kBxMrD8uxO\n6eGVd2+vwfzLbeV4ZJB4Li+qyH16oCA84sNzM/VfiYfgXHBgV3rEXU8u42CFI/YR\nbyS/j4kq9kHrIoM2ma5i3/wRSUc048p12VDjGdJb+ZqpekaYO11ZVWfWvG65RE8m\nVRXvFzxALQKBgQDxI1h3bstjw4o7l3Fzk8nIpH5Iq7Xc7U+MvO/XsQv8Opf0d4j6\nzOMS6QTn4/PIjtCNP8EjcLZIzuYeeBe7HiCLTRcFPPbLMIwfb3z/sbDG1u6MhrtU\nUcDC15d2Q/dHJL7ospn29zzLGuCoc20m1kpmt1wlBH9m5bYaDMdOD1vmEQKBgCbf\nK3Ax204bvtEEgaTNQbZchsAUpXZRjyif2WYJIsJwEwKrAmAY7iu76x0hCbDXgo7M\nf2At4Xj5V0uSBaP5p1sFnCWhxPDxf/oMoNnac4KnFOW5UrNac7v1U3sFUGi6a7Jv\nRt3xH5DVJW+7xv/zjse0dBy49WXFj0Xq2wFcG5NpAoGAGCgt3bYYDYdWaGggeILO\nmoJtCMohtCFMM74V8GYeaS9sudwxz8I+zn/naCDUxVKPqNB40NDzEnNsZWXD4M49\nZZLxoGUFL8SyMJbxscalob6wE+b+6C2sDRYwpWfDYaGgBh2BmJoGKhszLNHOqQhA\ndYBGfSQ2NU+2RP2DUfyGOVM=\n-----END PRIVATE KEY-----\n";

// The ID of the spreadsheet where you'll publish the data
const char spreadsheetId[] = "1ZnUrUaKHIvUhIMwcBgIDaeEoy9tRrlfxdUb_scSf7JM";

// Timer variables
unsigned long lastTime = 0;  
unsigned long timerDelay = 0; // Timer for Delay

// Token Callback function
void tokenStatusCallback(TokenInfo info);

// NTP server to request epoch time
const char* ntpServer = "pool.ntp.org";

// Variable to save current epoch time
unsigned long epochTime; 

// Define variables to store incoming readings
float incomingOxy;
float incomingBpm;
float incomingTemp;

//Structure example to send data
//Must match the sender structure
typedef struct struct_message {
    float oxy;
    float bpm;
    float temp;
} struct_message;

// Create a struct_message to hold incoming sensor readings
struct_message incomingReadings;

// Flag to indicate if data is received
bool dataReceived = false;

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    //Serial.println("Failed to obtain time");
    return(0);
  }
  time(&now);
  return now;
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  //Serial.print("Bytes received: ");
  //Serial.println(len);
  incomingOxy = incomingReadings.oxy;
  incomingBpm = incomingReadings.bpm;
  incomingTemp = incomingReadings.temp;
  
  // Set dataReceived flag to true
  dataReceived = true;
}

void setup() {
  // Init Serial Monitor
  Serial.begin(115200);

  //Configure time
  configTime(0, 0, ntpServer); 

  GSheet.printf("ESP Google Sheet Client v%s\n\n", ESP_GOOGLE_SHEET_CLIENT_VERSION);
    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    //Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Set the callback for Google API access token generation status (for debug only)
  GSheet.setTokenCallback(tokenStatusCallback);

  // Set the seconds to refresh the auth token before expire (60 to 3540, default is 300 seconds)
  GSheet.setPrerefreshSeconds(10 * 60);

  // Begin the access token generation for Google API authentication
  GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);

  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);

}
 
void loop() {
  // Call updateDisplay() only if data is received
  if (dataReceived) {
    updateDisplay();
    // Reset dataReceived flag to false
    dataReceived = false;
  }
}

void updateDisplay(){  
        // Call ready() repeatedly in loop for authentication checking and processing
        // Connect to Wi-Fi
        //WiFi.setAutoReconnect(true);
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        //Serial.print("Connecting to Wi-Fi");
        while (WiFi.status() != WL_CONNECTED) {
        //Serial.print(".");
        //delay(1000);
        }
        //Serial.println();
       // Serial.print("Connected with IP: ");
        //Serial.println(WiFi.localIP());
        //Serial.println();
        //Serial.print("RSSI: ");
        //Serial.println(WiFi.RSSI());
        
  bool ready = GSheet.ready();

    if (ready && millis() - lastTime > timerDelay){
        lastTime = millis();

        FirebaseJson response;

        //Serial.println("\nAppend spreadsheet values...");
        //Serial.println("----------------------------");

        FirebaseJson valueRange;

        // Display Readings in Serial Monitor
        //Serial.println("INCOMING READINGS");
        Serial.print("oxygen: ");
        Serial.println(incomingReadings.oxy);
        Serial.print("heartrate: ");
        Serial.println(incomingReadings.bpm);
        Serial.print("Temperature: ");
        Serial.println(incomingReadings.temp);
        //Serial.println();

        // Get timestamp
        epochTime = getTime();
        
        valueRange.add("majorDimension", "COLUMNS");
        valueRange.set("values/[0]/[0]", epochTime);
        valueRange.set("values/[1]/[0]", incomingReadings.oxy);
        valueRange.set("values/[2]/[0]", incomingReadings.bpm);
        valueRange.set("values/[3]/[0]", incomingReadings.temp);

        // For Google Sheet API ref doc, go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/append
        // Append values to the spreadsheet
        bool success = GSheet.values.append(&response /* returned response */, spreadsheetId /* spreadsheet Id to append */, "Data_logging!A1" /* range to append */, &valueRange /* data range to append */);
        if (success){
            response.toString(Serial, true);
            valueRange.clear();
        }
        else{
            Serial.println(GSheet.errorReason());
        }
        //Serial.println();
        Serial.println(ESP.getFreeHeap());
    }
    // End Program then back to void setup
    Serial.println("END");
    //WiFi.disconnect(true);
    ESP.restart();
}

void tokenStatusCallback(TokenInfo info){
    if (info.status == token_status_error){
        GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
        GSheet.printf("Token error: %s\n", GSheet.getTokenError(info).c_str());
    }
    else{
        GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
    }
}
