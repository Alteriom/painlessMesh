#ifndef SIMULATOR_UTILS_HPP_
#define SIMULATOR_UTILS_HPP_

/**
 * @file simulator_utils.hpp
 * @brief Utilities for testing painlessMesh examples using the Boost.Asio simulator
 * 
 * This file provides reusable components for creating virtual mesh networks
 * that can test example sketches in a controlled, deterministic environment.
 */

#include <boost/asio/ip/address.hpp>
#include <memory>
#include <vector>
#include <functional>

#include "Arduino.h"
#include "catch_utils.hpp"
#include "boost/asynctcp.hpp"
#include "painlessmesh/mesh.hpp"

using PMesh = painlessmesh::Mesh<painlessmesh::Connection>;
using namespace painlessmesh;

/**
 * @brief A test mesh node that wraps painlessMesh for simulation
 * 
 * This class creates a virtual mesh node that can be used in tests.
 * It handles the Boost.Asio networking setup and provides helper methods
 * for common testing scenarios.
 */
class SimulatedMeshNode : public PMesh {
public:
    /**
     * @brief Construct a simulated mesh node
     * @param scheduler Pointer to task scheduler
     * @param nodeId Unique node ID
     * @param io Boost.Asio io_context for networking
     */
    SimulatedMeshNode(Scheduler *scheduler, size_t nodeId, boost::asio::io_context &io)
        : io_service(io), messagesReceived(0), connectionsChanged(0) {
        this->nodeId = nodeId;
        this->init(scheduler, this->nodeId);
        timeOffset = runif(0, 1e09);
        pServer = std::make_shared<AsyncServer>(io_service, this->nodeId);
        painlessmesh::tcp::initServer<painlessmesh::Connection, PMesh>(*pServer, (*this));
        
        // Setup default callbacks for tracking
        this->onReceive([this](uint32_t from, std::string msg) {
            this->messagesReceived++;
            this->lastMessageFrom = from;
            this->lastMessage = msg;
            if (this->userReceiveCallback) {
                this->userReceiveCallback(from, msg);
            }
        });
        
        this->onChangedConnections([this]() {
            this->connectionsChanged++;
            if (this->userConnectionCallback) {
                this->userConnectionCallback();
            }
        });
    }

    /**
     * @brief Connect this node to another node
     * @param otherNode The node to connect to
     */
    void connectTo(SimulatedMeshNode &otherNode) {
        auto pClient = new AsyncClient(io_service);
        painlessmesh::tcp::connect<Connection, PMesh>(
            (*pClient), boost::asio::ip::make_address("127.0.0.1"), 
            otherNode.nodeId, (*this));
    }

    /**
     * @brief Set a custom receive callback
     * @param callback Function to call when message received
     */
    void setReceiveCallback(std::function<void(uint32_t, std::string)> callback) {
        userReceiveCallback = callback;
    }

    /**
     * @brief Set a custom connection changed callback
     * @param callback Function to call when connections change
     */
    void setConnectionCallback(std::function<void()> callback) {
        userConnectionCallback = callback;
    }

    /**
     * @brief Get number of messages received
     */
    size_t getMessagesReceived() const { return messagesReceived; }

    /**
     * @brief Get the last message received
     */
    std::string getLastMessage() const { return lastMessage; }

    /**
     * @brief Get the sender of last message
     */
    uint32_t getLastMessageFrom() const { return lastMessageFrom; }

    /**
     * @brief Get number of times connections changed
     */
    size_t getConnectionsChanged() const { return connectionsChanged; }

    /**
     * @brief Reset message counters
     */
    void resetCounters() {
        messagesReceived = 0;
        connectionsChanged = 0;
        lastMessage = "";
        lastMessageFrom = 0;
    }

    std::shared_ptr<AsyncServer> pServer;
    boost::asio::io_context &io_service;

private:
    size_t messagesReceived;
    size_t connectionsChanged;
    std::string lastMessage;
    uint32_t lastMessageFrom;
    std::function<void(uint32_t, std::string)> userReceiveCallback;
    std::function<void()> userConnectionCallback;
};

/**
 * @brief A collection of simulated mesh nodes for testing
 * 
 * This class manages multiple mesh nodes and provides utilities for
 * testing mesh networks with multiple nodes.
 */
class SimulatedMeshNetwork {
public:
    /**
     * @brief Create a simulated mesh network
     * @param scheduler Pointer to task scheduler
     * @param numNodes Number of nodes to create
     * @param io Boost.Asio io_context for networking
     * @param baseId Base ID for node numbering
     */
    SimulatedMeshNetwork(Scheduler *scheduler, size_t numNodes, 
                        boost::asio::io_context &io, size_t baseId = 6481)
        : io_service(io), baseID(baseId) {
        for (size_t i = 0; i < numNodes; ++i) {
            auto node = std::make_shared<SimulatedMeshNode>(
                scheduler, i + baseID, io_service);
            // Connect to a random existing node to form mesh
            if (i > 0) {
                node->connectTo((*nodes[runif(0, i - 1)]));
            }
            nodes.push_back(node);
        }
    }

    /**
     * @brief Update all nodes in the network
     * 
     * This should be called in a loop to simulate mesh operation.
     * It updates each node and processes network events.
     */
    void update() {
        for (auto &&node : nodes) {
            node->update();
            io_service.poll();
        }
    }

    /**
     * @brief Stop all nodes
     */
    void stop() {
        for (auto &&node : nodes) {
            node->stop();
        }
    }

    /**
     * @brief Get number of nodes in network
     */
    size_t size() const { return nodes.size(); }

    /**
     * @brief Get a node by index (0-based)
     */
    std::shared_ptr<SimulatedMeshNode> operator[](size_t index) {
        return nodes[index];
    }

    /**
     * @brief Get a node by its node ID
     */
    std::shared_ptr<SimulatedMeshNode> getByNodeId(uint32_t nodeId) {
        return nodes[nodeId - baseID];
    }

    /**
     * @brief Run the simulation for a specified number of iterations
     * @param iterations Number of update cycles to run
     * @param delayMs Delay between updates in milliseconds
     */
    void runFor(size_t iterations, size_t delayMs = 10) {
        for (size_t i = 0; i < iterations; ++i) {
            update();
            delay(delayMs);
        }
    }

    /**
     * @brief Wait until all nodes see the full mesh
     * @param timeout Maximum iterations to wait
     * @return true if all nodes converged, false if timeout
     */
    bool waitForFullMesh(size_t timeout = 10000) {
        for (size_t i = 0; i < timeout; ++i) {
            update();
            delay(10);
            
            bool allConnected = true;
            for (auto &&node : nodes) {
                if (layout::size(node->asNodeTree()) != nodes.size()) {
                    allConnected = false;
                    break;
                }
            }
            
            if (allConnected) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Wait until a condition is met
     * @param condition Function returning true when condition is met
     * @param timeout Maximum iterations to wait
     * @return true if condition met, false if timeout
     */
    bool waitUntil(std::function<bool()> condition, size_t timeout = 10000) {
        for (size_t i = 0; i < timeout; ++i) {
            update();
            delay(10);
            
            if (condition()) {
                return true;
            }
        }
        return false;
    }

    size_t baseID;
    std::vector<std::shared_ptr<SimulatedMeshNode>> nodes;
    boost::asio::io_context &io_service;
};

#endif // SIMULATOR_UTILS_HPP_
