#ifndef ALTERIOM_CUSTOM_PACKAGE_TEMPLATE_HPP
#define ALTERIOM_CUSTOM_PACKAGE_TEMPLATE_HPP

#include "painlessmesh/plugin.hpp"

/**
 * @file alteriom_custom_package_template.hpp
 * @brief Template and example for creating custom Alteriom packages
 *
 * HOW TO CREATE A CUSTOM PACKAGE
 * ===============================
 *
 * This file serves two purposes:
 *   1. A step-by-step guide to creating custom painlessMesh packages
 *   2. A concrete example: MpptPackage for MPPT solar charge controllers
 *
 * QUICK START
 * -----------
 * To create your own custom package:
 *   1. Pick an unused Type ID from the table below (use 203+ range)
 *   2. Choose a base class: BroadcastPackage (all nodes) or SinglePackage (one
 *      node)
 *   3. Add your data fields with appropriate types
 *   4. Implement the JSON constructor and addTo() method
 *   5. Add a test in test/catch/catch_custom_package.cpp
 *
 * RESERVED TYPE IDS
 * -----------------
 * The following IDs are already used; do NOT reuse them:
 *
 *   200 : SensorPackage      (environmental sensors: temp, humidity, pressure)
 *   202 : StatusPackage      (device health and configuration)
 *   203 : MpptPackage        (MPPT solar charge controller data)  <-- this file
 *   204 : MetricsPackage     (network performance metrics)
 *   400 : CommandPackage     (device control commands)
 *   600 : MeshNodeListPackage
 *   601 : MeshTopologyPackage
 *   602 : MeshAlertPackage
 *   603 : MeshBridgePackage
 *   604 : EnhancedStatusPackage
 *   605 : HealthCheckPackage
 *   610 : BridgeStatusPackage
 *   611 : BridgeElectionPackage
 *   612 : BridgeTakeoverPackage
 *   614 : NTPTimeSyncPackage
 *
 * Available ranges: 205-399 (add your package here and update this table).
 *
 *
 * CHOOSING BASE CLASS
 * -------------------
 *
 * BroadcastPackage  – sent to every node in the mesh.
 *   Use for: sensor readings, status updates, telemetry data.
 *   Base fields: from, routing (BROADCAST), type  (noJsonFields = 3)
 *
 * SinglePackage     – sent to one specific destination node.
 *   Use for: commands, acknowledgements, targeted responses.
 *   Base fields: from, dest, routing (SINGLE), type  (noJsonFields = 4)
 *
 *
 * FIELD TYPE GUIDELINES
 * ---------------------
 *
 * Choose types appropriate for your platform:
 *
 *   uint8_t   – flags, states, small counts (0-255)
 *   uint16_t  – larger counts, port numbers, voltages in mV (0-65535)
 *   uint32_t  – device IDs, Unix timestamps, large counters
 *   int8_t    – signed small values, e.g. temperature in °C (-128 to +127)
 *   float     – measured values requiring decimals (4 bytes; fine on both
 *               ESP8266 and ESP32)
 *   double    – high-precision measurements (8 bytes; prefer float on ESP8266)
 *   TSTRING   – text strings (always use TSTRING, NOT Arduino String)
 *   bool      – boolean flags; see BOOLEAN NAMING CONVENTION below
 *
 * BOOLEAN NAMING CONVENTION
 * -------------------------
 *   *Set suffix     – configuration data has been provided
 *                     e.g., serverAddressSet = true
 *   *Enabled suffix – feature is currently active/on
 *                     e.g., loggingEnabled = true
 *   is* prefix      – current runtime state
 *                     e.g., isCharging = true
 *
 *
 * JSON FIELD NAMING
 * -----------------
 *
 * Use SHORT keys to minimise over-the-air message sizes:
 *
 *   batteryVoltage  ->  "bv"
 *   solarCurrent    ->  "sc"
 *   chargeState     ->  "cs"
 *   deviceId        ->  "did"
 *   timestamp       ->  "ts"
 *
 * Always document the mapping in a comment near the field declaration.
 *
 *
 * TIME FIELDS
 * -----------
 *
 * For interval / duration fields, follow the Alteriom time convention:
 *   - Store internally in milliseconds (uint32_t)
 *   - Serialise both a _ms and a _s variant in JSON
 *   - Deserialise from the _ms variant only
 *
 * Timestamp fields (Unix epoch seconds) are an exception: single field, no
 * dual-unit serialisation needed.
 *
 *
 * ARDUINOJSON COMPATIBILITY
 * -------------------------
 *
 * Always wrap the jsonObjectSize() method in an
 * #if ARDUINOJSON_VERSION_MAJOR < 7 guard.  ArduinoJson v7 computes document
 * sizes automatically; v6 requires an explicit capacity hint.
 *
 * The formula is:
 *   JSON_OBJECT_SIZE(noJsonFields + <number of your own fields>)
 *   + <total length of all TSTRING fields>
 *
 *
 * MINIMAL PACKAGE TEMPLATE
 * ========================
 *
 * Copy this skeleton and replace the placeholder names / IDs:
 *
 * @code
 * namespace alteriom {
 *
 * class MyCustomPackage : public painlessmesh::plugin::BroadcastPackage {
 *  public:
 *   // --- Your data fields ---
 *   uint32_t myId    = 0;
 *   float    myValue = 0.0f;
 *   TSTRING  myText  = "";
 *
 *   // MQTT message_type (set to your chosen type ID)
 *   uint16_t messageType = 205;
 *
 *   MyCustomPackage() : BroadcastPackage(205) {}
 *
 *   MyCustomPackage(JsonObject jsonObj) : BroadcastPackage(jsonObj) {
 *     myId        = jsonObj["id"];
 *     myValue     = jsonObj["val"];
 *     myText      = jsonObj["txt"].as<TSTRING>();
 *     messageType = jsonObj["message_type"] | 205;
 *   }
 *
 *   JsonObject addTo(JsonObject&& jsonObj) const {
 *     jsonObj = BroadcastPackage::addTo(std::move(jsonObj));
 *     jsonObj["id"]           = myId;
 *     jsonObj["val"]          = myValue;
 *     jsonObj["txt"]          = myText;
 *     jsonObj["message_type"] = messageType;
 *     return jsonObj;
 *   }
 *
 * #if ARDUINOJSON_VERSION_MAJOR < 7
 *   size_t jsonObjectSize() const {
 *     // noJsonFields covers base-class fields; 3 = number of YOUR fields
 *     return JSON_OBJECT_SIZE(noJsonFields + 3) + myText.length();
 *   }
 * #endif
 * };
 *
 * } // namespace alteriom
 * @endcode
 *
 *
 * CONCRETE EXAMPLE: MpptPackage
 * ==============================
 *
 * The MpptPackage (Type 203) transmits real-time telemetry from an MPPT solar
 * charge controller (e.g. Renegy, Epever, Victron).  It is a BroadcastPackage
 * so every node in the mesh receives the data automatically.
 *
 * Fields at a glance:
 *
 *   solarVoltage    (float,   V)   – PV panel open-circuit / input voltage
 *   solarCurrent    (float,   A)   – PV panel current
 *   solarPower      (uint16_t, W)  – PV panel instantaneous power
 *   batteryVoltage  (float,   V)   – Battery terminal voltage
 *   batterySOC      (uint8_t, %)   – State of charge 0–100
 *   loadVoltage     (float,   V)   – Load output voltage
 *   loadCurrent     (float,   A)   – Load output current
 *   chargeState     (uint8_t)      – Controller state (see ChargeState enum)
 *   controllerTemp  (int8_t,  °C)  – Internal controller temperature
 *   deviceId        (uint32_t)     – Unique hardware identifier
 *   timestamp       (uint32_t)     – Unix timestamp of the reading
 */

namespace alteriom {

/**
 * @brief Charge state values for MpptPackage::chargeState
 */
enum ChargeState : uint8_t {
  CHARGE_OFF = 0,       ///< Charging disabled
  CHARGE_NORMAL = 1,    ///< Normal PWM charging
  CHARGE_MPPT = 2,      ///< Maximum Power Point Tracking active
  CHARGE_EQUALIZE = 3,  ///< Equalisation charge (battery maintenance)
  CHARGE_BOOST = 4,     ///< Boost / bulk charge stage
  CHARGE_FLOAT = 5,     ///< Float / maintenance stage
  CHARGE_LIMITED = 6    ///< Current-limited charging
};

/**
 * @brief Real-time telemetry from an MPPT solar charge controller
 *
 * Broadcasts voltage, current, power and status from an MPPT charge controller
 * to all nodes in the mesh (e.g. for logging, display, or load management).
 *
 * Adapting for your controller
 * ----------------------------
 * Most MPPT controllers expose data over RS-232/RS-485 or I²C.  Read the raw
 * values from your hardware, assign them to the struct fields, then call
 * sendBroadcast() as shown in alteriom_mppt_example.ino.
 *
 * Type ID: 203
 */
class MpptPackage : public painlessmesh::plugin::BroadcastPackage {
 public:
  // JSON key : "sv"  – PV panel voltage in Volts
  float solarVoltage = 0.0f;
  // JSON key : "sc"  – PV panel current in Amperes
  float solarCurrent = 0.0f;
  // JSON key : "sp"  – PV panel power in Watts
  uint16_t solarPower = 0;
  // JSON key : "bv"  – Battery terminal voltage in Volts
  float batteryVoltage = 0.0f;
  // JSON key : "bsoc" – Battery state of charge, 0–100 %
  uint8_t batterySOC = 0;
  // JSON key : "lv"  – Load output voltage in Volts
  float loadVoltage = 0.0f;
  // JSON key : "lc"  – Load output current in Amperes
  float loadCurrent = 0.0f;
  // JSON key : "cs"  – Charge controller state (see ChargeState enum)
  uint8_t chargeState = CHARGE_OFF;
  // JSON key : "ct"  – Controller internal temperature in °C (signed)
  int8_t controllerTemp = 0;
  // JSON key : "did" – Unique hardware / node identifier
  uint32_t deviceId = 0;
  // JSON key : "ts"  – Unix timestamp of measurement (seconds since epoch)
  uint32_t timestamp = 0;

  // MQTT Schema message_type for fast classification at the bridge
  uint16_t messageType = 203;  // MPPT_DATA

  // -------------------------------------------------------------------------
  // Constructors
  // -------------------------------------------------------------------------

  MpptPackage() : BroadcastPackage(203) {}

  /**
   * @brief Deserialise from a JSON object received over the mesh
   *
   * @param jsonObj  Parsed JSON object (ArduinoJson JsonObject)
   */
  MpptPackage(JsonObject jsonObj) : BroadcastPackage(jsonObj) {
    solarVoltage = jsonObj["sv"];
    solarCurrent = jsonObj["sc"];
    solarPower = jsonObj["sp"];
    batteryVoltage = jsonObj["bv"];
    batterySOC = jsonObj["bsoc"];
    loadVoltage = jsonObj["lv"];
    loadCurrent = jsonObj["lc"];
    chargeState = jsonObj["cs"];
    controllerTemp = jsonObj["ct"];
    deviceId = jsonObj["did"];
    timestamp = jsonObj["ts"];
    messageType = jsonObj["message_type"] | 203;
  }

  // -------------------------------------------------------------------------
  // Serialisation
  // -------------------------------------------------------------------------

  /**
   * @brief Serialise this package into the provided JSON object
   *
   * Call addTo() on a freshly created JsonObject, then serialise with
   * ArduinoJson's serializeJson() before passing the result to
   * mesh.sendBroadcast().
   */
  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = BroadcastPackage::addTo(std::move(jsonObj));
    jsonObj["sv"] = solarVoltage;
    jsonObj["sc"] = solarCurrent;
    jsonObj["sp"] = solarPower;
    jsonObj["bv"] = batteryVoltage;
    jsonObj["bsoc"] = batterySOC;
    jsonObj["lv"] = loadVoltage;
    jsonObj["lc"] = loadCurrent;
    jsonObj["cs"] = chargeState;
    jsonObj["ct"] = controllerTemp;
    jsonObj["did"] = deviceId;
    jsonObj["ts"] = timestamp;
    jsonObj["message_type"] = messageType;
    return jsonObj;
  }

#if ARDUINOJSON_VERSION_MAJOR < 7
  /**
   * @brief Required capacity hint for ArduinoJson v6
   *
   * noJsonFields covers the 3 base-class fields (from, routing, type).
   * The +12 accounts for the 12 fields declared in this class.
   * No TSTRING fields, so no extra string length term.
   */
  size_t jsonObjectSize() const { return JSON_OBJECT_SIZE(noJsonFields + 12); }
#endif
};

}  // namespace alteriom

#endif  // ALTERIOM_CUSTOM_PACKAGE_TEMPLATE_HPP
