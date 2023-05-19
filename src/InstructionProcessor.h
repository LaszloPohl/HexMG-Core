//***********************************************************************
// Hex Open Instruction Processor Header
// Creation date:  2021. 08. 02.
// Creator:        László Pohl
//***********************************************************************


//***********************************************************************
#ifndef HO_INSTRUCTION_PROCESSOR_HEADER
#define HO_INSTRUCTION_PROCESSOR_HEADER
//***********************************************************************


//***********************************************************************
#define _CRT_SECURE_NO_WARNINGS
#include <mutex>
#include <condition_variable>
#include "ho_exception.h"
#include "InstructionStream.h"
#include "CircuitModel.h"
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


//***********************************************************************
class InstructionProcessor {
//***********************************************************************
    IsNothingInstruction listDummy;
    IsInstruction* pListLast; // addInstructionStream needs
    std::mutex listLockMutex;
    ::std::condition_variable waitingForIntructionCV;
    GlobalCircuitModel globalCircuit;
    //***********************************************************************
    void processInstructions() {
    // Executes the instructions in the list one after the other.  
    // The execution of the next instruction always waits for the execution
    // of the previous instruction to finish.
    //***********************************************************************
        bool isNotFinished = true;
        do {
            {   std::unique_lock<std::mutex> uLock(listLockMutex);
                waitingForIntructionCV.wait(uLock, [this, isNotFinished] { return listDummy.next != nullptr || !isNotFinished; });
            } // uLock is destructed => listLockMutex is available
            listLockMutex.lock();
            IsInstruction *first;
            first = listDummy.next;
            listDummy.next = nullptr;
            pListLast = &listDummy;
            listLockMutex.unlock();
            isNotFinished = globalCircuit.processInstructions(first);
        } while (isNotFinished);
    }
public:
    //***********************************************************************
    InstructionProcessor() : pListLast{ &listDummy } {}
    //***********************************************************************
    ~InstructionProcessor() {  }
    //***********************************************************************
    void addInstructionStream(InstructionStream& src) {
    // all model definitions must be finished in src, except the global
    // circuit itself
    //***********************************************************************
        IsInstruction *first, *last;
        first = src.extractList(last);
        if (first != nullptr) {
            listLockMutex.lock();
            pListLast->next = first;
            pListLast = last;
            listLockMutex.unlock();
            waitingForIntructionCV.notify_one();
        }
    }
    //***********************************************************************
    static void waitToFinish(InstructionProcessor& ip) { ip.processInstructions(); }
    //***********************************************************************
};


}


#endif

