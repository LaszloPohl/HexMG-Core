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
#include "HximHeader.h"
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
    std::atomic<unsigned> counter = 0;
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
    void saveSFimCore(SimulationToSaveData* currentSave) {
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

        if constexpr (sizeof(rvt) != 8)
            throw hmgExcept("hmgSaver::saveSFimCore", "TODO: rvt size is not 8");

        if(!is_AC_type && currentSave->saveValuesDC.size()!=nProbe)
            throw hmgExcept("hmgSaver::saveSFimCore", "probe number for the FIM file should be %u, %u found", nProbe, (uns)currentSave->saveValuesDC.size());

        FILE* fp = nullptr;
        if (fopen_s(&fp, currentSave->fileName.c_str(), "wb") != 0)
            throw hmgExcept("hmgSaver::saveSFimCore", "cannot open file to write: %s", currentSave->fileName.c_str());

        fwrite(&head, sizeof(head), 1, fp);
        if (is_AC_type)
            fwrite(&currentSave->saveValuesAC[0], sizeof(cplx), nProbe, fp);
        else
            fwrite(&currentSave->saveValuesDC[0], sizeof(rvt), nProbe, fp);

        fclose(fp);
    }
    //***********************************************************************
    std::uint8_t char_to_hexadecimal(char ch) {
    //***********************************************************************
        if (ch >= '0' && ch <= '9')
            return (std::uint8_t)(ch - '0');
        else if (ch >= 'A' && ch <= 'F')
            return (std::uint8_t)(ch - 'A' + 10);
        else
            throw hmgExcept("hmgSaver::char_to_hexadecimal", "Not 0-9, A-F character (%c)", ch);
    }
    //***********************************************************************
    void saveHximCore(SimulationToSaveData* currentSave) {
    //***********************************************************************
        simulator::results::HximHeaderV1 head;

        head.flags = (std::uint8_t)currentSave->hflags; // Complex flag is set in CircuitStorage::save()
        head.stepOrdinal = (std::uint32_t)counter;

        switch (currentSave->analysisType) {
            case atDC:          
                head.analysisKind = 0; 
                head.analysisExtra1 = 0;
                break;
            case atTimeStep:    
                head.analysisKind = 1; 
                head.analysisExtra1 = currentSave->timeFreqValue;
                head.analysisExtra2 = currentSave->dtValue;
                break;
            case atAC:          
                head.analysisKind = 2; 
                head.analysisExtra1 = currentSave->timeFreqValue;
                break;
            case atTimeConst:   
                head.analysisKind = 3; 
                head.analysisExtra1 = currentSave->timeFreqValue;
                break;
        }

        for (uns i = 0; i < 16; i++) {
            head.gridId[i] = char_to_hexadecimal(currentSave->gridId[2 * i]) * 16 + char_to_hexadecimal(currentSave->gridId[2 * i + 1]);
        }

        for (uns i = 0; i < 16; i++) {
            head.runId[i] = char_to_hexadecimal(currentSave->runId[2 * i]) * 16 + char_to_hexadecimal(currentSave->runId[2 * i + 1]);
        }

        FILE* fp = nullptr;
        if (fopen_s(&fp, currentSave->fileName.c_str(), "wb") != 0)
            throw hmgExcept("hmgSaver::saveHximCore", "cannot open file to write: %s", currentSave->fileName.c_str());

        if constexpr (sizeof(rvt) != 8)
            throw hmgExcept("hmgSaver::saveHximCore", "TODO: rvt size is not 8");
        if constexpr (sizeof(cplx) != 16)
            throw hmgExcept("hmgSaver::saveHximCore", "TODO: cplx size is not 16");
        if constexpr (sizeof(float) != 4)
            throw hmgExcept("hmgSaver::saveHximCore", "TODO: float size is not 4");

        fwrite(&head, sizeof(head), 1, fp);
        if ((head.flags & hftComplex) != 0) {
            if ((head.flags & hftDouble) != 0)
                fwrite(&currentSave->saveValuesAC[0], sizeof(cplx), currentSave->saveValuesAC.size(), fp);
            else {
                std::vector<float> buff;
                buff.resize(currentSave->saveValuesAC.size() * 2);
                for (size_t i = 0; i < currentSave->saveValuesAC.size(); i++) {
                    cplx value = currentSave->saveValuesAC[i];
                    buff[2 * i]     = static_cast<float>(value.real());
                    buff[2 * i + 1] = static_cast<float>(value.imag());
                }
                fwrite(&buff[0], sizeof(float), buff.size(), fp);
            }
        }
        else {
            if ((head.flags & hftDouble) != 0)
                fwrite(&currentSave->saveValuesDC[0], sizeof(rvt), currentSave->saveValuesDC.size(), fp);
            else {
                std::vector<float> buff;
                buff.resize(currentSave->saveValuesDC.size());
                for (size_t i = 0; i < currentSave->saveValuesDC.size(); i++)
                    buff[i] = static_cast<float>(currentSave->saveValuesDC[i]);
                fwrite(&buff[0], sizeof(float), buff.size(), fp);
            }
        }

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
                    if (currentSave->isSFIM)
                        saveSFimCore(currentSave);
                    else if (currentSave->isHXIM) {
                        counter++;
                        saveHximCore(currentSave);
                    }
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

