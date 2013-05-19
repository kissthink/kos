#include "gdb/VContAction.h"
#include <cstring>
#include <ostream>

using namespace gdb;
using namespace std;

const char VContActionReply::hexchars[] = "0123456789abcdef";

namespace gdb {
  ostream& operator<<(ostream& os, const VContAction& va) {
    os << "action: " << va.action
       << " threadId: " << va.threadId
       << " threadAssigned: " << va.threadAssigned
       << " executed: " << va.executed;
    return os;
  }
}

void VContActionReply::serialize(char* buffer) const {
  buffer[0] = 'T';
  buffer[1] = hexchars[signal >> 4];
  buffer[2] = hexchars[signal % 16];
  strcpy(&buffer[3], "thread:");
  buffer[10] = hexchars[threadId >> 4];
  buffer[11] = hexchars[threadId % 16];
  buffer[12] = 0;
}
