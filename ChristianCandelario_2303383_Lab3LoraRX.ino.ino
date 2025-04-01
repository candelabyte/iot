//RX

#include <SPI.h>
#include <RH_RF95.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 32  // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)

#define node_id 2

#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 2
 
// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0
 
// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
 
void(* resetFunc) (void) = 0; //declare reset function at address 0

void setup() 
{
  Serial.begin(9600);
  delay(100);
  
  // Manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address or 0x3D for
    Serial.println(F("SSD1306 allocation failed"));
      for (;;)
      {
          delay(1000);
      }
    }
      // Setup oled display
    display.setTextSize(1);      // Normal 1:1 pixel scale
    display.setTextColor(WHITE); // Draw white text
    display.setCursor(0, 0);     // Start at top-left corner
    display.clearDisplay();

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
   display.fillRect(0, 0, SCREEN_WIDTH, 8, BLACK);
    display.setCursor(0, 0);
    display.println("Setup Failed");
    display.display();
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 915.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
 display.fillRect(0, 0, SCREEN_WIDTH, 8, BLACK);
  display.setCursor(0, 0);
  display.println("Setup Successful");
  display.display();
  // Defaults after init are 915.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(13, false);
}

bool validateChecksum(uint8_t *data, uint8_t len, uint8_t receivedChecksum) {
    uint8_t calculatedChecksum = 0;
    for (uint8_t i = 0; i < len; i++) {
        calculatedChecksum ^= data[i];  // XOR checksum
    }
    return calculatedChecksum == receivedChecksum;
}

void loop()
{
  if (rf95.available())
  {
    // Should be a message for us now   
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    display.fillRect(0, 0, SCREEN_WIDTH, 8, BLACK);
    display.setCursor(0, 0);
    display.println("Waiting for Reply");
    display.display();

    if (rf95.recv(buf, &len))
    {
      if (buf[0] != 0xAA) {
        Serial.println("Invalid message start byte");
        for (int i = 0; i < len - 1; i++) {  // Exclude checksum and starting
          Serial.print((char)buf[i]);
        }
        Serial.println();
        return;
      }
      uint8_t destID = buf[1];
      uint8_t senderID = buf[2];
      uint8_t payloadLen = buf[3];
      uint8_t receivedChecksum = buf[len - 1];
      // Validate checksum
      if (!validateChecksum(buf, len - 1, receivedChecksum)) {
          Serial.println("Checksum mismatch");
          return;
      }

      // Filter messages by destination ID
      if (destID != node_id && destID != 0xFF) {  // 0xFF is for broadcast
          Serial.println("Message not for this node");
          return;
      }

      RH_RF95::printBuffer("Received: ", buf, len);
      Serial.print("Got: ");
      for (int i = 4; i < len - 1; i++) {  // Exclude checksum and starting
        Serial.print((char)buf[i]);
      }
      Serial.println();
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);

      display.fillRect(0, 0, SCREEN_WIDTH, 8, BLACK);
      display.setCursor(0, 0);
      display.println("Message Received");
      display.display();  
      // Send a reply
      uint8_t data[] = "ACKPACKET";
      rf95.send(data, sizeof(data));
      rf95.waitPacketSent();
      Serial.println("Sent a reply");
      display.fillRect(0, 0, SCREEN_WIDTH, 8, BLACK);
      display.setCursor(0, 0);
      display.println("Sending Message");
      display.display();
    }
    else
    {
      Serial.println("Receive failed");
    }
  }
}