#include "BLEDevice.h"       // Include the BLE library for Bluetooth Low Energy functionality
#include <M5StickCPlus.h>    // Include the M5StickCPlus library for the M5StickC Plus device

// BLE UUIDs - Unique identifiers for BLE services and characteristics
#define bleServerName "candelario"  // Name of the BLE server to connect to
#define SERVICE_UUID "021265ed-4111-4da7-823f-c03ac45492f8"  // UUID for the main service
#define TEMPERATURE_UUID "6356a54c-0d62-4c4b-9581-6c078120fafc"  // UUID for temperature characteristic
#define VOLTAGE_UUID "24a4975c-e835-423b-b987-f6321cdfb663"  // UUID for voltage characteristic
#define LED_STATE_UUID "12345678-1234-5678-1234-56789abcdef0"  // UUID for LED state characteristic

// BLE client objects and characteristics
BLEClient* pClient;                                     // BLE client instance
BLERemoteCharacteristic* temperatureCharacteristic;     // Remote characteristic for temperature data
BLERemoteCharacteristic* voltageCharacteristic;         // Remote characteristic for voltage data
BLERemoteCharacteristic* ledCharacteristic;             // Remote characteristic for LED control
BLEAddress *pServerAddress;                             // Address of the BLE server

// Connection state flags
bool doConnect = false;    // Flag to trigger connection attempt
bool connected = false;    // Flag to track connection status

// Function to handle notification callbacks - Called when server sends notifications
void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  char value[length + 1]; // Create a buffer with an extra byte for null-termination
  memcpy(value, pData, length); // Copy received data into the buffer
  value[length] = '\0'; // Null-terminate the string

  // Check which characteristic sent the notification and update the display accordingly
  if (pBLERemoteCharacteristic->getUUID().toString() == TEMPERATURE_UUID) {
    // Display temperature on the M5StickC Plus LCD
    M5.Lcd.setCursor(0, 40, 2);
    M5.Lcd.fillRect(0, 40, 160, 20, BLACK); // Clear previous text
    M5.Lcd.printf("Temp: %s C", value);
    Serial.printf("Received Temperature: %s C\n", value);
  } else if (pBLERemoteCharacteristic->getUUID().toString() == VOLTAGE_UUID) {
    // Display voltage on the M5StickC Plus LCD
    M5.Lcd.setCursor(0, 60, 2);
    M5.Lcd.fillRect(0, 60, 160, 20, BLACK); // Clear previous text
    M5.Lcd.printf("Volt: %s V", value);
    Serial.printf("Received Voltage: %s V\n", value);
  }
}

// Function to connect to the BLE server
bool connectToServer() {
  Serial.println("Attempting to connect to the server...");
  M5.Lcd.setCursor(0, 20, 2);
  M5.Lcd.printf("Connecting to server...");

  // Create a BLE client
  pClient = BLEDevice::createClient();
  if (!pClient->connect(*pServerAddress)) {
    Serial.println("Failed to connect to server.");
    return false;
  }

  Serial.println("Connected to server.");
  M5.Lcd.setCursor(0, 40, 2);
  M5.Lcd.printf("Connected to server");

  // Get the service from the server using the service UUID
  BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
  if (!pRemoteService) {
    Serial.println("Service not found.");
    pClient->disconnect();
    return false;
  }

  // Retrieve characteristics from the service using their UUIDs
  temperatureCharacteristic = pRemoteService->getCharacteristic(TEMPERATURE_UUID);
  voltageCharacteristic = pRemoteService->getCharacteristic(VOLTAGE_UUID);
  ledCharacteristic = pRemoteService->getCharacteristic(LED_STATE_UUID);

  // Register for notifications if all characteristics are found
  if (temperatureCharacteristic && voltageCharacteristic && ledCharacteristic) {
    temperatureCharacteristic->registerForNotify(notifyCallback);
    voltageCharacteristic->registerForNotify(notifyCallback);
    connected = true;
    Serial.println("Characteristics found and notifications registered.");
  } else {
    Serial.println("Failed to find characteristics.");
    pClient->disconnect();
    return false;
  }

  return true;
}

// Class to handle BLE device scanning callbacks
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  // Called for each advertised device found during scanning
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    // Check if the advertised device name matches our target server name
    if (advertisedDevice.getName() == bleServerName) {
      Serial.println("Server found.");
      pServerAddress = new BLEAddress(advertisedDevice.getAddress());  // Store the server address
      advertisedDevice.getScan()->stop();  // Stop scanning once the server is found
      doConnect = true;  // Set flag to connect to this server
    }
  }
};

void setup() {
  Serial.begin(115200);  // Initialize serial communication at 115200 baud rate
  M5.begin();            // Initialize the M5StickC Plus
  M5.Lcd.setRotation(3); // Set screen rotation to landscape
  M5.Lcd.fillScreen(BLACK); // Clear the screen with black color
  M5.Lcd.setCursor(0, 0, 2); // Set cursor position and text size
  M5.Lcd.printf("BLE Client Ready"); // Display initial message

  BLEDevice::init("");   // Initialize the BLE device with empty name (as client)
}

void loop() {
  M5.update();  // Update button states
  
  // If not connected to a server, scan for one
  if (!connected) {
    Serial.println("Scanning for server...");
    M5.Lcd.setCursor(0, 20, 2);
    M5.Lcd.printf("Scanning for server...");
    
    // Set up BLE scanning
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());  // Set callback for scan results
    pBLEScan->setActiveScan(true);  // Active scanning requests more data from devices
    pBLEScan->start(5);  // Scan for 5 seconds
    
    // If a server was found during scanning, try to connect to it
    if (doConnect) {
      if (connectToServer()) {
        Serial.println("Successfully connected.");
        M5.Lcd.setCursor(0, 40, 2);
        M5.Lcd.printf("Connected!");
      } else {
        Serial.println("Connection failed. Retrying...");
        M5.Lcd.setCursor(0, 40, 2);
        M5.Lcd.printf("Connection failed.");
        delay(5000); // Retry delay
      }
      doConnect = false;  // Reset connection flag
    }
  }

  // If connected to a server, handle button interactions
  if (connected) {
    // Button A: Read LED state from the server and display it
    if (M5.BtnA.wasPressed()) {
      Serial.println("Button A pressed: Reading LED state...");
      M5.Lcd.setCursor(0, 80, 2);
      M5.Lcd.fillRect(0, 80, 160, 20, BLACK); // Clear previous text

      if (ledCharacteristic != nullptr) {
        // Read LED state from server
        std::string ledValStd = ledCharacteristic->readValue();
        char ledVal[ledValStd.length() + 1]; // Create buffer for null-terminated string
        strcpy(ledVal, ledValStd.c_str());  // Convert std::string to char array

        // Interpret LED state value and display on screen
        // Note: The LED state is inverted - "1" means OFF, "0" means ON
        if (strcmp(ledVal, "1") == 0) {
          M5.Lcd.printf("LED: OFF");
          Serial.println("LED State: OFF");
        } else if (strcmp(ledVal, "0") == 0) {
          M5.Lcd.printf("LED: ON");
          Serial.println("LED State: ON");
        } else {
          M5.Lcd.printf("LED: Unknown");
          Serial.printf("Unknown LED State: %s\n", ledVal);
        }
      } else {
        M5.Lcd.printf("LED: Error");
        Serial.println("LED characteristic not found.");
      }
    }

    // Button B: Toggle LED on the server and provide immediate feedback
    if (M5.BtnB.wasPressed()) {
      Serial.println("Button B pressed: Toggling LED on server...");
      M5.Lcd.setCursor(0, 100, 2);
      M5.Lcd.fillRect(0, 100, 160, 20, BLACK); // Clear previous text

      if (ledCharacteristic != nullptr) {
        // Read the current state of the LED
        std::string ledValStd = ledCharacteristic->readValue();
        char ledVal[ledValStd.length() + 1];
        strcpy(ledVal, ledValStd.c_str());

        // Toggle LED state - switch between "0" and "1"
        const char* newLedVal = (strcmp(ledVal, "1") == 0) ? "0" : "1";
        ledCharacteristic->writeValue(newLedVal); // Write new state to the Server

        // Display the new LED state on screen
        // Note: The LED state is inverted - "1" means OFF, "0" means ON
        M5.Lcd.printf("Server LED: %s", (strcmp(newLedVal, "1") == 0) ? "OFF" : "ON");
        Serial.printf("Server LED toggled to: %s\n", newLedVal);
      } else {
        M5.Lcd.printf("Error: LED Char");
        Serial.println("LED characteristic not found.");
      }
    }
  }

  delay(1000);  // Wait for 1 second before next loop iteration
}
