#pragma once

#include "simulator/firmware/firmware_base.hpp"
#include <painlessMesh.h>

/**
 * @brief Firmware adapter for basic.ino example
 * 
 * This wraps the basic example sketch logic for testing with the simulator.
 * It allows running the exact same code that runs on ESP32/ESP8266 hardware
 * in a simulated environment with 100+ virtual nodes.
 */
class BasicFirmware : public FirmwareBase {
public:
    BasicFirmware() : mesh_(nullptr), userScheduler_(nullptr) {}
    
    ~BasicFirmware() override {
        if (taskSendMessage_) {
            delete taskSendMessage_;
        }
    }

    void setup(painlessMesh* mesh, Scheduler* userScheduler) override {
        mesh_ = mesh;
        userScheduler_ = userScheduler;
        
        // Same configuration as basic.ino
        const char* MESH_PREFIX = "whateverYouLike";
        const char* MESH_PASSWORD = "somethingSneaky";
        const uint16_t MESH_PORT = 5555;
        
        // Set debug message types (same as basic.ino)
        mesh_->setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
        
        // Initialize mesh
        mesh_->init(MESH_PREFIX, MESH_PASSWORD, userScheduler_, MESH_PORT);
        
        // Register callbacks
        mesh_->onReceive([this](uint32_t from, String& msg) {
            this->receivedCallback(from, msg);
        });
        
        mesh_->onNewConnection([this](uint32_t nodeId) {
            this->newConnectionCallback(nodeId);
        });
        
        mesh_->onChangedConnections([this]() {
            this->changedConnectionCallback();
        });
        
        mesh_->onNodeTimeAdjusted([this](int32_t offset) {
            this->nodeTimeAdjustedCallback(offset);
        });
        
        // Setup periodic message sending task
        taskSendMessage_ = new Task(TASK_SECOND * 1, TASK_FOREVER, 
            [this]() { this->sendMessage(); });
        userScheduler_->addTask(*taskSendMessage_);
        taskSendMessage_->enable();
    }

    void loop() override {
        if (mesh_) {
            mesh_->update();
        }
    }

    const char* getName() const override {
        return "BasicExample";
    }

private:
    painlessMesh* mesh_;
    Scheduler* userScheduler_;
    Task* taskSendMessage_ = nullptr;
    
    void sendMessage() {
        if (!mesh_) return;
        
        String msg = "Hello from node ";
        msg += mesh_->getNodeId();
        mesh_->sendBroadcast(msg);
        
        // Random interval between 1 and 5 seconds (same as basic.ino)
        taskSendMessage_->setInterval(random(TASK_SECOND * 1, TASK_SECOND * 5));
    }
    
    void receivedCallback(uint32_t from, String& msg) {
        // In simulator, we can log to stdout or collect metrics
        printf("Node %u: Received from %u msg=%s\n", 
               mesh_->getNodeId(), from, msg.c_str());
        
        // Track metrics for validation
        messagesReceived_++;
    }
    
    void newConnectionCallback(uint32_t nodeId) {
        printf("Node %u: New Connection, nodeId = %u\n", 
               mesh_->getNodeId(), nodeId);
        connectionsEstablished_++;
    }
    
    void changedConnectionCallback() {
        printf("Node %u: Changed connections\n", mesh_->getNodeId());
        topologyChanges_++;
    }
    
    void nodeTimeAdjustedCallback(int32_t offset) {
        printf("Node %u: Adjusted time %u. Offset = %d\n", 
               mesh_->getNodeId(), mesh_->getNodeTime(), offset);
    }
    
    // Metrics for test validation
    uint32_t messagesReceived_ = 0;
    uint32_t connectionsEstablished_ = 0;
    uint32_t topologyChanges_ = 0;
};
