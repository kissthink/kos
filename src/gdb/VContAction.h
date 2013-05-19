#ifndef VContAction_h_
#define VContAction_h_ 1

#include "mach/platform.h"      // for Reboot from KASSERT
#include "util/EmbeddedQueue.h"

namespace gdb {

/**
 * Represents a pending action for a thread.
 * actions: 'c','C','s','S'
 * Stores action from vCont command and used for
 * generating Stop Reply Packets.
 */
struct VContAction : public EmbeddedElement<VContAction> {
  char action[3];     // 'c','C sig', 's', 'S sig'
  int threadId;       // used for sanity check
  bool empty;         // true if initialized
  bool threadAssigned;// threadId is assigned
  bool executed;      // do not reply until action completes
  VContAction()
  : threadId(-1), empty(true), threadAssigned(false), executed(false) {
    action[0] = 0;    // 'op'
    action[1] = 0;    // 'signal'
    action[2] = 0;    // 'signal'
  }

  bool operator==(const VContAction& other) const {
    if (action[0] == other.action[0]
      && action[1] == other.action[1]
      && action[2] == other.action[2]
      && threadId == other.threadId
      && empty == other.empty
      && threadAssigned == other.threadAssigned
      && executed == other.executed) return true;

    return false;
  }

  friend std::ostream& operator<<(std::ostream &, const VContAction &);
};

class VContActionReply {
public:
  VContActionReply(int threadId, int signal)
  : threadId(threadId)
  , signal(signal) {}

  void serialize(char* buffer) const;

private:
  static const char hexchars[];
  int threadId;
  int signal;
};

} // namespace gdb

#endif // VContAction_h_
