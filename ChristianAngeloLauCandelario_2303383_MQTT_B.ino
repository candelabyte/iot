// M5StickC Plus B - MQTT Client Implementation
#include <WiFi.h>           // Include WiFi library for network connectivity
#include <PubSubClient.h>   // Include PubSubClient library for MQTT communication
#include <M5StickCPlus.h>   // Include M5StickCPlus library for the M5StickC Plus device

// WiFi credentials - need to be configured for your network
const char* ssid = "chris";      // WiFi network name
const char* password = "";       // WiFi password (empty for open networks)

// MQTT Broker configuration
const char* mqtt_server = "test.mosquitto.org";  // Public MQTT broker address

// MQTT Topics - Note these are reversed compared to device A
const char* topic_publish   = "m5stickBcontrol";  // Topic to publish messages to (for controlling device A)
const char* topic_subscribe = "m5stickAcontrol";  // Topic to subscribe to (for receiving commands from device A)

// MQTT Client Setup
WiFiClient espClient;           // Create a WiFi client
PubSubClient client(espClient); // Create MQTT client using the WiFi client

// LED state tracking
bool ledState = false;          // Track the current state of the LED (false = OFF, true = ON)

// Callback function for incoming MQTT messages - executed when a message is received on the subscribed topic
void callback(char* topic, byte* payload, unsigned int length) {
  // Convert the received payload bytes to a String
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  // Process the message based on its content
  if (message == "on") {
    digitalWrite(10, LOW); // Turn LED ON (Note: LOW turns the LED ON due to how it's wired on the M5StickC Plus)
    ledState = true;
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);        
    M5.Lcd.println("LED ON");
  } else if (message == "off") {
    digitalWrite(10, HIGH); // Turn LED OFF (Note: HIGH turns the LED OFF due to how it's wired on the M5StickC Plus)
    ledState = false;
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);        
    M5.Lcd.println("LED OFF");
  }
}

// Function to connect to Wi-Fi network
void connectWiFi() {
  WiFi.begin(ssid, password);  // Initiate connection to WiFi with provided credentials
  
  // Wait until connected, showing status in serial monitor
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  
  Serial.println("Connected to WiFi");  // Connection successful
}

// Function to connect to the MQTT broker
void connectMQTT() {
  // Try to connect until successful
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    
    // Attempt to connect with a unique client ID for device B
    if (client.connect("M5StickB_1")) {
      Serial.println("Connected to MQTT broker");
      client.subscribe(topic_subscribe);  // Subscribe to the topic to receive messages
    } else {
      // Connection failed, print error code and retry after delay
      Serial.print("Failed, rc=");
      Serial.print(client.state());  // Print the error code
      Serial.println(" trying again in 5 seconds...");
      delay(5000);  // Wait 5 seconds before retrying
    }
  }
}

void setup() {
  M5.begin();              // Initialize M5StickC Plus
  pinMode(10, OUTPUT);     // Set GPIO10 as an output for the built-in LED

  Serial.begin(115200);    // Initialize serial communication at 115200 baud rate

  // Connect to Wi-Fi network
  connectWiFi();

  // Initialize display with title
  M5.Lcd.fillScreen(BLACK);    // Clear screen with black
  M5.Lcd.setTextSize(2);       // Set text size
  M5.Lcd.setTextColor(GREEN);  // Set text color to green
  M5.Lcd.setCursor(10, 30);    // Set cursor position
  M5.Lcd.println("MQTT LAB");  // Display title

  // Configure MQTT server and callback function
  client.setServer(mqtt_server, 1883);  // Set MQTT broker address and port (1883 is the default MQTT port)
  client.setCallback(callback);         // Set the function to be called when a message is received
}

void loop() {
  // Ensure MQTT connection is maintained
  if (!client.connected()) {
    connectMQTT();  // Reconnect if connection is lost
  }
  client.loop();    // Process incoming messages and maintain the MQTT connection

  // Update button states
  M5.update();      // Required to detect button presses on M5StickC Plus

  // Check if Button A was pressed to toggle LED state
  if (M5.BtnA.wasPressed()) {
    ledState = !ledState;                          // Toggle LED state
    String message = ledState ? "on" : "off";      // Create message based on new state
    client.publish(topic_publish, message.c_str()); // Publish message to MQTT topic
    
    // Log the sent message to serial monitor
    Serial.print("Sent: ");
    Serial.println(message);

    // Small delay to prevent button bounce
    delay(300);
  }
}
