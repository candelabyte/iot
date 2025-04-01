#include <BLEDevice.h>      // Include the BLE library for Bluetooth Low Energy functionality
#include <BLEServer.h>      // Include the BLE Server library for creating a BLE server
#include <M5StickCPlus.h>   // Include the M5StickCPlus library for the M5StickC Plus device

// Server settings
#define bleServerName "candelario"  // Name of the BLE server that clients will see
#define SERVICE_UUID "021265ed-4111-4da7-823f-c03ac45492f8"  // UUID for the main service

// BLE characteristics UUIDs - Unique identifiers for each characteristic
#define TEMPERATURE_UUID "6356a54c-0d62-4c4b-9581-6c078120fafc"  // UUID for temperature characteristic
#define VOLTAGE_UUID "24a4975c-e835-423b-b987-f6321cdfb663"      // UUID for voltage characteristic
#define LED_STATE_UUID "12345678-1234-5678-1234-56789abcdef0"    // UUID for LED state characteristic

// Variables
bool ledState = false;  // Current LED state (false = OFF, true = ON)
BLEServer *pServer;     // Pointer to the BLE server instance
BLEAdvertising *pAdvertising;  // Pointer to the BLE advertising instance

// BLE Characteristics - Define the data that can be shared with clients
BLECharacteristic temperatureCharacteristic(TEMPERATURE_UUID, BLECharacteristic::PROPERTY_NOTIFY);  // Temperature data (notify only)
BLECharacteristic voltageCharacteristic(VOLTAGE_UUID, BLECharacteristic::PROPERTY_NOTIFY);          // Voltage data (notify only)
BLECharacteristic ledCharacteristic(LED_STATE_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);  // LED state (read, write, notify)

// Function to read temperature from M5StickC Plus internal sensor
float readTemperature() {
  return M5.Axp.GetTempInAXP192(); // Returns temperature in Celsius from the AXP192 power management chip
}

// Function to read battery voltage from M5StickC Plus
float readVoltage() {
  return M5.Axp.GetBatVoltage(); // Returns battery voltage in volts
}

// Callback class for handling LED characteristic write events (when a client writes to the LED characteristic)
class LEDCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    // Get the value written by the client
    std::string ledValue = pCharacteristic->getValue();
    
    // Process the received value
    if (ledValue == "1") {
      ledState = true;             // Set LED state to ON
      digitalWrite(10, HIGH);      // Turn physical LED ON
      Serial.println("LED turned ON by client");
    } else if (ledValue == "0") {
      ledState = false;            // Set LED state to OFF
      digitalWrite(10, LOW);       // Turn physical LED OFF
      Serial.println("LED turned OFF by client");
    }

    // Update display after a Client write (note: display text is inverted relative to actual state)
    M5.Lcd.setCursor(0, 20, 2);
    M5.Lcd.fillRect(0, 20, 160, 20, BLACK);  // Clear previous text
    M5.Lcd.printf("LED: %s", ledState ? "OFF" : "ON");  // Display inverted state

    // Read current sensor values
    float temp = readTemperature();
    float voltage = readVoltage();
    
    // Format values as strings with 2 decimal places
    char tempStr[6], voltageStr[6];
    sprintf(tempStr, "%.2f", temp);
    sprintf(voltageStr, "%.2f", voltage);

    // Update characteristic values
    temperatureCharacteristic.setValue(tempStr);
    voltageCharacteristic.setValue(voltageStr);

    // Send notifications to connected clients
    temperatureCharacteristic.notify();
    voltageCharacteristic.notify();

    // Log all values to serial monitor
    Serial.printf("LED: %s | Temp: %s C | Volt: %s V\n",
                  ledState ? "OFF" : "ON", tempStr, voltageStr);
  }
};

// Function to set up the BLE server and its characteristics
void setupBLE() {
  // Initialize the BLE device with the server name
  BLEDevice::init(bleServerName);
  pServer = BLEDevice::createServer();

  // Create a BLE service with the specified UUID
  BLEService *bleService = pServer->createService(SERVICE_UUID);

  // Add temperature characteristic to the service
  bleService->addCharacteristic(&temperatureCharacteristic);
  temperatureCharacteristic.setValue("25.0");  // Set initial value

  // Add voltage characteristic to the service
  bleService->addCharacteristic(&voltageCharacteristic);
  voltageCharacteristic.setValue("5.0");  // Set initial value

  // Add LED state characteristic to the service
  bleService->addCharacteristic(&ledCharacteristic);
  ledCharacteristic.setValue("0");  // Default LED state off
  ledCharacteristic.setCallbacks(new LEDCharacteristicCallbacks());  // Set callback for client writes

  // Start the service
  bleService->start();

  // Set up BLE advertising so clients can find this server
  pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);  // Advertise our service UUID
  pAdvertising->setScanResponse(true);  // Respond to scan requests
  pAdvertising->start();  // Start advertising

  Serial.println("BLE server ready, waiting for clients...");
}

void setup() {
  Serial.begin(115200);  // Initialize serial communication at 115200 baud rate
  M5.begin();            // Initialize the M5StickC Plus
  M5.Lcd.setRotation(3); // Set screen rotation to landscape
  M5.Lcd.fillScreen(BLACK); // Clear the screen with black color
  M5.Lcd.setCursor(0, 0, 2); // Set cursor position and text size
  M5.Lcd.printf("BLE Server Ready"); // Display initial message

  pinMode(10, OUTPUT);     // Initialize GPIO pin 10 as output for the LED
  digitalWrite(10, LOW);   // Ensure LED starts in OFF state

  setupBLE();  // Initialize and start the BLE server
}

void loop() {
  M5.update(); // Update button states for the M5StickC Plus

  // Button A: Local toggle of LED on Server
  if (M5.BtnA.wasPressed()) {
    // Toggle LED state (ON to OFF or OFF to ON)
    ledState = !ledState;
    digitalWrite(10, ledState ? HIGH : LOW);  // Set the physical LED based on state

    // Update LED characteristic value and notify connected clients
    ledCharacteristic.setValue(ledState ? "1" : "0");  // "1" for ON, "0" for OFF
    ledCharacteristic.notify();  // Send notification to connected clients

    // Read current sensor values
    float temp = readTemperature();
    float voltage = readVoltage();

    // Format values as strings with 2 decimal places
    char tempStr[6], voltageStr[6];
    sprintf(tempStr, "%.2f", temp);
    sprintf(voltageStr, "%.2f", voltage);

    // Update characteristic values
    temperatureCharacteristic.setValue(tempStr);
    voltageCharacteristic.setValue(voltageStr);

    // Send notifications to connected clients
    temperatureCharacteristic.notify();
    voltageCharacteristic.notify();

    // Update display with all values
    // Note: LED display text is inverted relative to actual state
    M5.Lcd.setCursor(0, 20, 2);
    M5.Lcd.fillRect(0, 20, 160, 20, BLACK);  // Clear previous text
    M5.Lcd.printf("LED: %s", ledState ? "OFF" : "ON");  // Display inverted state

    M5.Lcd.setCursor(0, 40, 2);
    M5.Lcd.fillRect(0, 40, 160, 20, BLACK);  // Clear previous text
    M5.Lcd.printf("Temp: %s C", tempStr);    // Display temperature

    M5.Lcd.setCursor(0, 60, 2);
    M5.Lcd.fillRect(0, 60, 160, 20, BLACK);  // Clear previous text
    M5.Lcd.printf("Volt: %s V", voltageStr); // Display voltage

    // Log all values to serial monitor
    Serial.printf("LED: %s | Temp: %s C | Volt: %s V\n",
                  ledState ? "OFF" : "ON", tempStr, voltageStr);
  }
}
