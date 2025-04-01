// m5stick A
#include <WiFi.h>
#include <PubSubClient.h>
#include <M5StickCPlus.h>

// WiFi credentials input your own lol
const char* ssid = "chris";
const char* password = "";

// MQTT Broker
const char* mqtt_server   = "test.mosquitto.org";
const char* topic_publish = "m5stickAcontrol";
const char* topic_subscribe = "m5stickBcontrol";

// MQTT Client Setup
WiFiClient espClient;
PubSubClient client(espClient);

// LED and Button
bool ledState = false;

// Callback function for incoming MQTT messages
void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  if (message == "on") {
    // Turn LED ON
    digitalWrite(10, LOW);
    ledState = true;
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Lcd.println("LED ON");
  } else if (message == "off") {
    // Turn LED OFF
    digitalWrite(10, HIGH);
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
    // Use a unique client ID for this M5Stick A
    if (client.connect("M5StickA_1")) {
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
  Serial.begin(115200);

  // Configure built-in LED
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH); // Start with LED off

  // Initialize display
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.setCursor(10, 30);
  M5.Lcd.println("MQTT LAB");

  // Connect to Wi-Fi
  connectWiFi();

  // Configure MQTT server and callback
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  // Reconnect to MQTT if needed
  if (!client.connected()) {
    connectMQTT();
  }
  client.loop();

  // M5Stick event updates
  M5.update();

  // Check if Button A was pressed
  if (M5.BtnA.wasPressed()) {
    ledState = !ledState;
    String message = ledState ? "on" : "off";
    client.publish(topic_publish, message.c_str());
    Serial.print("Sent: ");
    Serial.println(message);

    // Small debounce
    delay(300);
  }
}
