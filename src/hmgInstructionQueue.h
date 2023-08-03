//***********************************************************************
// HexMG Instruction Queue Header
// Creation date:  2021. 08. 02.
// Creator:        László Pohl
//***********************************************************************


//***********************************************************************
#ifndef HMG_INSTRUCTION_QUEUE_HEADER
#define HMG_INSTRUCTION_QUEUE_HEADER
//***********************************************************************


//***********************************************************************
#include <mutex>
#include <condition_variable>
#include "hmgException.h"
#include "hmgInstructionStream.h"
#include "hmgComponent.h"
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


//***********************************************************************
class InstructionQueue {
//***********************************************************************
    IsNothingInstruction listDummy;
    IsInstruction* pListLast; // addInstructionStream needs
    std::mutex listLockMutex;
    std::condition_variable waitingForIntructionCV;
    //***********************************************************************
    void processInstructions() {
    // Sends the instructions to the instruction processor of the global circuit
    //***********************************************************************
        try {
            bool isNotFinished = true;
            CircuitStorage& gc = CircuitStorage::getInstance();
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
                isNotFinished = gc.processInstructions(first);
            } while (isNotFinished);
        }
        catch (const hmgExcept& err) {
            std::cerr << err.what() << std::endl;
            throw;
        }
    }
public:
    //***********************************************************************
    InstructionQueue() : pListLast{ &listDummy } {}
    //***********************************************************************
    void addInstructionStream(InstructionStream& src) {
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
    static void waitToFinish(InstructionQueue& ip) { ip.processInstructions(); }
    //***********************************************************************
};


}


#endif

