#ifndef PAINLESS_MESH_ACK_HPP
#define PAINLESS_MESH_ACK_HPP

#include <functional>
#include <list>
#include <map>
#include <set>

#include "painlessmesh/configuration.hpp"
#include "painlessmesh/logger.hpp"
#include "painlessmesh/plugin.hpp"

extern painlessmesh::logger::LogClass Log;

// Maximum number of messages that may await acknowledgment at once.
// Bounded to protect the small ESP8266 heap; override at build time if
// your application legitimately needs more concurrent tracked sends.
#ifndef PAINLESSMESH_MAX_PENDING_ACKS
#define PAINLESSMESH_MAX_PENDING_ACKS 32
#endif

// Maximum number of delayed broadcast acknowledgments queued by a receiver.
// A single shared scheduler task drains the queue after the jitter delay.
#ifndef PAINLESSMESH_MAX_QUEUED_BROADCAST_ACKS
#define PAINLESSMESH_MAX_QUEUED_BROADCAST_ACKS 32
#endif

namespace painlessmesh {

/**
 * Per-message delivery confirmation (issue #379)
 *
 * When a delivery callback is passed to Mesh::sendSingle() or
 * Mesh::sendBroadcast(), the outgoing package is tagged with a unique
 * msgId. Receiving nodes automatically reply with a MessageAckPackage
 * (type 630) and the sender's AckTracker matches the reply to the pending
 * message, firing the callback with the measured round-trip latency.
 * Messages that are not acknowledged within their timeout fire the
 * callback with delivered = false.
 */
namespace ack {

/**
 * Callback type for delivery confirmation
 *
 * @param nodeId The destination node this result refers to
 * @param delivered True when the node acknowledged the message, false when
 *        the acknowledgment timed out
 * @param latencyMs Round-trip time in ms when delivered, otherwise the
 *        configured timeout
 */
typedef std::function<void(uint32_t nodeId, bool delivered, uint32_t latencyMs)>
    deliveryCallback_t;

/**
 * Acknowledgment package sent back to the original sender
 *
 * Routed as a SINGLE package so it traverses multiple hops using the
 * normal routing logic.
 *
 * Type ID: 630 (MESSAGE_ACK)
 */
class MessageAckPackage : public plugin::SinglePackage {
 public:
  /** ID of the message being acknowledged */
  uint32_t messageId = 0;

  MessageAckPackage() : SinglePackage(protocol::MESSAGE_ACK) {}

  MessageAckPackage(uint32_t fromNode, uint32_t destNode, uint32_t msgId)
      : SinglePackage(protocol::MESSAGE_ACK) {
    from = fromNode;
    dest = destNode;
    messageId = msgId;
  }

  MessageAckPackage(JsonObject jsonObj) : SinglePackage(jsonObj) {
    messageId = jsonObj["msgId"] | (uint32_t)0;
  }

  JsonObject addTo(JsonObject&& jsonObj) const {
    jsonObj = SinglePackage::addTo(std::move(jsonObj));
    jsonObj["msgId"] = messageId;
    return jsonObj;
  }

#if ARDUINOJSON_VERSION_MAJOR < 7
  size_t jsonObjectSize() const { return JSON_OBJECT_SIZE(noJsonFields + 1); }
#endif
};

/**
 * A message awaiting acknowledgment(s)
 */
struct PendingAck {
  deliveryCallback_t callback;
  uint32_t sentAt = 0;
  uint32_t timeoutMs = 0;
  std::set<uint32_t> waitingFor;
};

struct DeliveryResult {
  deliveryCallback_t callback;
  uint32_t nodeId = 0;
  bool delivered = false;
  uint32_t latencyMs = 0;

  DeliveryResult(deliveryCallback_t callback, uint32_t nodeId, bool delivered,
                 uint32_t latencyMs)
      : callback(callback),
        nodeId(nodeId),
        delivered(delivered),
        latencyMs(latencyMs) {}
};

/**
 * AckTracker - tracks outgoing messages awaiting acknowledgment
 *
 * Time is passed in explicitly (millis() on device) so the tracker stays
 * platform independent and unit testable. All time comparisons are
 * uint32 wraparound safe.
 */
class AckTracker {
 public:
  /**
   * Seed the message-id counter
   *
   * Called once at mesh init with a random value so ids do not restart
   * at 1 after a reboot — a delayed ACK for a pre-reboot message could
   * otherwise match a fresh message's id and report a false
   * delivered = true.
   */
  void seed(uint32_t value) { counter = value; }

  /** Generate the next unique, non-zero message id */
  uint32_t nextMessageId() {
    if (++counter == 0) ++counter;
    return counter;
  }

  /** Whether the tracker is at its pending-message limit */
  bool full() const { return entries.size() >= PAINLESSMESH_MAX_PENDING_ACKS; }

  /**
   * Start tracking a message
   *
   * @param msgId Unique id from nextMessageId()
   * @param destinations Nodes expected to acknowledge
   * @param callback Fired once per destination (ack or timeout)
   * @param timeoutMs Time to wait for acknowledgments
   * @param now Current time (millis())
   * @return false when the callback/destinations are empty or the
   *         tracker is full (PAINLESSMESH_MAX_PENDING_ACKS)
   */
  bool track(uint32_t msgId, const std::list<uint32_t>& destinations,
             deliveryCallback_t callback, uint32_t timeoutMs, uint32_t now) {
    if (!callback || destinations.empty()) return false;
    if (full()) {
      Log(logger::ERROR,
          "AckTracker: pending-ack limit (%u) reached, not tracking %u\n",
          (unsigned)PAINLESSMESH_MAX_PENDING_ACKS, msgId);
      return false;
    }
    PendingAck entry;
    entry.callback = callback;
    entry.sentAt = now;
    entry.timeoutMs = timeoutMs;
    entry.waitingFor.insert(destinations.begin(), destinations.end());
    entries[msgId] = std::move(entry);
    return true;
  }

  /**
   * Process an incoming acknowledgment
   *
   * Fires the callback with delivered = true and the measured latency.
   *
   * @return true when the ack matched a pending message/destination
   */
  bool handleAck(uint32_t msgId, uint32_t fromNode, uint32_t now) {
    std::list<DeliveryResult> results;
    auto matched = collectAck(msgId, fromNode, now, results);
    for (auto&& result : results) {
      result.callback(result.nodeId, result.delivered, result.latencyMs);
    }
    return matched;
  }

  /**
   * Match an acknowledgment without invoking user code.
   *
   * Mesh receive dispatch uses this form so callbacks can be scheduled after
   * the connection read task has unwound. Tracker state is still updated
   * synchronously and exactly once.
   */
  bool collectAck(uint32_t msgId, uint32_t fromNode, uint32_t now,
                  std::list<DeliveryResult>& results) {
    auto it = entries.find(msgId);
    if (it == entries.end()) return false;
    if (it->second.waitingFor.count(fromNode) == 0) return false;
    auto latency = (uint32_t)(now - it->second.sentAt);
    if (latency >= it->second.timeoutMs) {
      // The polling task may not have run exactly at the deadline. Reject
      // this late ACK and expire every destination still waiting on the
      // same message before invoking user code (callbacks may reenter us).
      auto expired = std::move(it->second);
      entries.erase(it);
      for (auto&& nodeId : expired.waitingFor) {
        Log(logger::COMMUNICATION,
            "AckTracker: timeout waiting for ack from %u\n", nodeId);
        results.push_back(
            DeliveryResult(expired.callback, nodeId, false, expired.timeoutMs));
      }
      return false;
    }
    it->second.waitingFor.erase(fromNode);
    auto callback = it->second.callback;
    if (it->second.waitingFor.empty()) entries.erase(it);
    results.push_back(DeliveryResult(callback, fromNode, true, latency));
    return true;
  }

  /**
   * Fire timeout callbacks for expired messages
   *
   * @param now Current time (millis())
   * @return Number of messages still awaiting acknowledgment
   */
  size_t expire(uint32_t now) {
    std::list<DeliveryResult> results;
    collectExpired(now, results);
    for (const auto& result : results) {
      result.callback(result.nodeId, result.delivered, result.latencyMs);
    }
    return entries.size();
  }

  /**
   * Collect timeout results without invoking user callbacks
   *
   * Mesh uses this form to defer callbacks until receive dispatch has
   * unwound. Direct AckTracker users retain the synchronous expire() API.
   */
  size_t collectExpired(uint32_t now, std::list<DeliveryResult>& results) {
    // Collect expired entries and erase them from the map BEFORE exposing
    // results to user callbacks: a callback may reenter this tracker (track a
    // retry, call clear() via mesh.stop(), or poll checkAcks()), which
    // would invalidate a live iterator into `entries`.
    std::list<PendingAck> expired;
    for (auto it = entries.begin(); it != entries.end();) {
      if ((uint32_t)(now - it->second.sentAt) >= it->second.timeoutMs) {
        expired.push_back(std::move(it->second));
        it = entries.erase(it);
      } else {
        ++it;
      }
    }
    for (auto&& entry : expired) {
      for (auto&& nodeId : entry.waitingFor) {
        Log(logger::COMMUNICATION,
            "AckTracker: timeout waiting for ack from %u\n", nodeId);
        results.push_back(
            DeliveryResult(entry.callback, nodeId, false, entry.timeoutMs));
      }
    }
    return entries.size();
  }

  /** Number of messages awaiting acknowledgment */
  size_t pending() const { return entries.size(); }

  /** Drop all pending messages without firing callbacks */
  void clear() { entries.clear(); }

 protected:
  std::map<uint32_t, PendingAck> entries;
  uint32_t counter = 0;
};

}  // namespace ack
}  // namespace painlessmesh

#endif
