#ifndef VContAction_h_
#define VContAction_h_ 1

#include "util/EmbeddedQueue.h"
#include <cstring>
#include <ostream>

/**
 * Represents a pending action for a thread.
 * actions: 'c','C','s','S'
 * Stores action from vCont command and used for
 * generating Stop Reply Packets.
 */
struct VContAction : public EmbeddedElement<VContAction> {
  char action[3];     // 'c','C sig', 's', 'S sig'
  int threadId;       // used for sanity check
  bool executed;      // do not reply until action completes
  VContAction()
  : threadId(-1), executed(false) {
    action[0] = 0;    // 'op'
    action[1] = 0;    // 'signal'
    action[2] = 0;    // 'signal'
  }

  bool operator==(const VContAction& other) const {
    if (action[0] == other.action[0]
      && action[1] == other.action[1]
      && action[2] == other.action[2]
      && threadId == other.threadId
      && executed == other.executed) return true;
    return false;
  }
  bool isForCurrentThread() const {
    return (threadId == -1 || threadId == (int)Processor::getApicID()+1);
  }

  friend ostream& operator<<(ostream &, const VContAction &);
};

ostream& operator<<(ostream& os, const VContAction& va) {
  os << "action: " << va.action
     << " threadId: " << va.threadId
     << " executed: " << va.executed;
  return os;
}

#endif // VContAction_h_
