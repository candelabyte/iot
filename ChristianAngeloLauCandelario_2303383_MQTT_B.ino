// m5stick B
#include <WiFi.h>
#include <PubSubClient.h>
#include <M5StickCPlus.h>

// WiFi credentials input your own lol
const char* ssid = "chris";
const char* password = "";

// MQTT Broker
const char* mqtt_server = "test.mosquitto.org";

// Topics (Adjust if needed)
const char* topic_publish   = "m5stickBcontrol";
const char* topic_subscribe = "m5stickAcontrol";

// MQTT Client Setup
WiFiClient espClient;
PubSubClient client(espClient);

// LED and Button
bool ledState = false;

// Callback function to handle incoming messages
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (message == "on") {
    digitalWrite(10, LOW); // Turn LED ON
    ledState = true;
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);        
    M5.Lcd.println("LED ON");
  } else if (message == "off") {
    digitalWrite(10, HIGH); // Turn LED OFF
    ledState = false;
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);        
    M5.Lcd.println("LED OFF");
  }
}

// Connect to Wi-Fi
void connectWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

// Connect to MQTT broker
void connectMQTT() {
  while (!client.connected()) {
    Serial.println("Connecting to MQTT...");
    // Give this device a UNIQUE ID
    if (client.connect("M5StickB_1")) {
      Serial.println("Connected to MQTT broker");
      client.subscribe(topic_subscribe);
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" trying again in 5 seconds...");
      delay(5000);
    }
  }
}

void setup() {
  M5.begin();
  pinMode(10, OUTPUT); // Set GPIO10 as an output (built-in LED)

  Serial.begin(115200);

  // Connect to Wi-Fi
  connectWiFi();

  // Setup display
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.setCursor(10, 30);
  M5.Lcd.println("MQTT LAB");

  // Set MQTT server and callback
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  // Reconnect to MQTT if disconnected
  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();

  // Update M5 events
  M5.update();

  // If button A is pressed, toggle LED state and publish
  if (M5.BtnA.wasPressed()) {
    ledState = !ledState;
    String message = ledState ? "on" : "off";
    client.publish(topic_publish, message.c_str());
    Serial.print("Sent: ");
    Serial.println(message);

    // Short debounce
    delay(300);
  }
}
