//Collaborative work by
// Nelson, Hakam, Vignesh, fawaz, Hilman, Christian, Benedict, Danish, Syahmi

#include "painlessMesh.h"
#include <M5StickCPlus.h>

#define   MESH_PREFIX     "P20Group"
#define   MESH_PASSWORD   "P20P20P20"
#define   MESH_PORT       5555

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;

// User stub
void sendMessage(); // Prototype to avoid compiler warnings

Task taskSendMessage( TASK_SECOND * 1 , TASK_FOREVER, &sendMessage );

void sendMessage() {
  uint32_t nodeId = mesh.getNodeId();  // Get Node ID
  String msg = "{\"nodeId\": " + String(nodeId) + ", \"message\": \"Hello from Nelson M5Stick\"}";
  
  Serial.printf("Sending: %s\n", msg.c_str());
  mesh.sendBroadcast(msg);

  // Randomize next message interval between 1 and 5 seconds
  taskSendMessage.setInterval(random(TASK_SECOND * 1, TASK_SECOND * 5));
}

// Callback for receiving messages
void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
}

// Callback for new connections
void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

// Callback for when connections change
void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

// Callback for time adjustments
void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void setup() {
  M5.begin();  // Initialize the M5StickCPlus
  M5.Lcd.setRotation(3);  // Set display rotation for landscape mode
  M5.Lcd.fillScreen(TFT_BLACK);  // Clear the display
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_WHITE);

  Serial.begin(115200);

  // Uncomment the following line for more detailed debugging output:
  // mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE );
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // Enable basic debug messages

  // Initialize the mesh network
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  // Display the node ID on startup
  uint32_t nodeId = mesh.getNodeId();
  M5.Lcd.setCursor(10, 20);
  M5.Lcd.printf("Node ID:\n%u", nodeId);

  // Add and enable the sending task
  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();
}

void loop() {
  // Update the M5StickCPlus state
  M5.update();
  // Update the mesh network
  mesh.update();
}
