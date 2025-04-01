#include <BLEDevice.h>
#include <BLEServer.h>
#include <M5StickCPlus.h>

// Server settings
#define bleServerName "candelario"
#define SERVICE_UUID "021265ed-4111-4da7-823f-c03ac45492f8"

// BLE characteristics UUIDs
#define TEMPERATURE_UUID "6356a54c-0d62-4c4b-9581-6c078120fafc"
#define VOLTAGE_UUID "24a4975c-e835-423b-b987-f6321cdfb663"
#define LED_STATE_UUID "12345678-1234-5678-1234-56789abcdef0"

// Variables
bool ledState = false;  // Current LED state
BLEServer *pServer;
BLEAdvertising *pAdvertising;

// BLE Characteristics
BLECharacteristic temperatureCharacteristic(TEMPERATURE_UUID, BLECharacteristic::PROPERTY_NOTIFY);
BLECharacteristic voltageCharacteristic(VOLTAGE_UUID, BLECharacteristic::PROPERTY_NOTIFY);
BLECharacteristic ledCharacteristic(LED_STATE_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);

// Function to read temperature from M5StickC Plus
float readTemperature() {
  return M5.Axp.GetTempInAXP192(); // Returns temperature in Celsius
}

// Function to read voltage from M5StickC Plus
float readVoltage() {
  return M5.Axp.GetBatVoltage(); // Returns voltage in volts
}

// Callback function for LED characteristic write events (Client writes)
class LEDCharacteristicCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    std::string ledValue = pCharacteristic->getValue();
    if (ledValue == "1") {
      ledState = true;             // LED is ON
      digitalWrite(10, HIGH);      // Turn LED ON
      Serial.println("LED turned ON by client");
    } else if (ledValue == "0") {
      ledState = false;            // LED is OFF
      digitalWrite(10, LOW);       // Turn LED OFF
      Serial.println("LED turned OFF by client");
    }

    // Update display after a Client write (invert text)
    M5.Lcd.setCursor(0, 20, 2);
    M5.Lcd.fillRect(0, 20, 160, 20, BLACK);
    M5.Lcd.printf("LED: %s", ledState ? "OFF" : "ON");

    // Notify temperature and voltage values for consistency
    float temp = readTemperature();
    float voltage = readVoltage();
    char tempStr[6], voltageStr[6];
    sprintf(tempStr, "%.2f", temp);
    sprintf(voltageStr, "%.2f", voltage);

    temperatureCharacteristic.setValue(tempStr);
    voltageCharacteristic.setValue(voltageStr);

    temperatureCharacteristic.notify();
    voltageCharacteristic.notify();

    Serial.printf("LED: %s | Temp: %s C | Volt: %s V\n",
                  ledState ? "OFF" : "ON", tempStr, voltageStr);
  }
};

void setupBLE() {
  BLEDevice::init(bleServerName);
  pServer = BLEDevice::createServer();

  BLEService *bleService = pServer->createService(SERVICE_UUID);

  // Add temperature characteristic
  bleService->addCharacteristic(&temperatureCharacteristic);
  temperatureCharacteristic.setValue("25.0");

  // Add voltage characteristic
  bleService->addCharacteristic(&voltageCharacteristic);
  voltageCharacteristic.setValue("5.0");

  // Add LED state characteristic with write capability
  bleService->addCharacteristic(&ledCharacteristic);
  ledCharacteristic.setValue("0");  // Default LED state off
  ledCharacteristic.setCallbacks(new LEDCharacteristicCallbacks());

  bleService->start();

  // Set up BLE advertising
  pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->start();

  Serial.println("BLE server ready, waiting for clients...");
}

void setup() {
  Serial.begin(115200);
  M5.begin();
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.printf("BLE Server Ready");

  pinMode(10, OUTPUT);  // Initialize LED GPIO
  digitalWrite(10, LOW);  // Ensure LED starts off

  setupBLE();
}

void loop() {
  M5.update(); // Update button states

  // Button A: Local toggle on Server
  if (M5.BtnA.wasPressed()) {
    // Toggle LED state
    ledState = !ledState;
    digitalWrite(10, ledState ? HIGH : LOW);

    // Update LED characteristic and notify
    ledCharacteristic.setValue(ledState ? "1" : "0");
    ledCharacteristic.notify();

    // Read temperature and voltage
    float temp = readTemperature();
    float voltage = readVoltage();

    // Update and notify temperature and voltage
    char tempStr[6], voltageStr[6];
    sprintf(tempStr, "%.2f", temp);
    sprintf(voltageStr, "%.2f", voltage);

    temperatureCharacteristic.setValue(tempStr);
    voltageCharacteristic.setValue(voltageStr);

    temperatureCharacteristic.notify();
    voltageCharacteristic.notify();

    // Update display (invert text relative to ledState)
    M5.Lcd.setCursor(0, 20, 2);
    M5.Lcd.fillRect(0, 20, 160, 20, BLACK);
    M5.Lcd.printf("LED: %s", ledState ? "OFF" : "ON");

    M5.Lcd.setCursor(0, 40, 2);
    M5.Lcd.fillRect(0, 40, 160, 20, BLACK);
    M5.Lcd.printf("Temp: %s C", tempStr);

    M5.Lcd.setCursor(0, 60, 2);
    M5.Lcd.fillRect(0, 60, 160, 20, BLACK);
    M5.Lcd.printf("Volt: %s V", voltageStr);

    Serial.printf("LED: %s | Temp: %s C | Volt: %s V\n",
                  ledState ? "OFF" : "ON", tempStr, voltageStr);
  }
}
