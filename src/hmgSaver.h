//***********************************************************************
// Hex Open Instruction Processor Header
// Creation date:  2023. 06. 13.
// Creator:        László Pohl
//***********************************************************************


//***********************************************************************
#ifndef HMG_SAVER_HEADER
#define HMG_SAVER_HEADER
//***********************************************************************


//***********************************************************************
#include <mutex>
#include <condition_variable>
#include "hmgException.h"
#include "hmgCommon.h"
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


//***********************************************************************
class hmgSaver {
//***********************************************************************
    SimulationToSaveData first;
    SimulationToSaveData* pListLast = &first;
    std::mutex listLockMutex;
    std::condition_variable waitingForSaveDataCV;
    bool canBeFinished = false;
    //***********************************************************************
    void saveTxtCore(SimulationToSaveData* currentSave) {
    //***********************************************************************
        FILE* fp = nullptr;
        if (fopen_s(&fp, currentSave->fileName.c_str(), currentSave->isAppend ? "at" : "wt") != 0)
            throw hmgExcept("hmgSaver::saveTxtCore", "cannot open file to write: %s", currentSave->fileName.c_str());

        if (!currentSave->isRaw) {
            fprintf_s(fp, "* Analysis type: ");
            bool isDC = false;
            bool isAC = false;
            bool isTC = false;
            switch (currentSave->analysisType) {
                case atDC: {
                        fprintf_s(fp, "DC\n");
                        isDC = true;
                    }
                    break;
                case atTimeStep: {
                        fprintf_s(fp, "Time Step\tt = %g sec\tdt = %g sec\n%g\t%g\t", currentSave->timeFreqValue, currentSave->dtValue, currentSave->timeFreqValue, currentSave->dtValue);
                        isDC = true;
                    }
                    break;
                case atAC: {
                        fprintf_s(fp, "AC\tf = %g Hz\n%g\t", currentSave->timeFreqValue, currentSave->timeFreqValue);
                        isAC = true;
                    }
                    break;
                case atTimeConst: {
                        if(currentSave->isTau)
                            fprintf_s(fp, "Time Constant\tTau = %g sec (f = %g Hz)\n%g\t%g\t", currentSave->timeFreqValue, rvt1 / (2 * hmgPi * currentSave->timeFreqValue), currentSave->timeFreqValue, rvt1 / (2 * hmgPi * currentSave->timeFreqValue));
                        else
                            fprintf_s(fp, "Time Constant\tf = %g Hz sec (Tau = %g)\n%g\t%g\t", currentSave->timeFreqValue, rvt1 / (2 * hmgPi * currentSave->timeFreqValue), currentSave->timeFreqValue, rvt1 / (2 * hmgPi * currentSave->timeFreqValue));
                        isTC = true;
                    }
                    break;
                default:
                    throw hmgExcept("hmgSaver::saveTxtCore", "unknown analysis Type: %u", (uns)currentSave->analysisType);
            }
            if (isDC)
                for (size_t i = 0; i < currentSave->saveValuesDC.size(); i++) {
                    fprintf_s(fp, "%g", currentSave->saveValuesDC[i]);
                    if (i == currentSave->saveValuesDC.size() - 1 || ((i + 1) % currentSave->maxResultsPerRow) == 0)
                        fprintf_s(fp, "\n");
                    else
                        fprintf_s(fp, "\t");
                }
            if(isAC)
                for (size_t i = 0; i < currentSave->saveValuesAC.size(); i++) {
                    fprintf_s(fp, "%g%+gi", currentSave->saveValuesAC[i].real(), currentSave->saveValuesAC[i].imag());
                    if (i == currentSave->saveValuesAC.size() - 1 || ((i + 1) % currentSave->maxResultsPerRow) == 0)
                        fprintf_s(fp, "\n");
                    else
                        fprintf_s(fp, "\t");
                }
            if (isTC)
                for (size_t i = 0; i < currentSave->saveValuesAC.size(); i++) {
                    fprintf_s(fp, "%g", (currentSave->saveValuesAC[i].imag() * log(10.0) / hmgPi));
                    if (i == currentSave->saveValuesAC.size() - 1 || ((i + 1) % currentSave->maxResultsPerRow) == 0)
                        fprintf_s(fp, "\n");
                    else
                        fprintf_s(fp, "\t");
                }
        }
        else {
            bool isDC = false;
            bool isAC = false;
            bool isTC = false;
            switch (currentSave->analysisType) {
                case atDC: 
                        isDC = true;
                    break;
                case atTimeStep: {
                        fprintf_s(fp, "%g\t%g\t", currentSave->timeFreqValue, currentSave->dtValue);
                        isDC = true;
                    }
                    break;
                case atAC: {
                        fprintf_s(fp, "%g\t", currentSave->timeFreqValue);
                        isAC = true;
                    }
                    break;
                case atTimeConst: {
                        fprintf_s(fp, "%g\t%g\t", currentSave->timeFreqValue, rvt1 / (2 * hmgPi * currentSave->timeFreqValue));
                        isTC = true;
                    }
                    break;
                default:
                    throw hmgExcept("hmgSaver::saveTxtCore", "unknown analysis Type: %u", (uns)currentSave->analysisType);
            }
            if (isDC)
                for (size_t i = 0; i < currentSave->saveValuesDC.size(); i++) {
                    fprintf_s(fp, "%g", currentSave->saveValuesDC[i]);
                    if (i == currentSave->saveValuesDC.size() - 1 || ((i + 1) % currentSave->maxResultsPerRow) == 0)
                        fprintf_s(fp, "\n");
                    else
                        fprintf_s(fp, "\t");
                }
            if (isAC)
                for (size_t i = 0; i < currentSave->saveValuesAC.size(); i++) {
                    fprintf_s(fp, "%g\t%g", currentSave->saveValuesAC[i].real(), currentSave->saveValuesAC[i].imag());
                    if (i == currentSave->saveValuesAC.size() - 1 || ((i + 1) % currentSave->maxResultsPerRow) == 0)
                        fprintf_s(fp, "\n");
                    else
                        fprintf_s(fp, "\t");
                }
            if (isTC)
                for (size_t i = 0; i < currentSave->saveValuesAC.size(); i++) {
                    fprintf_s(fp, "%g", (currentSave->saveValuesAC[i].imag() * log(10.0) / hmgPi));
                    if (i == currentSave->saveValuesAC.size() - 1 || ((i + 1) % currentSave->maxResultsPerRow) == 0)
                        fprintf_s(fp, "\n");
                    else
                        fprintf_s(fp, "\t");
                }
        }
        //fprintf_s(fp, "\n");
        fclose(fp);
    }
    //***********************************************************************
    void saveFimCore(SimulationToSaveData* currentSave) {
    //***********************************************************************
#pragma pack(push)
#pragma pack(8)
        struct resultheadtype{
            int kgrid = 9;         /* gridsize=2^kgrid       */
            int kresol = 6;        /* field size =2^kresol   */
            int nlay = 8;          /* # of the layers        */
            int analtype = 0;      /* analysis type 0...4 => 0: DC, 1: AC, 2: Bode, 3: Timeconst, 4: Timestep */
            int nstep = 0;         /* sequence No.           */
            double t = 0;          /* time value [s]         */
            double dt = 0;         /* arriving time-step     */
            double f = 0;          /* frequency [Hz]         */
            double angle = 0;      /* for timeconst [rad]    */
            int douim = 0;         /* 0/1=normal/double im   */
            int sizeofdata = 8;    /* double/complex (8/16)  */
        };
#pragma pack(pop)

        uns xy_n = 1 << currentSave->xy_k;
        uns z_n  = 1 << currentSave->z_k;
        uns nProbe = xy_n * xy_n * (z_n + z_n + 1);
        bool is_AC_type = false;

        resultheadtype head;
        head.kgrid = currentSave->xy_k + currentSave->z_k;
        head.kresol = currentSave->xy_k;
        head.nlay = z_n;
        switch (currentSave->analysisType) {
            case atDC:          head.analtype = 0; break;
            case atTimeStep:    head.analtype = 0; break;
            case atAC:          head.analtype = 1; is_AC_type = true; break;
            case atTimeConst:   head.analtype = 3; is_AC_type = true; break;
        }

        if (is_AC_type)
            head.sizeofdata = 16;

        if(sizeof(rvt) != 8)
            throw hmgExcept("hmgSaver::saveFimCore", "TODO: rvt size is not 8");

        if(!is_AC_type && currentSave->saveValuesDC.size()!=nProbe)
            throw hmgExcept("hmgSaver::saveFimCore", "probe number for the FIM file should be %u, %u found", nProbe, (uns)currentSave->saveValuesDC.size());

        FILE* fp = nullptr;
        if (fopen_s(&fp, currentSave->fileName.c_str(), "wb") != 0)
            throw hmgExcept("hmgSaver::saveFimCore", "cannot open file to write: %s", currentSave->fileName.c_str());

        fwrite(&head, sizeof(head), 1, fp);
        if (is_AC_type)
            fwrite(&currentSave->saveValuesAC[0], sizeof(cplx), nProbe, fp);
        else
            fwrite(&currentSave->saveValuesDC[0], sizeof(rvt), nProbe, fp);

        fclose(fp);
    }
    //***********************************************************************
    void save() {
    //***********************************************************************
        try {
            bool isFinished = false;
            do {
                {   
                    std::unique_lock<std::mutex> uLock(listLockMutex);
                    waitingForSaveDataCV.wait(uLock, [this, isFinished] { return first.next != nullptr || isFinished || canBeFinished; });
                } // uLock is destructed => listLockMutex is available

                listLockMutex.lock();

                SimulationToSaveData* currentSave = first.next;
            
                if (currentSave != nullptr) first.next = currentSave->next;
                else                        isFinished = canBeFinished;
            
                if(first.next == nullptr)   pListLast = &first;

                listLockMutex.unlock();

                if (currentSave != nullptr) {

                    if (currentSave->isFIM)
                        saveFimCore(currentSave);
                    else
                        saveTxtCore(currentSave);
                    delete currentSave;
                }

            } while (!isFinished);
        }
        catch (const hmgExcept& err) {
            std::cerr << err.what() << std::endl;
            throw;
        }
    }
public:
    //***********************************************************************
    void addSimulationToSaveData(SimulationToSaveData* src) {
    //***********************************************************************
        listLockMutex.lock();
        pListLast->next = src;
        pListLast = src;
        listLockMutex.unlock();
        waitingForSaveDataCV.notify_one();
    }
    //***********************************************************************
    void finish() noexcept {
    //***********************************************************************
        listLockMutex.lock();
        canBeFinished = true; 
        listLockMutex.unlock();
        waitingForSaveDataCV.notify_one();
    }
    //***********************************************************************
    static void waitToFinish(hmgSaver* ip) { ip->save(); }
    //***********************************************************************
};


}


#endif

