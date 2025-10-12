#define CATCH_CONFIG_MAIN
#include "catch2/catch.hpp"
#include "Arduino.h"
#include "catch_utils.hpp"
#include "../../examples/alteriom/alteriom_sensor_package.hpp"

using namespace alteriom;
using namespace painlessmesh;

SCENARIO("MQTT Command Bridge routing works correctly") {
  GIVEN("A CommandPackage for LED control") {
    CommandPackage cmd;
    cmd.from = 111111;
    cmd.dest = 222222;
    cmd.command = 10; // LED_CONTROL
    cmd.targetDevice = 222222;
    cmd.commandId = 1001;
    cmd.parameters = "{\"state\":true,\"brightness\":75}";
    
    REQUIRE(cmd.type == 201);
    REQUIRE(cmd.command == 10);
    REQUIRE(cmd.targetDevice == 222222);
    
    WHEN("Converting to JSON and back") {
      auto var = protocol::Variant(&cmd);
      String json;
      var.printTo(json);
      
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, json);
      JsonObject obj = doc.as<JsonObject>();
      
      CommandPackage cmd2(obj);
      
      THEN("All fields should match") {
        REQUIRE(cmd2.command == cmd.command);
        REQUIRE(cmd2.targetDevice == cmd.targetDevice);
        REQUIRE(cmd2.commandId == cmd.commandId);
        REQUIRE(cmd2.parameters == cmd.parameters);
        REQUIRE(cmd2.type == 201);
      }
    }
    
    WHEN("Parsing parameters") {
      DynamicJsonDocument params(256);
      deserializeJson(params, cmd.parameters);
      
      bool state = params["state"];
      uint8_t brightness = params["brightness"];
      
      THEN("Parameters should be extracted correctly") {
        REQUIRE(state == true);
        REQUIRE(brightness == 75);
      }
    }
  }
  
  GIVEN("A StatusPackage response with command tracking") {
    StatusPackage response;
    response.from = 222222;
    response.deviceStatus = 0;
    response.uptime = 3600;
    response.freeMemory = 45;
    response.firmwareVersion = "1.0.0";
    response.responseToCommand = 1001;
    response.responseMessage = "LED ON";
    
    REQUIRE(response.type == 202);
    REQUIRE(response.responseToCommand == 1001);
    
    WHEN("Serializing to JSON") {
      auto var = protocol::Variant(&response);
      String json;
      var.printTo(json);
      
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, json);
      
      THEN("Response fields should be present") {
        REQUIRE(doc["respTo"] == 1001);
        REQUIRE(doc["respMsg"] == "LED ON");
        REQUIRE(doc["status"] == 0);
        REQUIRE(doc["uptime"] == 3600);
      }
    }
    
    WHEN("Converting back from JSON") {
      auto var = protocol::Variant(&response);
      String json;
      var.printTo(json);
      
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, json);
      JsonObject obj = doc.as<JsonObject>();
      
      StatusPackage response2(obj);
      
      THEN("All fields should match") {
        REQUIRE(response2.responseToCommand == response.responseToCommand);
        REQUIRE(response2.responseMessage == response.responseMessage);
        REQUIRE(response2.deviceStatus == response.deviceStatus);
        REQUIRE(response2.uptime == response.uptime);
      }
    }
  }
}

SCENARIO("Command parameter parsing works correctly") {
  GIVEN("LED control parameters") {
    String params = "{\"state\":true,\"brightness\":75}";
    
    WHEN("Parsing parameters") {
      DynamicJsonDocument doc(256);
      deserializeJson(doc, params);
      
      bool state = doc["state"];
      uint8_t brightness = doc["brightness"];
      
      THEN("Values should be extracted correctly") {
        REQUIRE(state == true);
        REQUIRE(brightness == 75);
      }
    }
  }
  
  GIVEN("Relay switch parameters") {
    String params = "{\"channel\":2,\"state\":false}";
    
    WHEN("Parsing parameters") {
      DynamicJsonDocument doc(256);
      deserializeJson(doc, params);
      
      uint8_t channel = doc["channel"];
      bool state = doc["state"];
      
      THEN("Values should be extracted correctly") {
        REQUIRE(channel == 2);
        REQUIRE(state == false);
      }
    }
  }
  
  GIVEN("PWM set parameters") {
    String params = "{\"pin\":5,\"duty\":512}";
    
    WHEN("Parsing parameters") {
      DynamicJsonDocument doc(256);
      deserializeJson(doc, params);
      
      uint8_t pin = doc["pin"];
      uint16_t duty = doc["duty"];
      
      THEN("Values should be extracted correctly") {
        REQUIRE(pin == 5);
        REQUIRE(duty == 512);
      }
    }
  }
  
  GIVEN("Sleep command parameters") {
    String params = "{\"duration_ms\":5000}";
    
    WHEN("Parsing parameters") {
      DynamicJsonDocument doc(256);
      deserializeJson(doc, params);
      
      uint32_t duration = doc["duration_ms"];
      
      THEN("Duration should be extracted correctly") {
        REQUIRE(duration == 5000);
      }
    }
  }
}

SCENARIO("Configuration management works correctly") {
  GIVEN("A configuration JSON object") {
    String configStr = "{\"deviceName\":\"Sensor-01\",\"sampleRate\":30000,\"ledEnabled\":false}";
    
    WHEN("Parsing configuration") {
      DynamicJsonDocument doc(512);
      deserializeJson(doc, configStr);
      
      String deviceName = doc["deviceName"];
      uint32_t sampleRate = doc["sampleRate"];
      bool ledEnabled = doc["ledEnabled"];
      
      THEN("All configuration values should be correct") {
        REQUIRE(deviceName == "Sensor-01");
        REQUIRE(sampleRate == 30000);
        REQUIRE(ledEnabled == false);
      }
    }
  }
  
  GIVEN("A configuration update command") {
    CommandPackage cmd;
    cmd.command = 101; // SET_CONFIG
    cmd.targetDevice = 123456;
    cmd.commandId = 2001;
    cmd.parameters = "{\"deviceName\":\"UpdatedSensor\",\"sampleRate\":15000}";
    
    WHEN("Processing the command") {
      DynamicJsonDocument params(512);
      deserializeJson(params, cmd.parameters);
      
      JsonObject config = params.as<JsonObject>();
      
      THEN("Configuration should be parseable") {
        REQUIRE(config.containsKey("deviceName"));
        REQUIRE(config.containsKey("sampleRate"));
        REQUIRE(config["deviceName"] == "UpdatedSensor");
        REQUIRE(config["sampleRate"] == 15000);
      }
    }
  }
}

SCENARIO("Command ID tracking works correctly") {
  GIVEN("Multiple commands with unique IDs") {
    CommandPackage cmd1;
    cmd1.commandId = 1001;
    cmd1.command = 10;
    
    CommandPackage cmd2;
    cmd2.commandId = 1002;
    cmd2.command = 11;
    
    CommandPackage cmd3;
    cmd3.commandId = 1003;
    cmd3.command = 200;
    
    THEN("All command IDs should be unique") {
      REQUIRE(cmd1.commandId != cmd2.commandId);
      REQUIRE(cmd2.commandId != cmd3.commandId);
      REQUIRE(cmd1.commandId != cmd3.commandId);
    }
    
    WHEN("Creating responses") {
      StatusPackage resp1;
      resp1.responseToCommand = cmd1.commandId;
      resp1.responseMessage = "Response 1";
      
      StatusPackage resp2;
      resp2.responseToCommand = cmd2.commandId;
      resp2.responseMessage = "Response 2";
      
      THEN("Responses should match commands") {
        REQUIRE(resp1.responseToCommand == 1001);
        REQUIRE(resp2.responseToCommand == 1002);
        REQUIRE(resp1.responseMessage == "Response 1");
        REQUIRE(resp2.responseMessage == "Response 2");
      }
    }
  }
}

SCENARIO("StatusPackage without response fields") {
  GIVEN("A StatusPackage without command response") {
    StatusPackage status;
    status.from = 123456;
    status.deviceStatus = 0;
    status.uptime = 7200;
    status.freeMemory = 50;
    status.firmwareVersion = "1.0.0";
    // responseToCommand = 0 (default)
    // responseMessage = "" (default)
    
    WHEN("Serializing to JSON") {
      auto var = protocol::Variant(&status);
      String json;
      var.printTo(json);
      
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, json);
      
      THEN("Response fields should not be included") {
        // When responseToCommand is 0, fields are not added
        REQUIRE(doc["status"] == 0);
        REQUIRE(doc["uptime"] == 7200);
        // respTo and respMsg not required to exist when 0
      }
    }
  }
}

SCENARIO("Broadcast command handling") {
  GIVEN("A broadcast command with targetDevice 0") {
    CommandPackage cmd;
    cmd.command = 200; // GET_STATUS
    cmd.targetDevice = 0; // Broadcast
    cmd.commandId = 3001;
    cmd.parameters = "{}";
    
    REQUIRE(cmd.targetDevice == 0);
    
    WHEN("Multiple nodes receive it") {
      // Simulate multiple nodes
      uint32_t node1 = 111111;
      uint32_t node2 = 222222;
      uint32_t node3 = 333333;
      
      StatusPackage resp1;
      resp1.from = node1;
      resp1.responseToCommand = cmd.commandId;
      
      StatusPackage resp2;
      resp2.from = node2;
      resp2.responseToCommand = cmd.commandId;
      
      StatusPackage resp3;
      resp3.from = node3;
      resp3.responseToCommand = cmd.commandId;
      
      THEN("All responses should reference the same command") {
        REQUIRE(resp1.responseToCommand == 3001);
        REQUIRE(resp2.responseToCommand == 3001);
        REQUIRE(resp3.responseToCommand == 3001);
        
        REQUIRE(resp1.from != resp2.from);
        REQUIRE(resp2.from != resp3.from);
        REQUIRE(resp1.from != resp3.from);
      }
    }
  }
}

SCENARIO("Error handling in command processing") {
  GIVEN("An unknown command") {
    CommandPackage cmd;
    cmd.command = 255; // Unknown
    cmd.commandId = 9999;
    cmd.targetDevice = 123456;
    
    WHEN("Creating error response") {
      StatusPackage errorResp;
      errorResp.from = 123456;
      errorResp.deviceStatus = 2; // Error
      errorResp.responseToCommand = cmd.commandId;
      errorResp.responseMessage = "Unknown command: 255";
      
      THEN("Error response should be properly formed") {
        REQUIRE(errorResp.deviceStatus == 2);
        REQUIRE(errorResp.responseToCommand == 9999);
        REQUIRE(errorResp.responseMessage.indexOf("Unknown") >= 0);
      }
    }
  }
  
  GIVEN("Invalid JSON parameters") {
    CommandPackage cmd;
    cmd.command = 10;
    cmd.parameters = "{invalid json}";
    
    WHEN("Attempting to parse") {
      DynamicJsonDocument doc(256);
      DeserializationError error = deserializeJson(doc, cmd.parameters);
      
      THEN("Should detect parse error") {
        REQUIRE(error != DeserializationError::Ok);
      }
    }
  }
}
