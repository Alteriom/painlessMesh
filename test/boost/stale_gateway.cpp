#include <boost/asio/ip/address.hpp>
#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"

#include "Arduino.h"

#include "catch_utils.hpp"

#include "boost/asynctcp.hpp"

WiFiClass WiFi;
ESPClass ESP;

#include "painlessmesh/gateway.hpp"
#include "painlessmesh/mesh.hpp"

using PMesh = painlessmesh::Mesh<painlessmesh::Connection>;

using namespace painlessmesh;
painlessmesh::logger::LogClass Log;

/**
 * A gateway that stopped being one without saying so.
 *
 * Found on the Alteriom HIL rig while validating 2.0.3 (farm run
 * 34613286441): the failover row promotes a backup to bridge and then
 * reboots it as a regular node. A reboot announces nothing -- the graceful
 * `leaving` status is only sent by a bridge that steps down in-process -- so
 * every other node kept the ex-bridge in knownBridges for bridgeTimeoutMs
 * (60 s), and getPrimaryBridge() picked it over the live bridge on RSSI. A
 * regular node registers no GATEWAY_DATA handler, so it dropped the request
 * without a reply, and the sender reported "Request timed out" 30 s later.
 * The same happens after any crash, power loss or reflash of a bridge.
 *
 * These scenarios run three real meshes over loopback TCP: a sender, the
 * ex-bridge (a plain node the sender still believes is a gateway, with the
 * better RSSI), and a live bridge. The live bridge's GATEWAY_DATA handler is
 * a stand-in for wifi.hpp's, which this suite never compiles; it answers the
 * way the real one does on a 200.
 */

class MeshTest : public PMesh {
 public:
  MeshTest(Scheduler *scheduler, size_t id, boost::asio::io_context &io)
      : io_service(io) {
    this->nodeId = id;
    this->init(scheduler, this->nodeId);
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

  // Exposed so the live bridge can register its stand-in gateway handler.
  using plugin::PackageHandler<Connection>::callbackList;

  std::shared_ptr<AsyncServer> pServer;
  boost::asio::io_context &io_service;
};

struct Rig {
  // An enum: these are bound to const references (std::make_shared,
  // Catch2's REQUIRE), which in C++14 needs a definition a static constexpr
  // member does not have.
  enum : uint32_t { SENDER = 7101, EX_BRIDGE = 7102, BRIDGE = 7103 };

  Scheduler scheduler;
  boost::asio::io_context io;
  std::shared_ptr<MeshTest> sender, exBridge, bridge;
  int served = 0;

  Rig() {
    sender = std::make_shared<MeshTest>(&scheduler, SENDER, io);
    exBridge = std::make_shared<MeshTest>(&scheduler, EX_BRIDGE, io);
    bridge = std::make_shared<MeshTest>(&scheduler, BRIDGE, io);
    // A line: ex-bridge -- sender -- bridge. Both gateways are one hop away.
    exBridge->connect(*sender);
    bridge->connect(*sender);

    // The live bridge serves every request with a 200, as wifi.hpp's
    // handler does when the destination answers 200.
    auto live = bridge;
    int *count = &served;
    bridge->callbackList.onPackage(
        protocol::GATEWAY_DATA,
        [live, count](protocol::Variant &variant,
                      std::shared_ptr<Connection> ingress, uint32_t) {
          ++(*count);
          auto pkg = variant.to<gateway::GatewayDataPackage>();
          gateway::GatewayAckPackage ack;
          ack.from = live->getNodeId();
          ack.dest = pkg.originNode;
          ack.messageId = pkg.messageId;
          ack.originNode = pkg.originNode;
          ack.success = true;
          ack.httpStatus = 200;
          ack.timestamp = live->getNodeTime();
          auto conn = router::findRoute<Connection>((*live), pkg.originNode);
          if (!conn) conn = ingress;
          protocol::Variant reply(&ack);
          router::send(std::move(reply), conn);
          return true;
        });
  }

  // Pump until `done` or `ms` of wall clock have passed. The harness's
  // delay() is usleep(), so a round count says nothing about elapsed time.
  template <typename Done>
  void pumpFor(uint32_t ms, Done done) {
    const auto start = millis();
    while (!done() && millis() - start < ms) pump(1);
  }

  void pump(size_t rounds) {
    for (size_t i = 0; i < rounds; ++i) {
      sender->update();
      exBridge->update();
      bridge->update();
      io.poll();
      delay(1);
    }
  }

  bool converged() {
    return layout::size(sender->asNodeTree()) == 3 &&
           layout::size(exBridge->asNodeTree()) == 3 &&
           layout::size(bridge->asNodeTree()) == 3;
  }

  void stop() {
    sender->stop();
    exBridge->stop();
    bridge->stop();
  }
};

SCENARIO("A sender routed to a gateway that rebooted as a regular node recovers at once",
         "[gateway][internet][stale_gateway]") {
  delay(1000);
  Log.setLogLevel(logger::ERROR);
  Rig rig;

  rig.pumpFor(20000, [&] { return rig.converged(); });
  REQUIRE(rig.converged());

  GIVEN("A sender that still believes the ex-bridge is its best gateway") {
    rig.sender->enableSendToInternet();
    // Long enough that only a silent drop reaches it: the fix must answer
    // well inside this, the unfixed library waits it out.
    rig.sender->setInternetRequestTimeout(8000);
    rig.sender->setInternetRetryDelay(100);

    // The ex-bridge's last status, still inside bridgeTimeoutMs, with the
    // stronger router signal: getPrimaryBridge() prefers it, as on the rig.
    rig.sender->updateBridgeStatus(Rig::EX_BRIDGE, true, -28, 6, 1000,
                                   "10.42.0.1", rig.sender->getNodeTime());
    rig.sender->updateBridgeStatus(Rig::BRIDGE, true, -62, 6, 1000,
                                   "10.42.0.1", rig.sender->getNodeTime());
    REQUIRE(rig.sender->getPrimaryBridge() != nullptr);
    REQUIRE(rig.sender->getPrimaryBridge()->nodeId == Rig::EX_BRIDGE);

    WHEN("It sends to the Internet") {
      bool called = false, success = false;
      uint16_t status = 0;
      TSTRING error;
      const auto started = millis();
      auto msgId = rig.sender->sendToInternet(
          "https://api.example.com/data", "{}",
          [&](bool ok, uint16_t http, TSTRING err) {
            called = true;
            success = ok;
            status = http;
            error = err;
          });
      REQUIRE(msgId != 0);

      // 6 s: well past the 4 s the fix needs, short of the 8 s timeout.
      rig.pumpFor(6000, [&] { return called; });
      const auto elapsed = millis() - started;

      THEN("The request is delivered through the live bridge, not timed out") {
        INFO("callback: called=" << called << " success=" << success
                                 << " http=" << status << " error=" << error
                                 << " after " << elapsed << " ms");
        REQUIRE(called);
        REQUIRE(success);
        REQUIRE(status == 200);
        REQUIRE(rig.served == 1);
        // Far inside the 8 s request timeout: the ex-bridge answered, it did
        // not leave the sender to wait the timeout out.
        REQUIRE(elapsed < 4000);
      }

      AND_THEN("The sender no longer counts the ex-bridge as a gateway") {
        for (const auto &b : rig.sender->getBridges()) {
          REQUIRE(b.nodeId != Rig::EX_BRIDGE);
        }
        REQUIRE(rig.sender->getPrimaryBridge() != nullptr);
        REQUIRE(rig.sender->getPrimaryBridge()->nodeId == Rig::BRIDGE);
      }
    }
  }
  rig.stop();
}

/**
 * The rig's exact sequence (farm run 34621323832): the sender knew only the
 * ex-bridge when it sent, and the live bridge's first status reached it half
 * a second after the ex-bridge answered. A request already accepted must ride
 * the ordinary retry backoff until a gateway is known, not fail on the spot.
 */
SCENARIO("A sender that learns of the live bridge just after the ex-bridge answers delivers through it",
         "[gateway][internet][stale_gateway]") {
  delay(1000);
  Log.setLogLevel(logger::ERROR);
  Rig rig;

  rig.pumpFor(20000, [&] { return rig.converged(); });
  REQUIRE(rig.converged());

  GIVEN("A sender whose only known gateway is the ex-bridge") {
    rig.sender->enableSendToInternet();
    rig.sender->setInternetRequestTimeout(8000);
    rig.sender->setInternetRetryDelay(200);
    rig.sender->updateBridgeStatus(Rig::EX_BRIDGE, true, -28, 6, 1000,
                                   "10.42.0.1", rig.sender->getNodeTime());

    WHEN("It sends, and the live bridge is advertised only after the ex-bridge has answered") {
      bool called = false, success = false;
      uint16_t status = 0;
      TSTRING error;
      const auto started = millis();
      rig.sender->sendToInternet("https://api.example.com/data", "{}",
                                 [&](bool ok, uint16_t http, TSTRING err) {
                                   called = true;
                                   success = ok;
                                   status = http;
                                   error = err;
                                 });

      auto exBridgeForgotten = [&] {
        for (const auto &b : rig.sender->getBridges()) {
          if (b.nodeId == Rig::EX_BRIDGE) return false;
        }
        return true;
      };
      rig.pumpFor(3000, [&] { return called || exBridgeForgotten(); });
      REQUIRE(exBridgeForgotten());
      const bool failedBeforeLiveBridgeKnown = called;

      rig.sender->updateBridgeStatus(Rig::BRIDGE, true, -62, 6, 1000,
                                     "10.42.0.1", rig.sender->getNodeTime());
      rig.pumpFor(6000, [&] { return called; });
      const auto elapsed = millis() - started;

      THEN("The request is still pending when the live bridge appears, and is delivered through it") {
        INFO("called=" << called << " success=" << success << " http=" << status
                       << " error=" << error << " after " << elapsed << " ms");
        REQUIRE_FALSE(failedBeforeLiveBridgeKnown);
        REQUIRE(called);
        REQUIRE(success);
        REQUIRE(status == 200);
        REQUIRE(rig.served == 1);
        REQUIRE(elapsed < 5000);
      }
    }
  }
  rig.stop();
}

SCENARIO("A sender whose only gateway rebooted as a regular node is told so",
         "[gateway][internet][stale_gateway]") {
  delay(1000);
  Log.setLogLevel(logger::ERROR);
  Rig rig;

  rig.pumpFor(20000, [&] { return rig.converged(); });
  REQUIRE(rig.converged());

  GIVEN("A sender that knows the ex-bridge and no other gateway") {
    rig.sender->enableSendToInternet();
    rig.sender->setInternetRequestTimeout(8000);
    rig.sender->setInternetRetryDelay(100);
    rig.sender->updateBridgeStatus(Rig::EX_BRIDGE, true, -28, 6, 1000,
                                   "10.42.0.1", rig.sender->getNodeTime());

    WHEN("It sends to the Internet") {
      bool called = false, success = true;
      TSTRING error;
      const auto started = millis();
      rig.sender->sendToInternet("https://api.example.com/data", "{}",
                                 [&](bool ok, uint16_t, TSTRING err) {
                                   called = true;
                                   success = ok;
                                   error = err;
                                 });
      // 6 s: well past the 4 s the fix needs, short of the 8 s timeout.
      rig.pumpFor(6000, [&] { return called; });
      const auto elapsed = millis() - started;

      THEN("It fails once its retries find no gateway, naming the node that is not one") {
        INFO("error=" << error << " after " << elapsed << " ms");
        REQUIRE(called);
        REQUIRE(success == false);
        REQUIRE(error.find("not an Internet gateway") != TSTRING::npos);
        REQUIRE(error.find("timed out") == TSTRING::npos);
        REQUIRE(elapsed < 4000);
        REQUIRE(rig.served == 0);
      }
    }
  }
  rig.stop();
}
