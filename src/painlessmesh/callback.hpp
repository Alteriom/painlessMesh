#ifndef _PAINLESS_MESH_CALLBACK_HPP_
#define _PAINLESS_MESH_CALLBACK_HPP_

#include <map>
#include <memory>

#include "painlessmesh/configuration.hpp"

#include "painlessmesh/protocol.hpp"

namespace painlessmesh {

/**
 * Helper functions to work with multiple callbacks
 */
namespace callback {
template <typename... Args>
class List {
 public:
  int execute(Args... args) {
    for (auto&& f : callbacks) {
      f(args...);
    }
    return callbacks.size();
  }

  size_t size() { return callbacks.size(); }

  void clear() { return callbacks.clear(); }

  /*
   * Needs to be wrapped into semaphore
   *
  int executeWithScheduler(Scheduler& scheduler, Args... args) {
    scheduler.execute();
    for (auto&& f : callbacks) {
      f(args...);
      scheduler.execute();
    }
    return callbacks.size();
  }*/

  /** Add callbacks to the end of the list.
   */
  void push_back(std::function<void(Args...)> func) {
    callbacks.push_back(func);
  }

 protected:
  std::list<std::function<void(Args...)>> callbacks;
};

/**
 * Manage callbacks for receiving packages
 */
template <typename... Args>
class PackageCallbackList {
 public:
  /**
   * Add a callback for specific package id
   */
  void onPackage(int id, std::function<void(Args...)> func) {
    if (clearPending) {
      (*pendingCallbackMap)[id].push_back(func);
    } else {
      (*callbackMap)[id].push_back(func);
    }
  }

  size_t size() {
    size_t size = 0;
    auto generation = clearPending ? pendingCallbackMap : callbackMap;
    for (auto&& key_value : *generation) {
      size += key_value.second.size();
    }
    return size;
  }

  void clear() {
    if (dispatchDepth > 0) {
      clearPending = true;
      pendingCallbackMap = std::make_shared<CallbackMap>();
      return;
    }
    callbackMap->clear();
  }

  /**
   * Execute all the callbacks associated with a certain package
   */
  int execute(int id, Args... args) {
    // Retain the selected generation for the complete call. A nested callback
    // may clear and replace the pending generation without destroying the
    // List/std::function currently executing on this stack.
    auto generation = clearPending ? pendingCallbackMap : callbackMap;
    ++dispatchDepth;
    auto result = (*generation)[id].execute(args...);
    --dispatchDepth;
    if (dispatchDepth == 0 && clearPending) {
      callbackMap = pendingCallbackMap;
      pendingCallbackMap.reset();
      clearPending = false;
    }
    return result;
  }

 protected:
  using CallbackMap = std::map<int, List<Args...>>;
  std::shared_ptr<CallbackMap> callbackMap = std::make_shared<CallbackMap>();
  // Registrations made after clear() during an active dispatch belong to
  // the next callback generation (for example stop(); init(); from a user
  // callback) and must survive removal of the currently executing one.
  std::shared_ptr<CallbackMap> pendingCallbackMap;
  size_t dispatchDepth = 0;
  bool clearPending = false;
};

template <typename T>
using MeshPackageCallbackList =
    PackageCallbackList<protocol::Variant&, std::shared_ptr<T>, uint32_t>;
}  // namespace callback
}  // namespace painlessmesh

#endif
