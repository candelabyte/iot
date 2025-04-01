
// LoRa Transmitter (TX) Node Implementation

#include <SPI.h>              // Include SPI library for communication with the LoRa module
#include <RH_RF95.h>          // Include RadioHead library for LoRa radio
#include <Wire.h>             // Include Wire library for I2C communication with the OLED display
#include <Adafruit_GFX.h>     // Include Adafruit GFX library for graphics
#include <Adafruit_SSD1306.h> // Include Adafruit SSD1306 library for OLED display

// OLED Display Configuration
#define SCREEN_WIDTH 128   // OLED display width, in pixels
#define SCREEN_HEIGHT 32   // OLED display height, in pixels
#define OLED_RESET    -1   // Reset pin # (or -1 if sharing Arduino reset pin)

// LoRa Radio Module Pin Configuration
#define RFM95_CS 10        // Chip select pin
#define RFM95_RST 9        // Reset pin
#define RFM95_INT 2        // Interrupt pin
#define node_id 1          // This transmitter node's ID
 
// LoRa Radio Frequency - must match the receiver's frequency
#define RF95_FREQ 915.0    // 915 MHz frequency band (can be changed to 433 MHz or other bands)

// Create an instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);  // Initialize with the CS and INT pins

// Create an instance of the OLED display
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Flag to alternate between destination nodes
bool toggle_node = false;  // Used to toggle between sending to node 2 and node 3

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
  Serial.print("Set Freq to:"); Serial.println(RF95_FREQ);
  
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

  // Set the transmitter power
  rf95.setTxPower(13, false);  // 13 dBm power, no RFO pin (using PA_BOOST)
}
// Packet counter to track number of messages sent
int16_t packetnum = 0;  // Incremented with each transmission

// Function to send a formatted message with protocol headers and checksum
// Parameters:
// - destID: ID of the destination node
// - senderID: ID of this sender node
// - message: Payload string to send
void sendMessage(uint8_t destID, uint8_t senderID, const char *message) {
    // Get the length of the message payload
    uint8_t payloadLen = strlen(message);
    
    // Check if message is too long for the packet buffer
    // We need 5 extra bytes: start byte, destID, senderID, payloadLen, and checksum
    if (payloadLen > RH_RF95_MAX_MESSAGE_LEN - 5) {
        Serial.println("Message too long!");
        return;
    }

    // Create the packet buffer
    uint8_t packet[RH_RF95_MAX_MESSAGE_LEN];
    
    // Fill the packet header
    packet[0] = 0xAA;        // Start byte (fixed value 0xAA)
    packet[1] = destID;      // Destination node ID
    packet[2] = senderID;    // Sender node ID
    packet[3] = payloadLen;  // Length of the payload
    
    // Copy the message payload into the packet after the header
    memcpy(&packet[4], message, payloadLen);

    // Calculate checksum using XOR of all bytes in the packet
    uint8_t checksum = 0;
    for (uint8_t i = 0; i < 4 + payloadLen; i++) {
        checksum ^= packet[i];  // XOR each byte
    }
    
    // Add the checksum as the last byte of the packet
    packet[4 + payloadLen] = checksum;

    // Send the complete packet
    rf95.send(packet, 5 + payloadLen);  // Total length = 4 header bytes + payload + 1 checksum byte
    rf95.waitPacketSent();              // Wait until transmission is complete
    Serial.println("Message sent!");
}

void loop()
{
  // Create a message with incrementing packet number
  uint8_t data[32];  // Buffer for the message
  sprintf(data, "Message %d from %d", int(packetnum++), int(node_id)); 
    
  // Retry configuration
  int max_retries = 3;        // Maximum number of transmission attempts
  bool ack_received = false;  // Flag to track if acknowledgment was received
  
  // Try sending the message up to max_retries times
  for (int attempt = 0; attempt < max_retries; attempt++) {
    Serial.println("Sending to rf95_rx");
    
    // Update display to show sending status
    display.fillRect(0, 0, SCREEN_WIDTH, 8, BLACK);  // Clear top line
    display.setCursor(0, 0);
    display.println("Sending Message");
    display.display();
    
    // Log the current attempt number
    Serial.print("Attempt "); Serial.println(attempt + 1);
    
    // Alternate between sending to node 2 and node 3
    if (toggle_node) {
      sendMessage(2, node_id, (char*)data);  // Send to node 2
    } else {
      sendMessage(3, node_id, (char*)data);  // Send to node 3
    }

    // Toggle the destination for next transmission
    toggle_node = !toggle_node;

    // Wait for acknowledgment with timeout (1000ms)
    if (rf95.waitAvailableTimeout(1000))
    { 
      // Buffer to store the reply
      uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
      uint8_t len = sizeof(buf);

      // Update display to show waiting for reply
      display.fillRect(0, 0, SCREEN_WIDTH, 8, BLACK);  // Clear top line
      display.setCursor(0, 0);
      display.println("Waiting for Reply");
      display.display();
      
      Serial.println("Wait for reply...");
      
      // Try to receive the reply message
      if (rf95.recv(buf, &len))
      {
        // Print the received message
        Serial.print("Got: ");
        for (int i = 0; i < len - 1; i++) {  // Exclude checksum and starting
          Serial.print((char)buf[i]);
        }
        Serial.println();
        
        // Check if the received message is the expected acknowledgment
        if (strcmp((char*)buf, "ACKPACKET") == 0) {  // Check for correct ACK
          ack_received = true;  // Set flag that ACK was received
          
          // Print signal strength of the received ACK
          Serial.print("Got ACK");
          Serial.print("RSSI:");  // Received Signal Strength Indicator
          Serial.println(rf95.lastRssi(), DEC);
          
          // Update display to show ACK received
          display.fillRect(0, 0, SCREEN_WIDTH, 8, BLACK);  // Clear top line
          display.setCursor(0, 0);
          display.println("Message Received");
          display.display();
          
          break;  // Exit retry loop if ACK is received
        };   
      }
    }
    else
    {
      // No reply received within timeout period
      Serial.println("No ACK, retrying..."); 
    }
  }
  
  // After all retry attempts, check if we ever received an ACK
  if(!ack_received)
  {
    Serial.println("No reply received");
  }
  
  // Wait 10 seconds before sending the next message
  delay(10000); 
}
