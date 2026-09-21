#!/bin/bash
#
# test/ci/check_member_init.py, on headers written to say exactly what it
# must and must not report.
#
# The checker is a line scanner, not a parser, so its promise is only as
# good as the cases pinned here. The first header is the #466 defect in its
# original shape -- a class with no constructor and a raw pointer member --
# beside every other shape found in src/ when the rule was introduced. The
# second is the same code with defaults, plus every construct that looked
# like an uninitialised member to an earlier draft of the scanner and is not
# one: a local variable in a member function, a function declaration, a
# comment, a string, a static, an enum class, a reference, an array, a
# multi-line default, a template.

set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
checker="$here/check_member_init.py"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

cat > "$work/bad.hpp" <<'EOF'
class AsyncServer;
namespace painlessmesh { namespace wifi {
class Mesh {
 public:
  void tcpServerInit() {
    if (_tcpListener != nullptr) { delete _tcpListener; }
    int local;  // a local, not a member: must not be reported
  }
 protected:
  uint8_t _meshChannel;                 // line 10
  AsyncServer* _tcpListener;            // line 11: the #466 defect
  std::shared_ptr<Task> bridgeStatusTask;
  struct BridgeCandidate {
    uint32_t nodeId;                    // line 14
    int8_t routerRSSI = 0;
  };
  uint32_t a = 0, b, *c = nullptr, *d;  // line 17: b and d
};
}}
struct time_sync_msg_t {
  int type = 0;
  uint32_t t0;                          // line 22
};
class Events {
  WiFiEventId_t eventScanDoneHandler;   // line 25: a known alias of size_t
};
EOF

cat > "$work/good.hpp" <<'EOF'
class AsyncServer;
template <class T>
class Mesh : public painlessmesh::Mesh<T> {
 public:
  Mesh() : _ref(*this) {}
  AsyncServer* listener();                 // a declaration, not a member
  void tcpServerInit() {
    int local;
    AsyncServer* fresh;
    if (_tcpListener != nullptr) { delete _tcpListener; }
    struct {
      int x;                               // deeper than the member depth
    } scratch;
    (void)scratch; (void)local; (void)fresh;
  }
  static const uint8_t MAX = 5;
  static constexpr int LIMIT = 3;
  enum class Role : uint8_t { NODE, BRIDGE };
  Role role = Role::NODE;
 protected:
  // AsyncServer* fakeInComment;
  const char* banner = "int fakeInString;";
  Mesh& _ref;
  char str[200];
  uint8_t _meshChannel = 1;
  AsyncServer* _tcpListener = nullptr;
  std::function<void(bool)> cb = [](bool) {
    int insideLambda;
    (void)insideLambda;
  };
  uint32_t a = 0, b = 1, *c = nullptr;
  int wide{0};
  struct Candidate {
    uint32_t nodeId = 0;
    int8_t routerRSSI = 0;
  };
  WiFiEventId_t eventSTAStartHandler = 0;
  // The limitation, pinned: an alias the checker does not know is not
  // seen. These two are uninitialised and must NOT be reported; if the
  // scanner ever learns to resolve aliases, move them to bad.hpp.
  typedef size_t unknown_id_t;
  unknown_id_t unseenArithmeticAlias;
  typedef int* IntPtr;
  IntPtr unseenPointerAlias;
};
EOF

echo "▸ the defect and its relatives are reported, one line each"
set +e
out="$(python3 "$checker" "$work/bad.hpp")"
status=$?
set -e
[ "$status" -eq 1 ] || { echo "expected exit 1 on bad.hpp, got $status"; echo "$out"; exit 1; }
for expect in ":10: Mesh::_meshChannel (arithmetic)" \
              ":11: Mesh::_tcpListener (pointer)" \
              ":14: BridgeCandidate::nodeId (arithmetic)" \
              ":17: Mesh::b (arithmetic)" \
              ":17: Mesh::d (pointer)" \
              ":22: time_sync_msg_t::t0 (arithmetic)" \
              ":25: Events::eventScanDoneHandler (arithmetic)"; do
  grep -qF -- "$expect" <<<"$out" || { echo "missing finding: $expect"; echo "$out"; exit 1; }
done
for absent in "local" "routerRSSI" "bridgeStatusTask" "Mesh::a " "Mesh::c " "type"; do
  ! grep -q -- "::${absent}" <<<"$out" || { echo "false positive: $absent"; echo "$out"; exit 1; }
done
[ "$(grep -c "has no default member initializer" <<<"$out")" -eq 7 ] || { echo "expected exactly 7 findings"; echo "$out"; exit 1; }

echo "▸ the same code with defaults, and everything that only looks like a member, passes"
python3 "$checker" "$work/good.hpp" >/dev/null || { echo "good.hpp was reported:"; python3 "$checker" "$work/good.hpp"; exit 1; }

echo "▸ a missing path is a usage error, not a pass"
set +e
python3 "$checker" "$work/nowhere" >/dev/null 2>&1
[ $? -eq 2 ] || { echo "expected exit 2 on a missing path"; exit 1; }
set -e

echo "OK: check_member_init reports the #466 shape and its relatives, and nothing that is not one"
