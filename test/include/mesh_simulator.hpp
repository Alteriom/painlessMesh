#ifndef MESH_SIMULATOR_HPP_
#define MESH_SIMULATOR_HPP_

/**
 * @file mesh_simulator.hpp
 * @brief Mesh network simulator for testing
 * 
 * This file contains the MeshTest and Nodes classes that were originally
 * in tcp_integration.cpp. These classes provide a Boost.Asio-based simulator
 * for testing mesh networks without physical hardware.
 * 
 * Originally created for tcp_integration tests, now extracted for reuse
 * across all example and integration tests.
 */

#include <boost/asio/ip/address.hpp>
#include <memory>
#include <vector>

#include "Arduino.h"
#include "catch_utils.hpp"
#include "boost/asynctcp.hpp"
#include "painlessmesh/mesh.hpp"

using PMesh = painlessmesh::Mesh<painlessmesh::Connection>;
using namespace painlessmesh;

/**
 * @brief Test mesh node for simulation
 * 
 * Extends PMesh with Boost.Asio networking for creating virtual mesh nodes
 * that can be tested without physical hardware.
 */
class MeshTest : public PMesh {
 public:
  MeshTest(Scheduler *scheduler, size_t id, boost::asio::io_context &io)
      : io_service(io) {
    this->nodeId = id;
    this->init(scheduler, this->nodeId);
    timeOffset = runif(0, 1e09);
    pServer = std::make_shared<AsyncServer>(io_service, this->nodeId);
    painlessmesh::tcp::initServer<painlessmesh::Connection, PMesh>(*pServer,
                                                                   (*this));
  }

  void connect(MeshTest &mesh) {
    auto pClient = new AsyncClient(io_service);
    painlessmesh::tcp::connect<Connection, PMesh>(
        (*pClient), boost::asio::ip::make_address("127.0.0.1"), mesh.nodeId,
        (*this));
  }

  std::shared_ptr<AsyncServer> pServer;
  boost::asio::io_context &io_service;
};

/**
 * @brief Collection of simulated mesh nodes
 * 
 * Manages multiple MeshTest nodes, automatically connecting them to form
 * a mesh network. Provides methods for updating all nodes and stopping
 * the simulation.
 */
class Nodes {
 public:
  Nodes(Scheduler *scheduler, size_t n, boost::asio::io_context &io)
      : io_service(io) {
    for (size_t i = 0; i < n; ++i) {
      auto m = std::make_shared<MeshTest>(scheduler, i + baseID, io_service);
      if (i > 0) m->connect((*nodes[runif(0, i - 1)]));
      nodes.push_back(m);
    }
  }
  
  void update() {
    for (auto &&m : nodes) {
      m->update();
      io_service.poll();
    }
  }

  void stop() {
    for (auto &&m : nodes) m->stop();
  }

  size_t size() const { return nodes.size(); }

  std::shared_ptr<MeshTest> get(size_t nodeId) {
    if (nodeId < baseID || (nodeId - baseID) >= nodes.size()) {
      return nullptr;
    }
    return nodes[nodeId - baseID];
  }

  size_t baseID = 6481;
  std::vector<std::shared_ptr<MeshTest>> nodes;
  boost::asio::io_context &io_service;
};

#endif  // MESH_SIMULATOR_HPP_
