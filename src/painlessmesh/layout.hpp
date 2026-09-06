#ifndef _PAINLESS_MESH_LAYOUT_HPP_
#define _PAINLESS_MESH_LAYOUT_HPP_

#include <list>
#include <memory>

#include "painlessmesh/protocol.hpp"

namespace painlessmesh {
namespace layout {

#include <memory>
/**
 * Whether the tree contains the given nodeId
 */
inline bool contains(protocol::NodeTree nodeTree, uint32_t nodeId) {
  if (nodeTree.nodeId == nodeId) {
    return true;
  }
  for (auto&& s : nodeTree.subs) {
    if (contains(s, nodeId)) return true;
  }
  return false;
}

/**
 * Remove the subtree rooted at nodeId from wherever it sits below tree.
 *
 * Only that node and what hangs under it go; the nodes on the way to it
 * stay. Used when a node turns up on a fresh direct connection while a
 * neighbour's tree still remembers its old place: a station has one
 * uplink, so the old place is stale, and routing to it would send packets
 * down a path that ends at a link that no longer exists.
 *
 * \return true if the node was found and removed.
 */
inline bool forget(protocol::NodeTree& tree, uint32_t nodeId) {
  for (auto it = tree.subs.begin(); it != tree.subs.end(); ++it) {
    if (it->nodeId == nodeId) {
      tree.subs.erase(it);
      return true;
    }
    if (forget(*it, nodeId)) return true;
  }
  return false;
}

/**
 * Remove from tree every node that appears anywhere in `elsewhere`.
 *
 * A node is in one place. When a neighbour's sync presents the nodes
 * below it, whatever another neighbour's cached tree still says about
 * those nodes is older, and a packet routed by the older copy goes down a
 * branch that ends at a link that no longer exists.
 *
 * \return how many nodes were removed.
 */
inline size_t forgetAll(protocol::NodeTree& tree,
                        const protocol::NodeTree& elsewhere) {
  size_t removed = forget(tree, elsewhere.nodeId) ? 1 : 0;
  for (auto&& s : elsewhere.subs) removed += forgetAll(tree, s);
  return removed;
}

/**
 * A short identity for what a tree says: which nodes, in which order, which
 * of them root. Never 0, so 0 can mean "nothing presented yet".
 *
 * A neighbour's sync is news only when this differs from its last one.
 */
inline uint32_t fingerprint(const protocol::NodeTree& tree,
                            uint32_t hash = 2166136261u) {
  auto mix = [&hash](uint32_t v) {
    for (int i = 0; i < 4; ++i) {
      hash ^= (v >> (8 * i)) & 0xff;
      hash *= 16777619u;
    }
  };
  mix(tree.nodeId);
  mix(tree.root ? 1u : 0u);
  for (auto&& s : tree.subs) hash = fingerprint(s, hash);
  mix(0xffffffffu);  // end of this node's subs
  return hash == 0 ? 1 : hash;
}

inline protocol::NodeTree excludeRoute(protocol::NodeTree&& tree,
                                       uint32_t exclude) {
  // Make sure to exclude any subs with nodeId == 0,
  // even if exlude is not set to zero
  tree.subs.remove_if([exclude](protocol::NodeTree s) {
    return s.nodeId == 0 || s.nodeId == exclude;
  });
  return tree;
}

template <class T>
class Layout {
 public:
  size_t stability = 0;
  std::list<std::shared_ptr<T> > subs;

  /** Return the nodeId of the node that we are running on.
   *
   * On the ESP hardware nodeId is uniquely calculated from the MAC address of
   * the node.
   */
  uint32_t getNodeId() const { return nodeId; }

  /**
   * Check whether this node is a root node.
   */
  bool isRoot() const { return root; }

  protocol::NodeTree asNodeTree() {
    auto nt = protocol::NodeTree(nodeId, root, hasTimeAuthority);
    for (auto&& s : subs) {
      if (s->nodeId == 0) continue;
      nt.subs.push_back(protocol::NodeTree(*s));
    }
    return nt;
  }

 protected:
  uint32_t nodeId = 0;
  bool root = false;
  bool hasTimeAuthority = false;
};

template <class T>
void syncLayout(Layout<T>& layout, uint32_t changedId) {
  // TODO: this should be called from changed connections and dropped
  // connections events
  for (auto&& sub : layout.subs) {
    if (sub->connected() && !sub->newConnection && sub->nodeId != 0 &&
        sub->nodeId != changedId) {  // Exclude current
      sub->nodeSyncTask.forceNextIteration();
    }
  }
  layout.stability /= 2;
}

class Neighbour : public protocol::NodeTree {
 public:
  // Inherit constructors
  using protocol::NodeTree::NodeTree;

  /**
   * fingerprint() of the tree this neighbour presented last, 0 before its
   * first sync. A sync that restates it carries nothing the cached tree
   * does not already reflect, however the cache has since been pruned.
   */
  uint32_t presented = 0;

  /**
   * Is the passed nodesync valid
   *
   * If not then the caller of this function should probably disconnect
   * this neighbour.
   */
  bool validSubs(protocol::NodeTree tree) {
    if (nodeId == 0)  // Cant really know, so valid as far as we know
      return true;
    if (nodeId != tree.nodeId) return false;
    for (auto&& s : tree.subs) {
      if (layout::contains(s, nodeId)) return false;
    }
    return true;
  }

  /**
   * Update subs with the new subs
   *
   * \param tree The possible new tree with this node as base
   *
   * Generally one probably wants to call validSubs before calling this
   * function.
   *
   * \return Whether we adopted the new tree
   */
  bool updateSubs(protocol::NodeTree tree) {
    if (nodeId == 0 || tree != (*this)) {
      nodeId = tree.nodeId;
      subs = tree.subs;
      root = tree.root;
      hasTimeAuthority = tree.hasTimeAuthority;
      return true;
    }
    return false;
  }

  /**
   * Create a request
   */
  protocol::NodeSyncRequest request(NodeTree&& layout) {
    auto subTree = excludeRoute(std::move(layout), nodeId);
    auto req = protocol::NodeSyncRequest(subTree.nodeId, nodeId, subTree.subs,
                                     subTree.root);
    req.hasTimeAuthority = subTree.hasTimeAuthority;
    return req;
  }

  /**
   * Create a reply
   */
  protocol::NodeSyncReply reply(NodeTree&& layout) {
    auto subTree = excludeRoute(std::move(layout), nodeId);
    return protocol::NodeSyncReply(subTree.nodeId, nodeId, subTree.subs,
                                   subTree.root);
  }
};

/**
 * The size of the mesh (the number of nodes)
 */
inline uint32_t size(protocol::NodeTree nodeTree) {
  auto no = 1;
  for (auto&& s : nodeTree.subs) {
    no += size(s);
  }
  return no;
}

/**
 * Whether the top node in the tree is also the root of the mesh
 */
inline bool isRoot(protocol::NodeTree nodeTree) {
  if (nodeTree.root) return true;
  return false;
}

/**
 * Whether any node in the tree is also root of the mesh
 */
inline bool isRooted(protocol::NodeTree nodeTree) {
  if (isRoot(nodeTree)) return true;
  for (auto&& s : nodeTree.subs) {
    if (isRooted(s)) return true;
  }
  return false;
}

/**
 * Return all nodes in a list container
 */
inline std::list<uint32_t> asList(protocol::NodeTree nodeTree,
                                  bool includeSelf = true) {
  std::list<uint32_t> lst;
  if (includeSelf) lst.push_back(nodeTree.nodeId);
  for (auto&& s : nodeTree.subs) {
    lst.splice(lst.end(), asList(s));
  }
  return lst;
}

}  // namespace layout
}  // namespace painlessmesh

#endif

