#include "BLEDevice.h"
#include <M5StickCPlus.h>

// BLE UUIDs
#define bleServerName "candelario"
#define SERVICE_UUID "021265ed-4111-4da7-823f-c03ac45492f8"
#define TEMPERATURE_UUID "6356a54c-0d62-4c4b-9581-6c078120fafc"
#define VOLTAGE_UUID "24a4975c-e835-423b-b987-f6321cdfb663"
#define LED_STATE_UUID "12345678-1234-5678-1234-56789abcdef0"

BLEClient* pClient;
BLERemoteCharacteristic* temperatureCharacteristic;
BLERemoteCharacteristic* voltageCharacteristic;
BLERemoteCharacteristic* ledCharacteristic;
BLEAddress *pServerAddress;

bool doConnect = false;
bool connected = false;

// Function to handle notification callbacks
void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  char value[length + 1]; // Create a buffer with an extra byte for null-termination
  memcpy(value, pData, length); // Copy received data into the buffer
  value[length] = '\0'; // Null-terminate the string

  if (pBLERemoteCharacteristic->getUUID().toString() == TEMPERATURE_UUID) {
    M5.Lcd.setCursor(0, 40, 2);
    M5.Lcd.fillRect(0, 40, 160, 20, BLACK); // Clear previous text
    M5.Lcd.printf("Temp: %s C", value);
    Serial.printf("Received Temperature: %s C\n", value);
  } else if (pBLERemoteCharacteristic->getUUID().toString() == VOLTAGE_UUID) {
    M5.Lcd.setCursor(0, 60, 2);
    M5.Lcd.fillRect(0, 60, 160, 20, BLACK); // Clear previous text
    M5.Lcd.printf("Volt: %s V", value);
    Serial.printf("Received Voltage: %s V\n", value);
  }
}

// Connect to server
bool connectToServer() {
  Serial.println("Attempting to connect to the server...");
  M5.Lcd.setCursor(0, 20, 2);
  M5.Lcd.printf("Connecting to server...");

  pClient = BLEDevice::createClient();
  if (!pClient->connect(*pServerAddress)) {
    Serial.println("Failed to connect to server.");
    return false;
  }

  Serial.println("Connected to server.");
  M5.Lcd.setCursor(0, 40, 2);
  M5.Lcd.printf("Connected to server");

  BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
  if (!pRemoteService) {
    Serial.println("Service not found.");
    pClient->disconnect();
    return false;
  }

  // Retrieve characteristics
  temperatureCharacteristic = pRemoteService->getCharacteristic(TEMPERATURE_UUID);
  voltageCharacteristic = pRemoteService->getCharacteristic(VOLTAGE_UUID);
  ledCharacteristic = pRemoteService->getCharacteristic(LED_STATE_UUID);

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

// Scan for server
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.getName() == bleServerName) {
      Serial.println("Server found.");
      pServerAddress = new BLEAddress(advertisedDevice.getAddress());
      advertisedDevice.getScan()->stop();
      doConnect = true;
    }
  }
};

void setup() {
  Serial.begin(115200);
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.printf("BLE Client Ready");

  BLEDevice::init("");
}

void loop() {
  M5.update();
  
  if (!connected) {
    Serial.println("Scanning for server...");
    M5.Lcd.setCursor(0, 20, 2);
    M5.Lcd.printf("Scanning for server...");
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true);
    pBLEScan->start(5);
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
      doConnect = false;
    }
  }

  if (connected) {
    // Button A: Read LED state from the server and display it.
    if (M5.BtnA.wasPressed()) {
      Serial.println("Button A pressed: Reading LED state...");
      M5.Lcd.setCursor(0, 80, 2);
      M5.Lcd.fillRect(0, 80, 160, 20, BLACK); // Clear previous text

      if (ledCharacteristic != nullptr) {
        // Read LED state from server
        std::string ledValStd = ledCharacteristic->readValue();
        char ledVal[ledValStd.length() + 1]; // Create buffer for null-terminated string
        strcpy(ledVal, ledValStd.c_str());  // Convert std::string to char array

        // Correct interpretation of LED state for Button A (assume this one is correct)
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

    // Button B: Toggle LED on the server and provide immediate feedback.
    if (M5.BtnB.wasPressed()) {
      Serial.println("Button B pressed: Toggling LED on server...");
      M5.Lcd.setCursor(0, 100, 2);
      M5.Lcd.fillRect(0, 100, 160, 20, BLACK); // Clear previous text

      if (ledCharacteristic != nullptr) {
        // Read the current state of the LED
        std::string ledValStd = ledCharacteristic->readValue();
        char ledVal[ledValStd.length() + 1];
        strcpy(ledVal, ledValStd.c_str());

        // Toggle LED state
        const char* newLedVal = (strcmp(ledVal, "1") == 0) ? "0" : "1";
        ledCharacteristic->writeValue(newLedVal); // Write new state to the Server

        // Provide immediate feedback with corrected (inverted) text.
        // Since the hardware is inverted relative to the value, display "OFF" when newLedVal is "1"
        // and "ON" when newLedVal is "0".
        M5.Lcd.printf("Server LED: %s", (strcmp(newLedVal, "1") == 0) ? "OFF" : "ON");
        Serial.printf("Server LED toggled to: %s\n", newLedVal);
      } else {
        M5.Lcd.printf("Error: LED Char");
        Serial.println("LED characteristic not found.");
      }
    }
  }

  delay(1000);
}
