#ifndef _PAINLESS_MESH_ACK_HPP_
#define _PAINLESS_MESH_ACK_HPP_

#include <functional>
#include <list>
#include <map>
#include <set>

#include "painlessmesh/configuration.hpp"
#include "painlessmesh/logger.hpp"
#include "painlessmesh/plugin.hpp"

extern painlessmesh::logger::LogClass Log;

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

/**
 * AckTracker - tracks outgoing messages awaiting acknowledgment
 *
 * Time is passed in explicitly (millis() on device) so the tracker stays
 * platform independent and unit testable. All time comparisons are
 * uint32 wraparound safe.
 */
class AckTracker {
 public:
  /** Generate the next unique, non-zero message id */
  uint32_t nextMessageId() {
    if (++counter == 0) ++counter;
    return counter;
  }

  /**
   * Start tracking a message
   *
   * @param msgId Unique id from nextMessageId()
   * @param destinations Nodes expected to acknowledge
   * @param callback Fired once per destination (ack or timeout)
   * @param timeoutMs Time to wait for acknowledgments
   * @param now Current time (millis())
   */
  void track(uint32_t msgId, const std::list<uint32_t>& destinations,
             deliveryCallback_t callback, uint32_t timeoutMs, uint32_t now) {
    if (!callback || destinations.empty()) return;
    PendingAck entry;
    entry.callback = callback;
    entry.sentAt = now;
    entry.timeoutMs = timeoutMs;
    entry.waitingFor.insert(destinations.begin(), destinations.end());
    entries[msgId] = std::move(entry);
  }

  /**
   * Process an incoming acknowledgment
   *
   * Fires the callback with delivered = true and the measured latency.
   *
   * @return true when the ack matched a pending message/destination
   */
  bool handleAck(uint32_t msgId, uint32_t fromNode, uint32_t now) {
    auto it = entries.find(msgId);
    if (it == entries.end()) return false;
    if (it->second.waitingFor.erase(fromNode) == 0) return false;
    auto latency = (uint32_t)(now - it->second.sentAt);
    auto callback = it->second.callback;
    if (it->second.waitingFor.empty()) entries.erase(it);
    callback(fromNode, true, latency);
    return true;
  }

  /**
   * Fire timeout callbacks for expired messages
   *
   * @param now Current time (millis())
   * @return Number of messages still awaiting acknowledgment
   */
  size_t expire(uint32_t now) {
    for (auto it = entries.begin(); it != entries.end();) {
      if ((uint32_t)(now - it->second.sentAt) >= it->second.timeoutMs) {
        auto entry = std::move(it->second);
        it = entries.erase(it);
        for (auto&& nodeId : entry.waitingFor) {
          Log(logger::COMMUNICATION,
              "AckTracker: timeout waiting for ack from %u\n", nodeId);
          entry.callback(nodeId, false, entry.timeoutMs);
        }
      } else {
        ++it;
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
