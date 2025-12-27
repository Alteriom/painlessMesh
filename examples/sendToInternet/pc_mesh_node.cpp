/**
 * PC Mesh Node - Test sendToInternet() from Regular Node Through Bridge
 * 
 * This example demonstrates a PC-based mesh node that can join a painlessMesh
 * network with ESP32/ESP8266 devices and test sendToInternet() functionality
 * as a REGULAR NODE (not a bridge).
 * 
 * PURPOSE:
 * Test the complete flow: PC Node → Bridge → Internet → Bridge → PC Node
 * 
 * CONNECTION METHOD:
 * ESP nodes use WiFi mesh credentials: mesh.init(MESH_PREFIX, MESH_PASSWORD, &scheduler, MESH_PORT)
 * PC node uses TCP/IP connection: connects to bridge's IP:port (e.g., 192.168.1.100:5555)
 * 
 * Both methods result in a mesh node that can use sendToInternet() - the difference
 * is the transport layer (WiFi mesh vs TCP/IP), not the painlessMesh protocol.
 * 
 * SETUP:
 * 1. Configure ESP bridge with: mesh.init("FishFarmMesh", "securepass", &scheduler, 5555)
 * 2. Run this PC node: ./pc_mesh_node <bridge_ip> 5555
 * 3. Run mock HTTP server: cd test/mock-http-server && python3 server.py
 * 4. PC node sends HTTP requests via sendToInternet() through bridge
 * 
 * COMPILE:
 * g++ -std=c++14 -o pc_mesh_node pc_mesh_node.cpp \
 *     -I../../src -I../../test/include -I../../test/ArduinoJson/src \
 *     -I../../test/TaskScheduler/src -I../../test/boost \
 *     ../../src/scheduler.cpp \
 *     -lboost_system -pthread
 * 
 * Or use CMake (see CMakeLists.txt in this directory)
 * 
 * RUN:
 * ./pc_mesh_node 192.168.1.100 5555
 * 
 * Where:
 * - 192.168.1.100 is your ESP bridge's IP address on the LAN
 * - 5555 is the TCP port the bridge is listening on (MESH_PORT)
 **/

#include <iostream>
#include <string>
#include <memory>
#include <chrono>
#include <thread>

// Boost includes
#include <boost/asio/ip/address.hpp>

// Arduino emulation for PC
#include "Arduino.h"
#include "boost/asynctcp.hpp"

// painlessMesh includes
#include "painlessmesh/mesh.hpp"
#include "painlessmesh/gateway.hpp"

// Global objects required by Arduino emulation
WiFiClass WiFi;
ESPClass ESP;

using PMesh = painlessmesh::Mesh<painlessmesh::Connection>;
using namespace painlessmesh;

// Global logger
painlessmesh::logger::LogClass Log;

/**
 * PC Mesh Node Class
 * 
 * This class wraps a painlessMesh instance and provides methods
 * to connect to a bridge node and send Internet requests through it.
 */
class PCMeshNode : public PMesh {
public:
    PCMeshNode(Scheduler* scheduler, uint32_t nodeId, boost::asio::io_context& io)
        : io_service(io), scheduler_(scheduler) {
        this->nodeId = nodeId;
        this->init(scheduler, this->nodeId);
        
        // Start listening server (so bridge can connect back to us if needed)
        pServer = std::make_shared<AsyncServer>(io_service, this->nodeId);
        painlessmesh::tcp::initServer<painlessmesh::Connection, PMesh>(*pServer, (*this));
        
        std::cout << "✓ PC Mesh Node initialized with ID: " << this->nodeId << std::endl;
    }
    
    /**
     * Connect to a bridge node
     */
    bool connectToBridge(const std::string& bridgeIP, uint16_t bridgePort) {
        std::cout << "Connecting to bridge at " << bridgeIP << ":" << bridgePort << "..." << std::endl;
        
        try {
            auto pClient = new AsyncClient(io_service);
            
            // Set up connection callbacks
            bool connected = false;
            pClient->onConnect([&connected](void*, AsyncClient* client) {
                connected = true;
                std::cout << "✓ Connected to bridge!" << std::endl;
            });
            
            pClient->onDisconnect([](void*, AsyncClient* client) {
                std::cout << "✗ Disconnected from bridge" << std::endl;
            });
            
            // Connect to the bridge
            painlessmesh::tcp::connect<Connection, PMesh>(
                (*pClient), 
                boost::asio::ip::make_address(bridgeIP), 
                bridgePort,
                (*this)
            );
            
            // Wait for connection with timeout
            auto startTime = std::chrono::steady_clock::now();
            while (!connected) {
                this->update();
                io_service.poll();
                
                // Use longer sleep to reduce CPU usage during wait
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::steady_clock::now() - startTime
                ).count();
                
                if (elapsed > 30) {
                    std::cout << "✗ Connection timeout after 30 seconds" << std::endl;
                    return false;
                }
            }
            
            return true;
        } catch (const std::exception& e) {
            std::cout << "✗ Connection error: " << e.what() << std::endl;
            return false;
        }
    }
    
    /**
     * Send an HTTP request to the Internet via the bridge
     */
    void testSendToInternet(const std::string& url, const std::string& payload = "") {
        std::cout << "\n📡 Testing sendToInternet()..." << std::endl;
        std::cout << "   URL: " << url << std::endl;
        if (!payload.empty()) {
            std::cout << "   Payload: " << payload << std::endl;
        }
        
        // Check if we have Internet connection through bridge
        if (!this->hasInternetConnection()) {
            std::cout << "   ⚠️  No Internet connection available through bridge" << std::endl;
            std::cout << "   Make sure the bridge node has router access and is connected" << std::endl;
            return;
        }
        
        std::cout << "   ✓ Bridge with Internet found" << std::endl;
        
        // Send the request
        uint32_t msgId = this->sendToInternet(
            url,
            payload,
            [url](bool success, uint16_t httpStatus, std::string error) {
                std::cout << "\n📥 Response received:" << std::endl;
                std::cout << "   Success: " << (success ? "✓ YES" : "✗ NO") << std::endl;
                std::cout << "   HTTP Status: " << httpStatus << std::endl;
                if (!error.empty()) {
                    std::cout << "   Error: " << error << std::endl;
                }
                
                // Validate the result
                if (success && httpStatus == 200) {
                    std::cout << "   🎉 TEST PASSED: Request successful!" << std::endl;
                } else {
                    std::cout << "   ❌ TEST FAILED: Request unsuccessful" << std::endl;
                }
            }
        );
        
        if (msgId > 0) {
            std::cout << "   ✓ Request queued with message ID: " << msgId << std::endl;
            std::cout << "   Waiting for response from bridge..." << std::endl;
        } else {
            std::cout << "   ✗ Failed to queue request" << std::endl;
        }
    }
    
    /**
     * Main update loop
     */
    void runUpdateLoop(int durationSeconds = 0) {
        std::cout << "\n🔄 Starting update loop";
        if (durationSeconds > 0) {
            std::cout << " (running for " << durationSeconds << " seconds)";
        }
        std::cout << "..." << std::endl;
        
        auto startTime = std::chrono::steady_clock::now();
        
        while (true) {
            this->update();
            io_service.poll();
            scheduler_->execute();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            
            if (durationSeconds > 0) {
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::steady_clock::now() - startTime
                ).count();
                
                if (elapsed >= durationSeconds) {
                    std::cout << "✓ Update loop completed after " << elapsed << " seconds" << std::endl;
                    break;
                }
            }
        }
    }
    
    std::shared_ptr<AsyncServer> pServer;
    boost::asio::io_context& io_service;
    Scheduler* scheduler_;
};

/**
 * Print usage information
 */
void printUsage(const char* programName) {
    std::cout << "\nUsage: " << programName << " <bridge_ip> <bridge_port> [mock_server_ip] [mock_server_port]\n" << std::endl;
    std::cout << "Arguments:" << std::endl;
    std::cout << "  bridge_ip        IP address of ESP bridge node (e.g., 192.168.1.100)" << std::endl;
    std::cout << "  bridge_port      Mesh port of bridge (default: 5555)" << std::endl;
    std::cout << "  mock_server_ip   IP address of mock HTTP server (optional, default: localhost)" << std::endl;
    std::cout << "  mock_server_port Port of mock HTTP server (optional, default: 8080)" << std::endl;
    std::cout << "\nExample:" << std::endl;
    std::cout << "  " << programName << " 192.168.1.100 5555" << std::endl;
    std::cout << "  " << programName << " 192.168.1.100 5555 192.168.1.50 8080" << std::endl;
    std::cout << std::endl;
}

/**
 * Run automated test suite
 */
void runTests(PCMeshNode& node, const std::string& mockServerUrl) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "Starting Automated Test Suite" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    // Wait for mesh to stabilize
    std::cout << "\nWaiting 5 seconds for mesh to stabilize..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    // Test 1: HTTP 200 Success
    std::cout << "\n[Test 1/5] HTTP 200 Success" << std::endl;
    node.testSendToInternet(mockServerUrl + "/status/200");
    node.runUpdateLoop(5);
    
    // Test 2: HTTP 404 Not Found
    std::cout << "\n[Test 2/5] HTTP 404 Not Found" << std::endl;
    node.testSendToInternet(mockServerUrl + "/status/404");
    node.runUpdateLoop(5);
    
    // Test 3: HTTP 500 Server Error
    std::cout << "\n[Test 3/5] HTTP 500 Server Error" << std::endl;
    node.testSendToInternet(mockServerUrl + "/status/500");
    node.runUpdateLoop(5);
    
    // Test 4: WhatsApp API Simulation
    std::cout << "\n[Test 4/5] WhatsApp API Simulation" << std::endl;
    std::string whatsappUrl = mockServerUrl + "/whatsapp?phone=%2B1234567890&apikey=test&text=Hello%20from%20PC%20node";
    node.testSendToInternet(whatsappUrl);
    node.runUpdateLoop(5);
    
    // Test 5: Echo with JSON Payload
    std::cout << "\n[Test 5/5] Echo Endpoint with JSON Payload" << std::endl;
    // Using raw string literal for safer JSON construction
    // In production, consider using ArduinoJson for proper JSON handling
    char jsonBuffer[256];
    snprintf(jsonBuffer, sizeof(jsonBuffer), 
             R"({"source":"pc_node","test":"sendToInternet","timestamp":%lu})", 
             (unsigned long)millis());
    std::string payload(jsonBuffer);
    node.testSendToInternet(mockServerUrl + "/echo", payload);
    node.runUpdateLoop(5);
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "Test Suite Complete!" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
}

/**
 * Main entry point
 */
int main(int argc, char* argv[]) {
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "painlessMesh - PC Mesh Node Example" << std::endl;
    std::cout << "Testing sendToInternet() Through Bridge" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    // Parse command line arguments
    if (argc < 3) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string bridgeIP = argv[1];
    uint16_t bridgePort = std::stoi(argv[2]);
    std::string mockServerIP = (argc > 3) ? argv[3] : "127.0.0.1";
    uint16_t mockServerPort = (argc > 4) ? std::stoi(argv[4]) : 8080;
    
    std::cout << "\nConfiguration:" << std::endl;
    std::cout << "  Bridge:      " << bridgeIP << ":" << bridgePort << std::endl;
    std::cout << "  Mock Server: http://" << mockServerIP << ":" << mockServerPort << std::endl;
    std::cout << std::endl;
    
    // Setup logging
    Log.setLogLevel(painlessmesh::logger::ERROR | painlessmesh::logger::STARTUP | 
                   painlessmesh::logger::CONNECTION | painlessmesh::logger::COMMUNICATION);
    
    // Create IO context and scheduler
    boost::asio::io_context io_service;
    Scheduler scheduler;
    
    // Create PC mesh node with a random node ID
    uint32_t nodeId = 1000000 + (millis() % 1000000);
    PCMeshNode node(&scheduler, nodeId, io_service);
    
    // Enable sendToInternet API
    node.enableSendToInternet();
    std::cout << "✓ sendToInternet() API enabled" << std::endl;
    
    // Connect to bridge
    if (!node.connectToBridge(bridgeIP, bridgePort)) {
        std::cerr << "\n✗ Failed to connect to bridge" << std::endl;
        std::cerr << "Make sure:" << std::endl;
        std::cerr << "  1. Bridge node is running on " << bridgeIP << ":" << bridgePort << std::endl;
        std::cerr << "  2. Firewall allows TCP connections on port " << bridgePort << std::endl;
        std::cerr << "  3. PC and bridge are on the same network" << std::endl;
        return 1;
    }
    
    // Wait for mesh to establish
    std::cout << "\nWaiting for mesh to establish..." << std::endl;
    node.runUpdateLoop(10);
    
    // Build mock server URL
    std::string mockServerUrl = "http://" + mockServerIP + ":" + std::to_string(mockServerPort);
    
    // Run automated tests
    runTests(node, mockServerUrl);
    
    // Keep running for a bit to receive any late responses
    std::cout << "\nKeeping connection alive for 10 more seconds..." << std::endl;
    node.runUpdateLoop(10);
    
    std::cout << "\n✓ PC Mesh Node shutting down..." << std::endl;
    
    return 0;
}
