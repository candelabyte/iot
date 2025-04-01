// Mesh Network Lab - Collaborative work by
// Nelson, Hakam, Vignesh, fawaz, Hilman, Christian, Benedict, Danish, Syahmi

#include "painlessMesh.h"   // Include the painlessMesh library for creating mesh networks
#include <M5StickCPlus.h>   // Include the M5StickCPlus library for the M5StickC Plus device

// Mesh network configuration
#define   MESH_PREFIX     "P20Group"    // Name of the mesh network
#define   MESH_PASSWORD   "P20P20P20"   // Password for the mesh network
#define   MESH_PORT       5555          // Port used for the mesh network communication

Scheduler userScheduler; // Scheduler to control tasks
painlessMesh  mesh;      // Main mesh network object

// Function prototype declaration
void sendMessage(); // Prototype to avoid compiler warnings

// Task definition for sending messages periodically
// Parameters: interval (1 second), repetition (forever), callback function
Task taskSendMessage( TASK_SECOND * 1 , TASK_FOREVER, &sendMessage );

// Function to send a message to all nodes in the mesh network
void sendMessage() {
  uint32_t nodeId = mesh.getNodeId();  // Get this device's unique Node ID
  
  // Create a JSON-formatted message containing the node ID and a greeting
  String msg = "{\"nodeId\": " + String(nodeId) + ", \"message\": \"Hello from Nelson M5Stick\"}";
  
  // Log the message to serial monitor
  Serial.printf("Sending: %s\n", msg.c_str());
  
  // Broadcast the message to all nodes in the mesh
  mesh.sendBroadcast(msg);

  // Randomize the next message interval between 1 and 5 seconds
  // This helps prevent message collisions in the network
  taskSendMessage.setInterval(random(TASK_SECOND * 1, TASK_SECOND * 5));
}

// Callback function that executes when a message is received from another node
void receivedCallback(uint32_t from, String &msg) {
  // Log the sender's ID and the message content to serial monitor
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
}

// Callback function that executes when a new node connects to the mesh
void newConnectionCallback(uint32_t nodeId) {
  // Log the new node's ID to serial monitor
  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

// Callback function that executes when the mesh network topology changes
void changedConnectionCallback() {
  // Log that connections have changed
  Serial.printf("Changed connections\n");
}

// Callback function for time synchronization events
// The mesh network synchronizes time across all nodes
void nodeTimeAdjustedCallback(int32_t offset) {
  // Log the current mesh time and the adjustment offset
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void setup() {
  M5.begin();                  // Initialize the M5StickCPlus
  M5.Lcd.setRotation(3);       // Set display rotation for landscape mode
  M5.Lcd.fillScreen(TFT_BLACK);// Clear the display with black color
  M5.Lcd.setTextSize(2);       // Set text size
  M5.Lcd.setTextColor(TFT_WHITE); // Set text color to white

  Serial.begin(115200);        // Initialize serial communication at 115200 baud rate

  // Debug message configuration
  // Uncomment the following line for more detailed debugging output:
  // mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE );
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // Enable only basic debug messages (errors and startup info)

  // Initialize the mesh network with the configured settings
  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  
  // Register callback functions for different mesh events
  mesh.onReceive(&receivedCallback);              // Message received
  mesh.onNewConnection(&newConnectionCallback);   // New node connected
  mesh.onChangedConnections(&changedConnectionCallback); // Network topology changed
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);   // Time synchronization

  // Display the node ID on the M5StickC Plus screen
  uint32_t nodeId = mesh.getNodeId();
  M5.Lcd.setCursor(10, 20);
  M5.Lcd.printf("Node ID:\n%u", nodeId);

  // Set up the periodic message sending task
  userScheduler.addTask(taskSendMessage);  // Add the task to the scheduler
  taskSendMessage.enable();                // Enable the task to start running
}

void loop() {
  M5.update();    // Update the M5StickCPlus state (button readings, etc.)
  mesh.update();  // Update the mesh network (process messages, maintain connections)
                  // This must be called regularly to keep the mesh network functioning
}
