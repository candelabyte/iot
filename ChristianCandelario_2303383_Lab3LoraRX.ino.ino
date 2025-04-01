// LoRa Receiver (RX) Node Implementation

#include <SPI.h>              // Include SPI library for communication with the LoRa module
#include <RH_RF95.h>          // Include RadioHead library for LoRa radio
#include <Wire.h>             // Include Wire library for I2C communication with the OLED display
#include <Adafruit_GFX.h>     // Include Adafruit GFX library for graphics
#include <Adafruit_SSD1306.h> // Include Adafruit SSD1306 library for OLED display

// OLED Display Configuration
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 32  // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

// Node identification - each node in the network has a unique ID
#define node_id 2        // This receiver node's ID

// LoRa Radio Module Pin Configuration
#define RFM95_CS 10      // Chip select pin
#define RFM95_RST 9      // Reset pin
#define RFM95_INT 2      // Interrupt pin
 
// LoRa Radio Frequency - must match the transmitter's frequency
#define RF95_FREQ 915.0  // 915 MHz frequency band (can be changed to 433 MHz or other bands)
 
// Create an instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);  // Initialize with the CS and INT pins

// Create an instance of the OLED display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
 
// Software reset function - allows the device to be reset programmatically
void(* resetFunc) (void) = 0; // Declare reset function at address 0

void setup() 
{
  Serial.begin(9600);  // Initialize serial communication at 9600 baud
  delay(100);          // Short delay for stability
  
  // Manual reset of the LoRa module
  digitalWrite(RFM95_RST, LOW);   // Set reset pin low to reset
  delay(10);                      // Wait 10ms
  digitalWrite(RFM95_RST, HIGH);  // Set reset pin high to enable
  delay(10);                      // Wait 10ms for module to stabilize

  // Initialize the OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for most 128x32 OLED displays
    Serial.println(F("SSD1306 allocation failed"));
    // Infinite loop if display initialization fails
    for (;;)
    {
        delay(1000);
    }
  }
  
  // Configure OLED display settings
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.clearDisplay();      // Clear the display buffer

  // Initialize the LoRa radio module
  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    // Display error message on OLED
    display.fillRect(0, 0, SCREEN_WIDTH, 8, BLACK);  // Clear top line
    display.setCursor(0, 0);
    display.println("Setup Failed");
    display.display();  // Update display with new content
    while (1);  // Infinite loop on failure
  }
  Serial.println("LoRa radio init OK!");

  // Set the LoRa radio frequency
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);  // Infinite loop on failure
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
  
  // Display success message on OLED
  display.fillRect(0, 0, SCREEN_WIDTH, 8, BLACK);  // Clear top line
  display.setCursor(0, 0);
  display.println("Setup Successful");
  display.display();  // Update display with new content
  
  // Note: Default LoRa settings after initialization:
  // - Frequency: 915.0MHz
  // - Power: 13dBm
  // - Bandwidth: 125 kHz
  // - Coding Rate: 4/5
  // - Spreading Factor: 128chips/symbol
  // - CRC: enabled

  // Set the transmitter power (used for acknowledgment packets)
  rf95.setTxPower(13, false);  // 13 dBm power, no RFO pin (using PA_BOOST)
}

// Function to validate the checksum of received data
// Uses XOR (exclusive OR) of all bytes in the message to verify data integrity
bool validateChecksum(uint8_t *data, uint8_t len, uint8_t receivedChecksum) {
    uint8_t calculatedChecksum = 0;
    // Calculate checksum by XORing all bytes in the message (excluding the checksum byte)
    for (uint8_t i = 0; i < len; i++) {
        calculatedChecksum ^= data[i];  // XOR checksum
    }
    // Compare calculated checksum with the received checksum
    return calculatedChecksum == receivedChecksum;
}

void loop()
{
  // Check if there's any incoming LoRa packet
  if (rf95.available())
  {
    // Buffer to store the received message
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];  // Maximum message length supported by RadioHead
    uint8_t len = sizeof(buf);             // Length of the buffer
    
    // Update display to show we're waiting for a message
    display.fillRect(0, 0, SCREEN_WIDTH, 8, BLACK);  // Clear top line
    display.setCursor(0, 0);
    display.println("Waiting for Reply");
    display.display();

    // Try to receive the message
    if (rf95.recv(buf, &len))
    {
      // Check for valid message format - first byte should be 0xAA (start byte)
      if (buf[0] != 0xAA) {
        Serial.println("Invalid message start byte");
        // Print the message anyway for debugging
        for (int i = 0; i < len - 1; i++) {  // Exclude checksum and starting
          Serial.print((char)buf[i]);
        }
        Serial.println();
        return;  // Skip further processing
      }
      
      // Parse message header fields
      uint8_t destID = buf[1];       // Destination node ID
      uint8_t senderID = buf[2];     // Sender node ID
      uint8_t payloadLen = buf[3];   // Length of the payload
      uint8_t receivedChecksum = buf[len - 1];  // Last byte is the checksum
      
      // Validate message integrity using checksum
      if (!validateChecksum(buf, len - 1, receivedChecksum)) {
          Serial.println("Checksum mismatch");
          return;  // Skip further processing if checksum is invalid
      }

      // Check if this message is intended for this node
      // Accept messages addressed to this node's ID or broadcast messages (0xFF)
      if (destID != node_id && destID != 0xFF) {  // 0xFF is for broadcast
          Serial.println("Message not for this node");
          return;  // Skip further processing
      }

      // Print the received message in hexadecimal format for debugging
      RH_RF95::printBuffer("Received: ", buf, len);
      
      // Print the actual message content (payload)
      Serial.print("Got: ");
      for (int i = 4; i < len - 1; i++) {  // Start from index 4 (after header) and exclude checksum
        Serial.print((char)buf[i]);
      }
      Serial.println();
      
      // Print the signal strength (RSSI) of the received message
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);

      // Update display to show message received
      display.fillRect(0, 0, SCREEN_WIDTH, 8, BLACK);  // Clear top line
      display.setCursor(0, 0);
      display.println("Message Received");
      display.display();
      
      // Send an acknowledgment reply
      uint8_t data[] = "ACKPACKET";  // Acknowledgment message
      rf95.send(data, sizeof(data));  // Send the acknowledgment
      rf95.waitPacketSent();          // Wait until packet is sent
      Serial.println("Sent a reply");
      
      // Update display to show sending acknowledgment
      display.fillRect(0, 0, SCREEN_WIDTH, 8, BLACK);  // Clear top line
      display.setCursor(0, 0);
      display.println("Sending Message");
      display.display();
    }
    else
    {
      // If message reception failed
      Serial.println("Receive failed");
    }
  }
}
