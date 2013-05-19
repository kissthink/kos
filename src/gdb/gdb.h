#ifndef Gdb_h_
#define Gdb_h_ 1

#include "gdb/VContAction.h"

class SpinLock;

namespace gdb {

class GdbCpuState;
class Semaphore;

struct CpuStates {
  GdbCpuState* curCpuState;
  GdbCpuState* cCpuState;
  GdbCpuState* gCpuState;
  bool initialized;
  bool blocked;

  CpuStates() :
  curCpuState(0), cCpuState(0), gCpuState(0)
  , initialized(false)
  , blocked(true)
  {}
};

class GDB {
public:
  static GDB& getInstance();                // singleton method
  void init(int numCpu);                    // call before using GDB

  // returns CPU states
  GdbCpuState* getCurrentCpuState() const;
  GdbCpuState* _getCurrentCpuState() const;
  GdbCpuState* getCpuState(int cpuIdx);

  void setupGDB(int cpuIdx);                // setup GDB for APs

  // enumeration
  void startEnumerate();
  GdbCpuState* next();

  // access registers
  char* getRegs64() const;                  // returns a buffer storing 64-bit registers
  char* getRegs32() const;                  // a buffer storing 32-bit registers

  const char* getCpuId(int cpuIdx);         // returns CPU name used by GDB
  void startGdb();                          // starts initial breakpoint
  int getNumCpus() const;
  int getNumCpusInitialized() const;

  // CPU states for 'g', 'c' operations
  void setGCpu(int cpuIdx);
  GdbCpuState* getGCpu() const;
  void setCCpu(int cpuIdx);
  GdbCpuState* getCCpu() const;

  void sendIPIToAllOtherCores() const;        // sends INT 3 IPI to other available cores

  // semaphores to synchronize cores access to GDB interrupt handlers
  void P(int cpuIdx);
  void V(int cpuIdx);

  // vCont related
  void addVContAction(VContAction* action, int cpuIdx);
  void addVContAction(VContAction* action);
  bool isEmptyVContActionQueue() const;
  bool isEmptyVContActionQueue(int cpuIdx) const;
  VContAction* nextVContAction();
  const VContAction* nextVContAction() const;
  void popVContAction();
  void setVContActionReply(int signal);
  VContActionReply* removeVContActionReply();

private:
  GDB();
  GdbCpuState* cpuStates;
  CpuStates* cpuStatesPtr;
  int numCpu;
  int numInitialized;
  int enumIdx;
  Semaphore* sem;                                         // split binary semaphore
  EmbeddedQueue<VContAction>* vContActionQueue;           // pending vConts queued here
  VContActionReply* vContActionReply;
};

} // namespace gdb

#endif // Gdb_h_
