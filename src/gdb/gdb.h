#ifndef Gdb_h_
#define Gdb_h_ 1

#include "gdb/VContAction.h"

/**
 * TODO: Need to take care of a case where APIC ID is not 0,1,2,3,...
 */

class SpinLock;

namespace gdb {

class GdbCpuState;

struct CpuStates
{
    GdbCpuState* curCpuState;
    GdbCpuState* cCpuState;
    GdbCpuState* gCpuState;
    bool initialized;
    bool blocked;

    CpuStates()
    : curCpuState(0)
    , cCpuState(0)
    , gCpuState(0)
    , initialized(false)
    , blocked(true)
    {}
};

class GDB
{
public:
    /**
     * Singleton method
     */
    static GDB& getInstance();

    /**
     * initializes internal members.
     * Call this before using GDB.
     */
    void init(int numCpu);

    /**
     * Returns CpuState of the current core.
     */
    GdbCpuState* getCurrentCpuState() const;

    /**
     * Returns CpuState of the current core.
     */
    GdbCpuState* _getCurrentCpuState() const;

    /**
     * Returns CpuState of the asked core.
     */
    GdbCpuState* getCpuState(int cpuIdx);

    /**
     * Call me before using for AP.
     */
    void setupGDB(int cpuIdx);

    /**
     * Reset enumeration.
     */
    void startEnumerate();

    /**
     * Returns next CpuState or NULL.
     */
    GdbCpuState* next();

    /**
     * Returns a buffer storing 64-bit registers
     * for the current cpu.
     */
    char* getRegs64() const;

    /**
     * Returns a buffer storing 32-bit registers
     * for the current cpu.
     */
    char* getRegs32() const;

    /**
     * Returns CPU representation used by GDB.
     */
    const char* getCpuId(int cpuIdx);

    /**
     * Call me at BSP to start debugging.
     */
    void startGdb();

    /**
     * Returns number of cpus in the system.
     */
    int getNumCpus() const;

    /**
     * Returns number of initialized cpu.
     */
    int getNumCpusInitialized() const;

    /**
     * Sets CPU to use for G operations.
     */
    void setGCpu(int cpuIdx);

    /**
     * Get CpuState for G operations.
     */
    GdbCpuState* getGCpu() const;

    /**
     * Sets CPU to use for C operations.
     */
    void setCCpu(int cpuIdx);

    /**
     * Get CpuState for C operations.
     */
    GdbCpuState* getCCpu() const;

    /**
     * Sends IPI (INT 3) to all other cores.
     */
    void sendIPIToAllOtherCores() const;

    /**
     * P semaphore for the current cpu.
     */
    void P(int cpuIdx);

    /**
     * V semaphore for the current cpu.
     */
    void V(int cpuIdx);

    /**
     * Add a VContAction to a thread.
     */
    void addVContAction(VContAction* action, int cpuIdx);

    /**
     * Add a VContAction to the current thread.
     */
    void addVContAction(VContAction* action);

    /**
     * Checks if current thread's vContActionQueue
     * is empty.
     */
    bool isEmptyVContActionQueue() const;

    /**
     * Checks if a thread's VContActionQueue
     * is empty.
     */
    bool isEmptyVContActionQueue(int cpuIdx) const;

    /**
     * Returns vContAction for the current thread
     * from the front of the queue.
     */
    VContAction* nextVContAction();

    /**
     * Returns vContACtion for the current thread
     * from the front of the queue.
     */
    const VContAction* nextVContAction() const;

    /**
     * Remove vContAction from the front of the
     * queue of the current thread.
     */
    void popVContAction();

    /**
     * Sets stop reply for the current thread.
     * Fails if the reply is already set.
     */
    void setVContActionReply(int signal);

    /**
     * Consumes stop reply for the current thread.
     */
    VContActionReply* removeVContActionReply();

private:
    GDB();
    GdbCpuState* cpuStates;
    CpuStates* cpuStatesPtr;
    int numCpu;
    int numInitialized;
    int enumIdx;

    /**
     * Used as a part of split binary semaphore.
     */
//    Semaphore* sem;
    SpinLock* sem;

    /**
     * Stores pending vCont ops for each thread.
     */
    EmbeddedQueue<VContAction>* vContActionQueue;

    /**
     * Stores stop reply message for previous vCont ops.
     */
    VContActionReply* vContActionReply;
};

} // namespace gdb

#endif // Gdb_h_
