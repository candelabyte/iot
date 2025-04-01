#include <WiFi.h>         // Include WiFi library for network connectivity
#include <WebServer.h>     // Include WebServer library for handling HTTP requests
#include <M5StickCPlus.h>  // Include M5StickCPlus library for the M5StickC Plus device

/* WiFi credentials - intentionally left blank for security */
const char* ssid = "";      // WiFi network name
const char* password = "";  // WiFi password

// Variables for storing sensor data from the IMU (Inertial Measurement Unit)
float pitch, roll, yaw;     // Gyroscope data (orientation angles)
float temperature;          // Temperature sensor data
float accX, accY, accZ;     // Accelerometer data (acceleration in 3 axes)

// Create a web server instance on port 80 (standard HTTP port)
WebServer server(80);

void setup() {
  Serial.begin(115200);  // Initialize serial communication at 115200 baud rate

  // Initialize the M5StickC Plus and its components
  M5.begin();
  int imuStatus = M5.IMU.Init(); // Initialize the Inertial Measurement Unit (IMU)
  if (imuStatus != 0) {
    Serial.println("IMU initialization failed!");
  }

  // Configure the display
  M5.Lcd.setRotation(3);         // Set screen rotation to landscape mode
  M5.Lcd.fillScreen(BLACK);      // Clear the screen with black color
  M5.Lcd.setCursor(0, 0, 2);     // Set cursor position and text size
  M5.Lcd.printf("RESTful API");  // Display title

  // Connect to WiFi network
  WiFi.begin(ssid, password);           // Start WiFi connection with provided credentials
  WiFi.setHostname("group01-stick");    // Set device hostname on the network
  M5.Lcd.setCursor(0, 20, 2);
  M5.Lcd.printf("Connecting to Wi-Fi...");
  
  // Wait until connected to WiFi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");  // Print dots while connecting
  }
  
  // Display connection success and IP address
  Serial.println("\nWi-Fi Connected!");
  M5.Lcd.setCursor(0, 40, 2);
  M5.Lcd.printf("IP: %s", WiFi.localIP().toString().c_str());

  // Initialize the built-in LED on GPIO pin 10
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH); // Set initial state to OFF (inverted logic: HIGH = OFF, LOW = ON)

  // Define REST API endpoints and their handler functions
  server.on("/", handleRoot);                          // Root endpoint - shows available endpoints
  server.on("/gyro", handleGyro);                      // Endpoint for gyroscope data
  server.on("/accel", handleAccel);                    // Endpoint for accelerometer data
  server.on("/temp", handleTemp);                      // Endpoint for temperature data
  server.on("/led/on", []() { handleLED(true); });     // Endpoint to turn LED on
  server.on("/led/off", []() { handleLED(false); });   // Endpoint to turn LED off
  server.on("/buzzer/on", []() { handleBuzzer(true); });   // Endpoint to turn buzzer on
  server.on("/buzzer/off", []() { handleBuzzer(false); }); // Endpoint to turn buzzer off
  server.onNotFound(handleNotFound);                   // Handler for undefined endpoints

  // Start the HTTP server
  server.begin();
  Serial.println("HTTP server started!");
}

// Function to read all sensor data and update the display
void refreshSensorData() {
  // Read data from all sensors
  M5.IMU.getAhrsData(&pitch, &roll, &yaw);       // Get orientation data (pitch, roll, yaw angles)
  M5.IMU.getAccelData(&accX, &accY, &accZ);      // Get acceleration data (X, Y, Z axes)
  M5.IMU.getTempData(&temperature);              // Get temperature data
  temperature = (temperature - 32.0) * 5.0 / 9.0; // Convert from Fahrenheit to Celsius

  // Update the display with the new sensor data
  M5.Lcd.fillScreen(BLACK);                      // Clear the screen
  M5.Lcd.setCursor(0, 0, 2);
  M5.Lcd.printf("Refreshed Data:");
  
  // Display gyroscope data (orientation)
  M5.Lcd.setCursor(0, 20, 2);
  M5.Lcd.printf("Gyro - P:%.2f R:%.2f Y:%.2f", pitch, roll, yaw);
  
  // Display accelerometer data
  M5.Lcd.setCursor(0, 40, 2);
  M5.Lcd.printf("Accel - X:%.2f Y:%.2f Z:%.2f", accX, accY, accZ);
  
  // Display temperature data
  M5.Lcd.setCursor(0, 60, 2);
  M5.Lcd.printf("Temp: %.2f C", temperature);
}

// Handler function for the root endpoint "/"
// Lists all available API endpoints
void handleRoot() {
  // Display available endpoints on the M5StickC Plus screen
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

  // Create and send the HTTP response with the list of endpoints
  String response = "Available Endpoints:\n";
  response += "/gyro - Get gyroscope data\n";
  response += "/accel - Get accelerometer data\n";
  response += "/temp - Get temperature data\n";
  response += "/led/on - Turn LED ON\n";
  response += "/led/off - Turn LED OFF\n";
  response += "/buzzer/on - Turn Buzzer ON\n";
  response += "/buzzer/off - Turn Buzzer OFF\n";
  
  // Send HTTP 200 OK response with the endpoint list
  server.send(200, "text/plain", response);
}

// Handler function for the "/gyro" endpoint
// Returns gyroscope (orientation) data in JSON format
void handleGyro() {
  // Read current gyroscope data
  M5.IMU.getAhrsData(&pitch, &roll, &yaw);
  
  // Format the data as a JSON string
  String response = "{ \"pitch\": " + String(pitch, 2) + 
                    ", \"roll\": " + String(roll, 2) + 
                    ", \"yaw\": " + String(yaw, 2) + " }";
  
  // Send HTTP 200 OK response with the JSON data
  server.send(200, "application/json", response);
}

// Handler function for the "/accel" endpoint
// Returns accelerometer data in JSON format
void handleAccel() {
  // Read current accelerometer data
  M5.IMU.getAccelData(&accX, &accY, &accZ);
  
  // Format the data as a JSON string
  String response = "{ \"accelX\": " + String(accX, 2) +
                    ", \"accelY\": " + String(accY, 2) +
                    ", \"accelZ\": " + String(accZ, 2) + " }";
  
  // Send HTTP 200 OK response with the JSON data
  server.send(200, "application/json", response);
}

// Handler function for the "/temp" endpoint
// Returns temperature data in JSON format
void handleTemp() {
  // Read current temperature data
  M5.IMU.getTempData(&temperature);
  temperature = (temperature - 32.0) * 5.0 / 9.0; // Convert from Fahrenheit to Celsius
  
  // Format the data as a JSON string
  String response = "{ \"temperature\": " + String(temperature, 2) + " }";
  
  // Send HTTP 200 OK response with the JSON data
  server.send(200, "application/json", response);
}

// Handler function for the LED control endpoints
// Controls the built-in LED on the M5StickC Plus
void handleLED(bool state) {
  // Set LED state (note: inverted logic - LOW turns ON, HIGH turns OFF)
  digitalWrite(10, state ? LOW : HIGH);
  
  // Send HTTP 200 OK response with confirmation message
  server.send(200, "text/plain", state ? "LED ON" : "LED OFF");
}

// Handler function for the buzzer control endpoints
// Controls a buzzer connected to GPIO2
void handleBuzzer(bool state) {
  if (state) {
    tone(2, 2000); // Generate a 2000Hz tone on GPIO2
  } else {
    noTone(2);     // Stop the tone on GPIO2
  }
  
  // Send HTTP 200 OK response with confirmation message
  server.send(200, "text/plain", state ? "Buzzer ON" : "Buzzer OFF");
}

// Handler function for undefined endpoints
// Returns a 404 Not Found error
void handleNotFound() {
  // Send HTTP 404 Not Found response
  server.send(404, "text/plain", "404: Not Found");
}

void loop() {
  server.handleClient();  // Process incoming HTTP requests
  
  // Check if the HOME button is pressed to refresh sensor data
  if (digitalRead(M5_BUTTON_HOME) == LOW) {
    refreshSensorData();  // Read and display all sensor data
    delay(300);           // Debounce delay to prevent multiple triggers
  }
}
