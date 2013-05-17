#include "gdb.h"
#include "GdbCpu.h"
#include "util/Log.h"
#include "mach/platform.h"
#include "mach/CPU.h"
#include "mach/APIC.h"
#include "mach/Memory.h"
#include "mach/Processor.h"
#include "util/SpinLock.h"

using namespace gdb;

extern void set_debug_traps();      // from x86_64-stub.cc
extern void breakpoint();           // from x86_64-stub.cc

// Used as a mutex
// declared as non-member to call inside const member methods
SpinLock mutex;

GDB& GDB::getInstance()
{
    static GDB gdb;
    return gdb;
}

GDB::GDB()
: cpuStates(0)
, cpuStatesPtr(0)
, numCpu(0)
, numInitialized(0)
, enumIdx(0)
, sem(0)
, vContActionQueue(0)
, vContActionReply(0)
{}

void GDB::init(int numCpu)
{
    KASSERT(numCpu > 0, numCpu);

    this->numCpu = numCpu;
    cpuStates = new GdbCpuState[numCpu];
    cpuStatesPtr = new CpuStates[numCpu];
    sem = new SpinLock[numCpu];
    vContActionQueue = new EmbeddedQueue<VContAction>[numCpu];
    for (int i = 0; i < numCpu; i++) {
        cpuStates[i].setCpuId(i);
    }

    setupGDB(0);        // install CpuStates needed for GDB in BSP

    kcdbg << "GDB state initialized for " << numCpu << " cores.\n";
}

GdbCpuState* GDB::getCurrentCpuState() const
{
    ScopedLock<> so(mutex);
    return _getCurrentCpuState();
}

GdbCpuState* GDB::_getCurrentCpuState() const
{
    mword x;
    asm volatile("mov %%gs:0, %0" : "=r"(x));
    return reinterpret_cast<GdbCpuState *>(x);
}

GdbCpuState* GDB::getCpuState(int cpuIdx)
{
    ScopedLock<> so(mutex);

    KASSERT(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
    return &cpuStates[cpuIdx];
}

void GDB::setupGDB(int cpuIdx)
{
    ScopedLock<> so(mutex);

    KASSERT(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
    cpuStatesPtr[cpuIdx].curCpuState = &cpuStates[cpuIdx];
    cpuStatesPtr[cpuIdx].initialized = true;
    kcdbg << "setup cpu: " << cpuIdx+1 << "\n";
    MSR::write(MSR::GS_BASE, mword(&cpuStatesPtr[cpuIdx]));
    numInitialized = cpuIdx+1;
    sem[cpuIdx].acquire();      // simulate semaphore with initial count 0
    cpuStates[cpuIdx].setCpuState(cpuState::RUNNING);
}

void GDB::startEnumerate()
{
    ScopedLock<> so(mutex);
    enumIdx = 0;
}

GdbCpuState* GDB::next()
{
    ScopedLock<> so(mutex);

    if (enumIdx < numCpu) {
        return &cpuStates[enumIdx++];
    }

    return NULL;
}

char* GDB::getRegs64() const
{
    ScopedLock<> so(mutex);

    GdbCpuState* curCpuState = _getCurrentCpuState();
    return reinterpret_cast<char *>(curCpuState->getRegs64());
}

char* GDB::getRegs32() const
{
    ScopedLock<> so(mutex);

    GdbCpuState* curCpuState = _getCurrentCpuState();
    return reinterpret_cast<char *>(curCpuState->getRegs32());
}

const char* GDB::getCpuId(int cpuIdx)
{
    ScopedLock<> so(mutex);

    KASSERT(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
    return cpuStates[cpuIdx].getId();
}

void GDB::startGdb()
{
    KASSERT(numCpu > 0, numCpu);

    set_debug_traps();
    kcout << "Waiting for GDB connection\n";
    cpuStates[0].setCpuState(cpuState::RUNNING);
    breakpoint();
}

int GDB::getNumCpus() const
{
    ScopedLock<> so(mutex);
    return numCpu;
}

int GDB::getNumCpusInitialized() const
{
    ScopedLock<> so(mutex);
    return numInitialized;
}

void GDB::setGCpu(int cpuIdx)
{
    ScopedLock<> so(mutex);
    KASSERT(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
    asm volatile("mov %0, %%gs:16" :: "r"(&cpuStates[cpuIdx]) : "memory");
}

GdbCpuState* GDB::getGCpu() const
{
    ScopedLock<> so(mutex);

    mword x;
    asm volatile("mov %%gs:16, %0" : "=r"(x));
    if (x) {
        return reinterpret_cast<GdbCpuState *>(x);
    }

    return _getCurrentCpuState();
}

void GDB::setCCpu(int cpuIdx)
{
    ScopedLock<> so(mutex);

    KASSERT(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
    asm volatile("mov %0, %%gs:8" :: "r"(&cpuStates[cpuIdx]) : "memory");
}

GdbCpuState* GDB::getCCpu() const
{
    ScopedLock<> so(mutex);

    mword x;
    asm volatile("mov %%gs:8, %0" : "=r"(x));
    if (x) {
        return reinterpret_cast<GdbCpuState *>(x);
    }
    return _getCurrentCpuState();
}

/**
 * Send INT 3 IPI to all other cores
 * that are currently not in an interrupt handler
 */
void GDB::sendIPIToAllOtherCores() const
{
    int curCpuIdx = Processor::getApicID();
    for (int i = 0; i < numCpu; i++) {
        if (cpuStates[i].getCpuState() != cpuState::BREAKPOINT &&
                cpuStates[i].getCpuState() != cpuState::UNKNOWN) {
            LAPIC* lapic = (LAPIC *) lapicAddr;
            kcdbg << "sending ipi 0x3 from " << curCpuIdx+1 << " to core: " << i+1 << "\n";
            KASSERT(lapic != 0, "LAPIC is NULL");
            mword err = lapic->sendIPI(i, 0x3);
            KASSERT(err == 0, FmtHex(err));
        }
    }
}

void GDB::P(int cpuIdx)
{
    KASSERT(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
    KASSERT(cpuIdx < numInitialized, cpuIdx);
    sem[cpuIdx].acquire();
}

void GDB::V(int cpuIdx)
{
    KASSERT(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
    KASSERT(cpuIdx < numInitialized, cpuIdx);
    sem[cpuIdx].release();
}

void GDB::addVContAction(VContAction* action, int cpuIdx)
{
    ScopedLock<> so(mutex);

    KASSERT(action != 0, action);
    KASSERT(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
    KASSERT(cpuIdx < numInitialized, cpuIdx);
    vContActionQueue[cpuIdx].push(action);
}

void GDB::addVContAction(VContAction* action)
{
    ScopedLock<> so(mutex);

    KASSERT(action != 0, action);
    vContActionQueue[Processor::getApicID()].push(action);
}

bool GDB::isEmptyVContActionQueue() const
{
    ScopedLock<> so(mutex);
    return vContActionQueue[Processor::getApicID()].empty();
}

bool GDB::isEmptyVContActionQueue(int cpuIdx) const
{
    ScopedLock<> so(mutex);

    KASSERT(cpuIdx >= 0 && cpuIdx < numCpu, cpuIdx);
    KASSERT(cpuIdx < numInitialized, cpuIdx);
    return vContActionQueue[cpuIdx].empty();
}

VContAction* GDB::nextVContAction()
{
    ScopedLock<> so(mutex);
    return vContActionQueue[Processor::getApicID()].front();
}

const VContAction* GDB::nextVContAction() const
{
    ScopedLock<> so(mutex);
    return vContActionQueue[Processor::getApicID()].front();
}

void GDB::popVContAction()
{
    ScopedLock<> so(mutex);
    vContActionQueue[Processor::getApicID()].pop();
}

void GDB::setVContActionReply(int signal)
{
    ScopedLock<> so(mutex);

    KASSERT(vContActionReply == 0, "Pending vCont stop reply.");
    vContActionReply = new VContActionReply(
        Processor::getApicID(),
        signal
    );
}

VContActionReply* GDB::removeVContActionReply()
{
    ScopedLock<> so(mutex);

    KASSERT(vContActionReply, "vCont stop reply not set.");
    VContActionReply* reply = vContActionReply;
    vContActionReply = 0;
    return reply;
}
