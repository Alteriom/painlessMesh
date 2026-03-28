//************************************************************
// AlteriomPainlessMesh – MPPT Charge Controller Example
//
// This sketch shows how to use a custom package (MpptPackage)
// to broadcast solar charge-controller telemetry over a
// painlessMesh network.
//
// Hardware assumptions:
//   - An MPPT charge controller connected via Serial / RS-485
//     (e.g. Renegy, Epever, Victron BlueSolar, etc.)
//   - ESP8266 or ESP32 running this firmware
//
// How it works:
//   1. Every 10 seconds the node reads the charge controller
//      and broadcasts an MpptPackage to all mesh nodes.
//   2. Any node that receives an MpptPackage prints the values
//      to Serial (bridge nodes can forward them to MQTT/HTTP).
//   3. A CommandPackage handler is included so a bridge node can
//      request an immediate reading (command code 10).
//
// See alteriom_custom_package_template.hpp for a step-by-step
// guide to creating your own custom packages.
//************************************************************

#include "AlteriomPainlessMesh.h"
#include "alteriom_custom_package_template.hpp"
#include "alteriom_sensor_package.hpp"  // for CommandPackage

#define MESH_PREFIX   "AlteriomMesh"
#define MESH_PASSWORD "somethingSneaky"
#define MESH_PORT     5555

// How often to broadcast MPPT data (milliseconds)
#define MPPT_SEND_INTERVAL 10000

Scheduler userScheduler;
painlessMesh mesh;

using namespace alteriom;

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------
void sendMpptData();
void handleIncomingPackage(uint32_t from, String& msg);
void handleCommandPackage(CommandPackage& cmd);
void newConnectionCallback(uint32_t nodeId);
void changedConnectionCallback();
void nodeTimeAdjustedCallback(int32_t offset);

// ---------------------------------------------------------------------------
// Scheduled tasks
// ---------------------------------------------------------------------------
Task taskSendMppt(MPPT_SEND_INTERVAL, TASK_FOREVER, &sendMpptData);

// ---------------------------------------------------------------------------
// Stub: read data from your MPPT controller
//
// Replace this function body with real hardware reads, e.g.:
//   - Modbus RTU over RS-485 (using ModbusMaster library)
//   - Vendor protocol over SoftwareSerial / HardwareSerial
//   - I²C registers for controllers that support it
// ---------------------------------------------------------------------------
MpptPackage readMpptController() {
  MpptPackage data;
  data.from      = mesh.getNodeId();
  data.deviceId  = mesh.getNodeId();
  data.timestamp = mesh.getNodeTime() / 1000;  // convert µs → s

  // --- Replace these lines with real hardware reads ---
  data.solarVoltage   = 18.5f;   // V  (example: 18.5 V PV panel)
  data.solarCurrent   = 5.2f;    // A
  data.solarPower     = 96;      // W  (= solarVoltage * solarCurrent)
  data.batteryVoltage = 12.6f;   // V  (12 V lead-acid, ~80 % SOC)
  data.batterySOC     = 80;      // %
  data.loadVoltage    = 12.4f;   // V
  data.loadCurrent    = 2.1f;    // A
  data.chargeState    = CHARGE_MPPT;
  data.controllerTemp = 32;      // °C
  // ----------------------------------------------------

  return data;
}

// ---------------------------------------------------------------------------
// Setup
// ---------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&handleIncomingPackage);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask(taskSendMppt);
  taskSendMppt.enable();

  Serial.println("MPPT node initialised");
}

// ---------------------------------------------------------------------------
// Loop
// ---------------------------------------------------------------------------
void loop() {
  mesh.update();
}

// ---------------------------------------------------------------------------
// Broadcast MPPT telemetry
// ---------------------------------------------------------------------------
void sendMpptData() {
  MpptPackage data = readMpptController();

  // Serialise with ArduinoJson v7 API
  JsonDocument doc;
  JsonObject obj = doc.to<JsonObject>();
  data.addTo(std::move(obj));

  String msg;
  serializeJson(doc, msg);

  if (mesh.sendBroadcast(msg)) {
    Serial.printf(
        "MPPT sent: PV=%.1fV/%.1fA/%dW  Bat=%.1fV/%d%%  "
        "Load=%.1fV/%.1fA  State=%d  Temp=%d°C\n",
        data.solarVoltage, data.solarCurrent, data.solarPower,
        data.batteryVoltage, data.batterySOC,
        data.loadVoltage, data.loadCurrent,
        data.chargeState, data.controllerTemp);
  } else {
    Serial.println("sendBroadcast failed – check mesh connectivity");
  }
}

// ---------------------------------------------------------------------------
// Receive handler
// ---------------------------------------------------------------------------
void handleIncomingPackage(uint32_t from, String& msg) {
  JsonDocument doc;
  if (deserializeJson(doc, msg) != DeserializationError::Ok) {
    Serial.printf("JSON parse error from %u\n", from);
    return;
  }

  JsonObject obj = doc.as<JsonObject>();
  uint16_t msgType = obj["type"];

  switch (msgType) {
    case 205: {  // MpptPackage
      MpptPackage received(obj);
      Serial.printf(
          "MPPT from %u: PV=%.1fV/%.1fA/%dW  Bat=%.1fV/%d%%  "
          "Load=%.1fV/%.1fA  State=%d  Temp=%d°C\n",
          received.from,
          received.solarVoltage, received.solarCurrent, received.solarPower,
          received.batteryVoltage, received.batterySOC,
          received.loadVoltage, received.loadCurrent,
          received.chargeState, received.controllerTemp);
      break;
    }

    case 400: {  // CommandPackage
      CommandPackage cmd(obj);
      if (cmd.dest == mesh.getNodeId()) {
        handleCommandPackage(cmd);
      }
      break;
    }

    default:
      // Silently ignore unknown types
      break;
  }
}

// ---------------------------------------------------------------------------
// Command handler
// ---------------------------------------------------------------------------
void handleCommandPackage(CommandPackage& cmd) {
  switch (cmd.command) {
    case 10:  // Immediate MPPT data request
      Serial.printf("Immediate data request from command %u\n", cmd.commandId);
      sendMpptData();
      break;

    default:
      Serial.printf("Unknown command %d\n", cmd.command);
      break;
  }
}

// ---------------------------------------------------------------------------
// Mesh callbacks
// ---------------------------------------------------------------------------
void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New connection: nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.println("Connections changed");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Time adjusted: offset = %d µs\n", offset);
}
