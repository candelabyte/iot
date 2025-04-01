#include <WiFi.h>
#include <WebServer.h>
#include <M5StickCPlus.h>

/* Put your SSID & Password - removed*/
const char* ssid = "";
const char* password = "";

// Variables for sensor data
float pitch, roll, yaw, temperature, accX, accY, accZ;

// REST server on port 80
WebServer server(80);

void setup() {
  Serial.begin(115200);

  // Initialize stick
  M5.begin();
  int imuStatus = M5.IMU.Init(); // Initialize IMU
  if (imuStatus != 0) {
    Serial.println("IMU initialization failed!");
  }

  // Display configuration
  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.printf("RESTful API");

  // Wifi connection
  WiFi.begin(ssid, password);
  WiFi.setHostname("group01-stick");
  M5.Lcd.setCursor(0, 20, 2);
  M5.Lcd.printf("Connecting to Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi Connected!");
  M5.Lcd.setCursor(0, 40, 2);
  M5.Lcd.printf("IP: %s", WiFi.localIP().toString().c_str());

  // LED Initialization
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH); // inverted logic

  // REST API Endpoints
  server.on("/", handleRoot);
  server.on("/gyro", handleGyro);
  server.on("/accel", handleAccel);
  server.on("/temp", handleTemp);
  server.on("/led/on", []() { handleLED(true); });
  server.on("/led/off", []() { handleLED(false); });
  server.on("/buzzer/on", []() { handleBuzzer(true); });
  server.on("/buzzer/off", []() { handleBuzzer(false); });
  server.onNotFound(handleNotFound);

  // Start the server
  server.begin();
  Serial.println("HTTP server started!");
}

void refreshSensorData() {
  // Refresh gyroscope, accelerometer, and temperature data
  M5.IMU.getAhrsData(&pitch, &roll, &yaw);
  M5.IMU.getAccelData(&accX, &accY, &accZ);
  M5.IMU.getTempData(&temperature);
  temperature = (temperature - 32.0) * 5.0 / 9.0; // Convert to Celsius

  // Update display
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.printf("Refreshed Data:");
  M5.Lcd.setCursor(0, 20, 2);
  M5.Lcd.printf("Gyro - P:%.2f R:%.2f Y:%.2f", pitch, roll, yaw);
  M5.Lcd.setCursor(0, 40, 2);
  M5.Lcd.printf("Accel - X:%.2f Y:%.2f Z:%.2f", accX, accY, accZ);
  M5.Lcd.setCursor(0, 60, 2);
  M5.Lcd.printf("Temp: %.2f C", temperature);
}

void handleRoot() {
  // Display root information
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.printf("Endpoints:");
  M5.Lcd.setCursor(0, 20, 2);
  M5.Lcd.printf("/gyro - Gyroscope");
  M5.Lcd.setCursor(0, 40, 2);
  M5.Lcd.printf("/accel - Accelerometer");
  M5.Lcd.setCursor(0, 60, 2);
  M5.Lcd.printf("/temp - Temperature");
  M5.Lcd.setCursor(0, 80, 2);
  M5.Lcd.printf("/led/on - LED ON");
  M5.Lcd.setCursor(0, 100, 2);
  M5.Lcd.printf("/led/off - LED OFF");
  M5.Lcd.setCursor(0, 120, 2);
  M5.Lcd.printf("/buzzer/on - Buzzer ON");
  M5.Lcd.setCursor(0, 140, 2);
  M5.Lcd.printf("/buzzer/off - Buzzer OFF");

  String response = "Available Endpoints:\n";
  response += "/gyro - Get gyroscope data\n";
  response += "/accel - Get accelerometer data\n";
  response += "/temp - Get temperature data\n";
  response += "/led/on - Turn LED ON\n";
  response += "/led/off - Turn LED OFF\n";
  response += "/buzzer/on - Turn Buzzer ON\n";
  response += "/buzzer/off - Turn Buzzer OFF\n";
  server.send(200, "text/plain", response);
}

void handleGyro() {
  // Read gyroscope data
  M5.IMU.getAhrsData(&pitch, &roll, &yaw);
  String response = "{ \"pitch\": " + String(pitch, 2) + 
                    ", \"roll\": " + String(roll, 2) + 
                    ", \"yaw\": " + String(yaw, 2) + " }";
  server.send(200, "application/json", response);
}

void handleAccel() {
  // Read accelerometer data
  M5.IMU.getAccelData(&accX, &accY, &accZ);
  String response = "{ \"accelX\": " + String(accX, 2) +
                    ", \"accelY\": " + String(accY, 2) +
                    ", \"accelZ\": " + String(accZ, 2) + " }";
  server.send(200, "application/json", response);
}

void handleTemp() {
  // Read temperature
  M5.IMU.getTempData(&temperature);
  temperature = (temperature - 32.0) * 5.0 / 9.0; // Convert to Celsius
  String response = "{ \"temperature\": " + String(temperature, 2) + " }";
  server.send(200, "application/json", response);
}

void handleLED(bool state) {
  digitalWrite(10, state ? LOW : HIGH); // Inverted logic for LED control
  server.send(200, "text/plain", state ? "LED ON" : "LED OFF");
}

void handleBuzzer(bool state) {
  if (state) {
    tone(2, 2000); // Generate 2000Hz tone on GPIO2
  } else {
    noTone(2); // Stop the tone on GPIO2
  }
  server.send(200, "text/plain", state ? "Buzzer ON" : "Buzzer OFF");
}

void handleNotFound() {
  server.send(404, "text/plain", "404: Not Found");
}

void loop() {
  server.handleClient();

  // Refresh sensor data when HOME button is pressed
  if (digitalRead(M5_BUTTON_HOME) == LOW) {
    refreshSensorData();
    delay(300); // Debounce delay
  }
}
