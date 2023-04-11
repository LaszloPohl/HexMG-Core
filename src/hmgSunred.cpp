//***********************************************************************
// HexMG Successive Network Reduction Engine CPP
// Creation date:  2023. 03. 17.
// Creator:        Pohl László
//***********************************************************************


//***********************************************************************
#include "hmgSunred.h"
#include "hmgComponent.h"
//***********************************************************************


//***********************************************************************
namespace nsHMG {
//***********************************************************************


//***********************************************************************
void SunredTreeNode::loadLeafDataFromSubcircuit(ComponentBase* src, ComponentSubCircuit* pSubckt) {
//***********************************************************************
    srcComponent = src;
    srcCell2 = srcCell1 = nullptr;
    CNodeIndex.clear();
    cuns ncSiz = (uns)srcComponent->def->nodesConnectedTo.size();
    for (uns i = 0; i < ncSiz; i++) {
        const auto& node = srcComponent->def->nodesConnectedTo[i];
        if (node.type == ComponentDefinition::CDNodeType::internal)
            CNodeIndex.push_back({ i, node.index });
        else if (node.type == ComponentDefinition::CDNodeType::external)
            CNodeIndex.push_back({ i, node.index | externalNodeFlag });
        // ground and unconnected nodes ignored
    }
    
    std::sort(CNodeIndex.begin(), CNodeIndex.end(), [](IndexPair a, IndexPair b) { return a.nodeIndex < b.nodeIndex; });

    ANodeIndex.clear();
    BNodeIndex.clear();
    isSymmDC = srcComponent->isJacobianMXSymmetrical(true);
    isSymmAC = srcComponent->isJacobianMXSymmetrical(false);
    
    // is common node?

    cuns siz = (uns)CNodeIndex.size();
    uns nCommon = 0;
    for (uns i = 0; i < siz - 1; i++)
        if (CNodeIndex[i].nodeIndex == CNodeIndex[i + 1].nodeIndex) { // ANodeIndex is sorted
            nCommon++;
        }
    if (nCommon != 0) {
        ANodeIndex.resize(siz - nCommon);
        BNodeIndex.resize(siz);
        BNodeIndex[0] = 0;
        ANodeIndex[0].nodeIndex = CNodeIndex[0].nodeIndex;
        nCommon = 0;
        for (uns i = 1; i < siz; i++) {
            if (CNodeIndex[i].nodeIndex != CNodeIndex[i - 1].nodeIndex) {
                BNodeIndex[i] = i - nCommon;
                ANodeIndex[i - nCommon].nodeIndex = CNodeIndex[i].nodeIndex;
            }
            else {
                BNodeIndex[i] = BNodeIndex[i - 1];
                nCommon++;
            }
        }
    }
    else {
        ANodeIndex.resize(CNodeIndex.size());
        for (uns i = 0; i < CNodeIndex.size(); i++)
            ANodeIndex[i].nodeIndex = CNodeIndex[i].nodeIndex;
    }

    // setting mergers

    for (auto& A : ANodeIndex) {
        cuns nodeIndex = A.nodeIndex & ~externalNodeFlag;
        cuns sumMergers = ((A.nodeIndex & externalNodeFlag) != 0) 
            ? (uns)(pSubckt->externalNodesToComponents[nodeIndex].size() - 1)
            : (uns)(pSubckt->internalNodesToComponents[nodeIndex].size() - 1);
        A.sumMergers = (ush)sumMergers;
        if (A.sumMergers != sumMergers)
            throw hmgExcept("SunredTreeNode::loadLeafDataFromSubcircuit", "Too many components connect to the same node (%u)", sumMergers);
        A.mergersSoFar = 0;
    }
 }


//***********************************************************************
void SunredTreeNode::loadNodeDataFromTwoNodes(SunredTreeNode* src1, SunredTreeNode* src2) {
//***********************************************************************
    srcComponent = nullptr;
    srcCell1 = src1;
    srcCell2 = src2;

    if ((src1->srcComponent == nullptr && src1->srcCell1 == nullptr) || (src2->srcComponent == nullptr && src2->srcCell1 == nullptr))
        throw hmgExcept("SunredTreeNode::loadNodeDataFromTwoNodes", "Disabled cell as source in sunred tree");

    ANodeIndex.clear();
    BNodeIndex.clear();
    CNodeIndex.clear();
    isSymmDC = src1->isSymmDC && src2->isSymmDC;
    isSymmAC = src1->isSymmAC && src2->isSymmAC;

    auto it1 = src1->ANodeIndex.cbegin();
    auto it2 = src2->ANodeIndex.cbegin();
    while (it1 != src1->ANodeIndex.cend() || it2 != src2->ANodeIndex.cend()) {
        if (it2 == src2->ANodeIndex.cend()) {
            if (it1->sumMergers == 0) // 1 component connects this node
                BNodeIndex.push_back(it1->nodeIndex);
            else
                ANodeIndex.push_back(*it1);
            it1++;
        }
        else if (it1 == src1->ANodeIndex.cend()) {
            if (it2->sumMergers == 0) // 1 component connects this node
                BNodeIndex.push_back(it2->nodeIndex);
            else
                ANodeIndex.push_back(*it2);
            it2++;
        }
        else {
            if (it1->nodeIndex == it2->nodeIndex) { // merge
                if (it1->mergersSoFar + it2->mergersSoFar + 1 == it1->sumMergers) // reduce
                    BNodeIndex.push_back(it1->nodeIndex);
                else // merge
                    ANodeIndex.push_back(Connections{ it1->nodeIndex, it1->sumMergers, (ush)(it1->mergersSoFar + it2->mergersSoFar + 1) });
                it1++;
                it2++;
            }
            else if (it1->nodeIndex < it2->nodeIndex) {
                if (it1->sumMergers == 0) // 1 component connects this node
                    BNodeIndex.push_back(it1->nodeIndex);
                else
                    ANodeIndex.push_back(*it1);
                it1++;
            }
            else { // it1->nodeIndex > it2->nodeIndex
                if (it2->sumMergers == 0) // 1 component connects this node
                    BNodeIndex.push_back(it2->nodeIndex);
                else
                    ANodeIndex.push_back(*it2);
                it2++;
            }
        }
    }
}


//***********************************************************************
void SunredTreeNode::forwsubsDC(ComponentSubCircuit* pSrc) {
//***********************************************************************
    cuns Asiz = (uns)ANodeIndex.size();
    cuns Bsiz = (uns)BNodeIndex.size();
    cuns Csiz = (uns)CNodeIndex.size();
    //***********************************************************************
    if (srcComponent != nullptr) { // leaf
    //***********************************************************************
        bool isChanged = false;
        if (Asiz == Csiz) { // only single nodes
            for (uns y = 0; y < Csiz; y++)
                for (uns x = isSymmDC ? y : 0; x < Csiz; x++) {
                    isChanged = dc->YRED.refresh_unsafe(y, x, srcComponent->getYDC(CNodeIndex[y].componentTerminalIndex, CNodeIndex[x].componentTerminalIndex)) || isChanged;
                }
            for (uns x = 0; x < Csiz; x++) {
                dc->JRED[x] = srcComponent->getJreducedDC(CNodeIndex[x].componentTerminalIndex);
            }
        }
        else { // there is common node
            
            // copy
            
            for (uns y = 0; y < Csiz; y++)
                for (uns x = isSymmDC ? y : 0; x < Csiz; x++) {
                    isChanged = dc->leaf->YA.refresh_unsafe(y, x, srcComponent->getYDC(CNodeIndex[y].componentTerminalIndex, CNodeIndex[x].componentTerminalIndex)) || isChanged;
                }
            for (uns x = 0; x < Csiz; x++) {
                dc->leaf->JA[x] = srcComponent->getJreducedDC(CNodeIndex[x].componentTerminalIndex);
            }
            
            // merge
            
            dc->YRED.zero_unsafe();
            dc->JRED.zero();
            for (uns y = 0; y < Bsiz; y++)
                for (uns x = isSymmDC ? y : 0; x < Bsiz; x++) {
                    dc->YRED.get_elem(BNodeIndex[y], BNodeIndex[x]) += dc->leaf->YA[y][x];
                }
            for (uns x = 0; x < Bsiz; x++) {
                dc->JRED[BNodeIndex[x]] += dc->leaf->JA[x];
            }
        }
        isChangedDC = isChanged;
    }
    //***********************************************************************
    else if (srcCell1 != nullptr) { // nonleaf
    //***********************************************************************

        cuns s1Asiz = (uns)srcCell1->ANodeIndex.size();
        const auto& ANodeIndex1 = srcCell1->ANodeIndex;
        cuns s2Asiz = (uns)srcCell2->ANodeIndex.size();
        const auto& ANodeIndex2 = srcCell2->ANodeIndex;

        //***********************************************************************
        // sorting of admittances
        //***********************************************************************

        if (srcCell1->isChangedDC || srcCell2->isChangedDC) {

            isChangedDC = true;
            dc->YRED.zero_unsafe();
            dc->calc->XB.zero_unsafe();
            dc->calc->YB_NZB.zero_unsafe();

            if (isSymmDC) { // src1->YRED and src2->YRED => YRED (as YA) and XB and YB_NZB (XAT is not used)
                
                // src1 => this

                uns dAy = 0, dBy = 0;
                for (uns y = 0; y < s1Asiz; y++) {

                    // jump

                    while (dAy < Asiz && ANodeIndex[dAy].nodeIndex < ANodeIndex1[y].nodeIndex)
                        dAy++;
                    while (dBy < Bsiz && BNodeIndex[dBy] < ANodeIndex1[y].nodeIndex)
                        dBy++;

                    // merge

                    if (dAy < Asiz && ANodeIndex1[y].nodeIndex == ANodeIndex[dAy].nodeIndex) {
                        uns dAx = dAy, dBx = dBy;
                        for (uns x = y; x < s1Asiz; x++) {

                            // jump

                            while (dAx < Asiz && ANodeIndex[dAx].nodeIndex < ANodeIndex1[x].nodeIndex)
                                dAx++;
                            while (dBx < Bsiz && BNodeIndex[dBx] < ANodeIndex1[x].nodeIndex)
                                dBx++;

                            // merge

                            if (dAx < Asiz && ANodeIndex1[x].nodeIndex == ANodeIndex[dAx].nodeIndex) { // => YA
                                dc->YRED[dAy][dAx] += srcCell1->dc->YRED[y][x];
                                dAx++;
                            }
                            else if (ANodeIndex1[x].nodeIndex == BNodeIndex[dBx]) { // => XB
                                dc->calc->XB[dAy][dBx] += srcCell1->dc->YRED[y][x];
                                dBx++;
                            }
                            else
                                throw hmgExcept("SunredTreeNode::forwsubsDC", "nodeIndex not found / 1");
                        }
                        dAy++;
                    }
                    else if (ANodeIndex1[y].nodeIndex == BNodeIndex[dBy]) {
                        uns dAx = dAy, dBx = dBy;
                        for (uns x = y; x < s1Asiz; x++) {

                            // jump

                            while (dAx < Asiz && ANodeIndex[dAx].nodeIndex < ANodeIndex1[x].nodeIndex)
                                dAx++;
                            while (dBx < Bsiz && BNodeIndex[dBx] < ANodeIndex1[x].nodeIndex)
                                dBx++;

                            // merge

                            if (dAx < Asiz && ANodeIndex1[x].nodeIndex == ANodeIndex[dAx].nodeIndex) { // => XA
                                dc->calc->XB[dAx][dBy] += srcCell1->dc->YRED[y][x]; // XB = XAT
                                dAx++;
                            }
                            else if (ANodeIndex1[x].nodeIndex == BNodeIndex[dBx]) { // => YB
                                dc->calc->YB_NZB[dBy][dBx] += srcCell1->dc->YRED[y][x];
                                dBx++;
                            }
                            else
                                throw hmgExcept("SunredTreeNode::forwsubsDC", "nodeIndex not found / 2");
                        }
                        dBy++;
                    }
                    else
                        throw hmgExcept("SunredTreeNode::forwsubsDC", "nodeIndex not found / 3");
                }

                // src2 => this

                dAy = 0;
                dBy = 0;
                for (uns y = 0; y < s2Asiz; y++) {

                    // jump

                    while (dAy < Asiz && ANodeIndex[dAy].nodeIndex < ANodeIndex2[y].nodeIndex)
                        dAy++;
                    while (dBy < Bsiz && BNodeIndex[dBy] < ANodeIndex2[y].nodeIndex)
                        dBy++;

                    // merge

                    if (dAy < Asiz && ANodeIndex2[y].nodeIndex == ANodeIndex[dAy].nodeIndex) {
                        uns dAx = dAy, dBx = dBy;
                        for (uns x = y; x < s2Asiz; x++) {

                            // jump

                            while (dAx < Asiz && ANodeIndex[dAx].nodeIndex < ANodeIndex2[x].nodeIndex)
                                dAx++;
                            while (dBx < Bsiz && BNodeIndex[dBx] < ANodeIndex2[x].nodeIndex)
                                dBx++;

                            // merge

                            if (dAx < Asiz && ANodeIndex2[x].nodeIndex == ANodeIndex[dAx].nodeIndex) { // => YA
                                dc->YRED[dAy][dAx] += srcCell2->dc->YRED[y][x];
                                dAx++;
                            }
                            else if (ANodeIndex2[x].nodeIndex == BNodeIndex[dBx]) { // => XB
                                dc->calc->XB[dAy][dBx] += srcCell2->dc->YRED[y][x];
                                dBx++;
                            }
                            else
                                throw hmgExcept("SunredTreeNode::forwsubsDC", "nodeIndex not found / 4");
                        }
                        dAy++;
                    }
                    else if (ANodeIndex2[y].nodeIndex == BNodeIndex[dBy]) {
                        uns dAx = dAy, dBx = dBy;
                        for (uns x = y; x < s2Asiz; x++) {

                            // jump

                            while (dAx < Asiz && ANodeIndex[dAx].nodeIndex < ANodeIndex2[x].nodeIndex)
                                dAx++;
                            while (dBx < Bsiz && BNodeIndex[dBx] < ANodeIndex2[x].nodeIndex)
                                dBx++;

                            // merge

                            if (dAx < Asiz && ANodeIndex2[x].nodeIndex == ANodeIndex[dAx].nodeIndex) { // => XA
                                dc->calc->XB[dAx][dBy] += srcCell2->dc->YRED[y][x]; // XB = XAT
                                dAx++;
                            }
                            else if (ANodeIndex2[x].nodeIndex == BNodeIndex[dBx]) { // => YB
                                dc->calc->YB_NZB[dBy][dBx] += srcCell2->dc->YRED[y][x];
                                dBx++;
                            }
                            else
                                throw hmgExcept("SunredTreeNode::forwsubsDC", "nodeIndex not found / 5");
                        }
                        dBy++;
                    }
                    else
                        throw hmgExcept("SunredTreeNode::forwsubsDC", "nodeIndex not found / 6");
                }

                if (Bsiz != 1)
                    dc->calc->YB_NZB.symmetrize_from_upper();
            }
            else { // result is nonsymm (one of the inputs can be); src1->YRED and src2->YRED => YRED (as YA) and XAT and XB and YB_NZB

                dc->calc->XAT.zero_unsafe();

                // src1 => this

                uns dAy = 0, dBy = 0;
                for (uns y = 0; y < s1Asiz; y++) {

                    // jump

                    while (dAy < Asiz && ANodeIndex[dAy].nodeIndex < ANodeIndex1[y].nodeIndex)
                        dAy++;
                    while (dBy < Bsiz && BNodeIndex[dBy] < ANodeIndex1[y].nodeIndex)
                        dBy++;

                    // merge

                    if (dAy < Asiz && ANodeIndex1[y].nodeIndex == ANodeIndex[dAy].nodeIndex) {
                        uns dAx = 0, dBx = 0;
                        for (uns x = 0; x < s1Asiz; x++) {

                            // jump

                            while (dAx < Asiz && ANodeIndex[dAx].nodeIndex < ANodeIndex1[x].nodeIndex)
                                dAx++;
                            while (dBx < Bsiz && BNodeIndex[dBx] < ANodeIndex1[x].nodeIndex)
                                dBx++;

                            // merge

                            if (dAx < Asiz && ANodeIndex1[x].nodeIndex == ANodeIndex[dAx].nodeIndex) { // => YA
                                dc->YRED[dAy][dAx] += srcCell1->dc->YRED.get_elem(y, x);
                                dAx++;
                            }
                            else if (ANodeIndex1[x].nodeIndex == BNodeIndex[dBx]) { // => XB
                                dc->calc->XB[dAy][dBx] += srcCell1->dc->YRED.get_elem(y, x);
                                dBx++;
                            }
                            else
                                throw hmgExcept("SunredTreeNode::forwsubsDC", "nodeIndex not found / 7");
                        }
                        dAy++;
                    }
                    else if (ANodeIndex1[y].nodeIndex == BNodeIndex[dBy]) {
                        uns dAx = 0, dBx = 0;
                        for (uns x = 0; x < s1Asiz; x++) {

                            // jump

                            while (dAx < Asiz && ANodeIndex[dAx].nodeIndex < ANodeIndex1[x].nodeIndex)
                                dAx++;
                            while (dBx < Bsiz && BNodeIndex[dBx] < ANodeIndex1[x].nodeIndex)
                                dBx++;

                            // merge

                            if (dAx < Asiz && ANodeIndex1[x].nodeIndex == ANodeIndex[dAx].nodeIndex) { // => XA
                                dc->calc->XAT[dAx][dBy] += srcCell1->dc->YRED.get_elem(y, x);
                                dAx++;
                            }
                            else if (ANodeIndex1[x].nodeIndex == BNodeIndex[dBx]) { // => YB
                                dc->calc->YB_NZB[dBy][dBx] += srcCell1->dc->YRED.get_elem(y, x);
                                dBx++;
                            }
                            else
                                throw hmgExcept("SunredTreeNode::forwsubsDC", "nodeIndex not found / 8");
                        }
                        dBy++;
                    }
                    else
                        throw hmgExcept("SunredTreeNode::forwsubsDC", "nodeIndex not found / 9");
                }

                // src2 => this

                dAy = 0;
                dBy = 0;
                for (uns y = 0; y < s2Asiz; y++) {

                    // jump

                    while (dAy < Asiz && ANodeIndex[dAy].nodeIndex < ANodeIndex2[y].nodeIndex)
                        dAy++;
                    while (dBy < Bsiz && BNodeIndex[dBy] < ANodeIndex2[y].nodeIndex)
                        dBy++;

                    // merge

                    if (dAy < Asiz && ANodeIndex2[y].nodeIndex == ANodeIndex[dAy].nodeIndex) {
                        uns dAx = 0, dBx = 0;
                        for (uns x = 0; x < s2Asiz; x++) {

                            // jump

                            while (dAx < Asiz && ANodeIndex[dAx].nodeIndex < ANodeIndex2[x].nodeIndex)
                                dAx++;
                            while (dBx < Bsiz && BNodeIndex[dBx] < ANodeIndex2[x].nodeIndex)
                                dBx++;

                            // merge

                            if (dAx < Asiz && ANodeIndex2[x].nodeIndex == ANodeIndex[dAx].nodeIndex) { // => YA
                                dc->YRED[dAy][dAx] += srcCell2->dc->YRED.get_elem(y, x);
                                dAx++;
                            }
                            else if (ANodeIndex2[x].nodeIndex == BNodeIndex[dBx]) { // => XB
                                dc->calc->XB[dAy][dBx] += srcCell2->dc->YRED.get_elem(y, x);
                                dBx++;
                            }
                            else
                                throw hmgExcept("SunredTreeNode::forwsubsDC", "nodeIndex not found / 10");
                        }
                        dAy++;
                    }
                    else if (ANodeIndex2[y].nodeIndex == BNodeIndex[dBy]) {
                        uns dAx = 0, dBx = 0;
                        for (uns x = 0; x < s2Asiz; x++) {

                            // jump

                            while (dAx < Asiz && ANodeIndex[dAx].nodeIndex < ANodeIndex2[x].nodeIndex)
                                dAx++;
                            while (dBx < Bsiz && BNodeIndex[dBx] < ANodeIndex2[x].nodeIndex)
                                dBx++;

                            // merge

                            if (dAx < Asiz && ANodeIndex2[x].nodeIndex == ANodeIndex[dAx].nodeIndex) { // => XA
                                dc->calc->XAT[dAx][dBy] += srcCell2->dc->YRED.get_elem(y, x);
                                dAx++;
                            }
                            else if (ANodeIndex2[x].nodeIndex == BNodeIndex[dBx]) { // => YB
                                dc->calc->YB_NZB[dBy][dBx] += srcCell2->dc->YRED.get_elem(y, x);
                                dBx++;
                            }
                            else
                                throw hmgExcept("SunredTreeNode::forwsubsDC", "nodeIndex not found / 11");
                        }
                        dBy++;
                    }
                    else
                        throw hmgExcept("SunredTreeNode::forwsubsDC", "nodeIndex not found / 12");
                }
            }

        }
        else {
            isChangedDC = false;
        }

        //***********************************************************************
        // sorting of defect 1: reading common nodes defect
        //***********************************************************************

        dc->calc->JAUA.zero();

        for (uns i = 0; i < Bsiz; i++)
            dc->calc->JBUB[i] = (BNodeIndex[i] & externalNodeFlag) != 0
                ? -pSrc->externalNodes[BNodeIndex[i] & ~externalNodeFlag]->getDDC()
                : -pSrc->internalNodesAndVars[BNodeIndex[i]].getDDC();

        //***********************************************************************
        // sorting of defect 2: input cells
        //***********************************************************************

        uns dAx = 0, dBx = 0;
        for (uns x = 0; x < s1Asiz; x++) {

            // jump

            while (dAx < Asiz && ANodeIndex[dAx].nodeIndex < ANodeIndex1[x].nodeIndex)
                dAx++;
            while (dBx < Bsiz && BNodeIndex[dBx] < ANodeIndex1[x].nodeIndex)
                dBx++;

            // merge

            if (dAx < Asiz && ANodeIndex1[x].nodeIndex == ANodeIndex[dAx].nodeIndex) { // => JA
                dc->calc->JAUA[dAx] += srcCell1->dc->JRED[x];
                dAx++;
            }
            else if (ANodeIndex1[x].nodeIndex == BNodeIndex[dBx]) { // => JB
                dc->calc->JBUB[dBx] += srcCell1->dc->JRED[x];
                dBx++;
            }
            else
                throw hmgExcept("SunredTreeNode::forwsubsDC", "nodeIndex not found / 10");
        }

        dAx = 0;
        dBx = 0;
        for (uns x = 0; x < s2Asiz; x++) {

            // jump

            while (dAx < Asiz && ANodeIndex[dAx].nodeIndex < ANodeIndex2[x].nodeIndex)
                dAx++;
            while (dBx < Bsiz && BNodeIndex[dBx] < ANodeIndex2[x].nodeIndex)
                dBx++;

            // merge

            if (dAx < Asiz && ANodeIndex2[x].nodeIndex == ANodeIndex[dAx].nodeIndex) { // => JA
                dc->calc->JAUA[dAx] += srcCell2->dc->JRED[x];
                dAx++;
            }
            else if (ANodeIndex2[x].nodeIndex == BNodeIndex[dBx]) { // => JB
                dc->calc->JBUB[dBx] += srcCell2->dc->JRED[x];
                dBx++;
            }
            else
                throw hmgExcept("SunredTreeNode::forwsubsDC", "nodeIndex not found / 10");
        }

        //***********************************************************************
        // reduction
        //***********************************************************************

        if (isSymmDC) {
            if (isChangedDC) {
                if (Bsiz == 1) {
                    dc->calc->NZBXA.math_1_ninv_mulT(dc->calc->YB_NZB, dc->calc->XB); // XA should be here but internal data structure of XB is the same
                    dc->YRED.math_1_add_mul_symm(dc->YRED, dc->calc->XB, dc->calc->NZBXA);
                }
                else if (Bsiz == 2) {
                    dc->calc->NZBXA.math_2_ninv_mul_symmT(dc->calc->YB_NZB, dc->calc->XB);
                    dc->YRED.math_2_add_mul_symm(dc->YRED, dc->calc->XB, dc->calc->NZBXA);
                }
                else {
                    dc->calc->YB_NZB.math_symm_ninv_of_nonsymm();
                    dc->calc->NZBXA.math_mul_t_unsafe(dc->calc->YB_NZB, dc->calc->XB);
                    dc->calc->NZBXAT.transp(dc->calc->NZBXA);
                    dc->YRED.math_add_mul_t_symm(dc->YRED, dc->calc->XB, dc->calc->NZBXAT);
                }
            }
        }
        else {
            if (isChangedDC) {
                if (Bsiz == 1) {
                    dc->calc->NZBXA.math_1_ninv_mulT(dc->calc->YB_NZB, dc->calc->XAT);
                    dc->YRED.math_1_add_mul(dc->YRED, dc->calc->XB, dc->calc->NZBXA);
                }
                else if (Bsiz == 2) {
                    dc->calc->NZBXA.math_2_ninv_mul(dc->calc->YB_NZB, dc->calc->XAT);
                    dc->YRED.math_2_add_mul(dc->YRED, dc->calc->XB, dc->calc->NZBXA);
                }
                else {
                    dc->calc->YB_NZB.math_ninv_np();
                    dc->calc->NZBXA.math_mul_t_unsafe(dc->calc->YB_NZB, dc->calc->XAT);
                    dc->calc->NZBXAT.transp(dc->calc->NZBXA);
                    dc->YRED.math_add_mul_t_unsafe(dc->YRED, dc->calc->XB, dc->calc->NZBXAT);
                }
            }
        }

        //***********************************************************************
        // forwsubs
        //***********************************************************************

        if (Bsiz == 1) {
            math_1x1_mul(dc->calc->NZBJB, dc->calc->YB_NZB, dc->calc->JBUB);
            math_1_add_mul_jred(dc->JRED, dc->calc->JAUA, dc->calc->XB, dc->calc->NZBJB);
        }
        else if (Bsiz == 2) {
            math_2x2_mul(dc->calc->NZBJB, dc->calc->YB_NZB, dc->calc->JBUB);
            math_2_add_mul_jred(dc->JRED, dc->calc->JAUA, dc->calc->XB, dc->calc->NZBJB);
        }
        else {
            math_mul(dc->calc->NZBJB, dc->calc->YB_NZB, dc->calc->JBUB);
            math_add_mul(dc->JRED, dc->calc->JAUA, dc->calc->XB, dc->calc->NZBJB);
        }

    } // else: empty node, belongs to a disabled component, nothing to do
}


//***********************************************************************
void SunredTreeNode::forwsubsAC(ComponentSubCircuit* pSrc) {
//***********************************************************************
    cuns Asiz = (uns)ANodeIndex.size();
    cuns Bsiz = (uns)BNodeIndex.size();
    cuns Csiz = (uns)CNodeIndex.size();
    //***********************************************************************
    if (srcComponent != nullptr) { // leaf
    //***********************************************************************
        bool isChanged = false;
        if (Asiz == Csiz) { // only single nodes
            for (uns y = 0; y < Csiz; y++)
                for (uns x = isSymmAC ? y : 0; x < Csiz; x++) {
                    isChanged = ac->YRED.refresh_unsafe(y, x, srcComponent->getYAC(CNodeIndex[y].componentTerminalIndex, CNodeIndex[x].componentTerminalIndex)) || isChanged;
                }
            for (uns x = 0; x < Csiz; x++) {
                ac->JRED[x] = srcComponent->getJreducedAC(CNodeIndex[x].componentTerminalIndex);
            }
        }
        else { // there is common node
            
            // copy
            
            for (uns y = 0; y < Csiz; y++)
                for (uns x = isSymmAC ? y : 0; x < Csiz; x++) {
                    isChanged = ac->leaf->YA.refresh_unsafe(y, x, srcComponent->getYAC(CNodeIndex[y].componentTerminalIndex, CNodeIndex[x].componentTerminalIndex)) || isChanged;
                }
            for (uns x = 0; x < Csiz; x++) {
                ac->leaf->JA[x] = srcComponent->getJreducedAC(CNodeIndex[x].componentTerminalIndex);
            }
            
            // merge
            
            ac->YRED.zero_unsafe();
            ac->JRED.zero();
            for (uns y = 0; y < Bsiz; y++)
                for (uns x = isSymmAC ? y : 0; x < Bsiz; x++) {
                    ac->YRED.get_elem(BNodeIndex[y], BNodeIndex[x]) += ac->leaf->YA[y][x];
                }
            for (uns x = 0; x < Bsiz; x++) {
                ac->JRED[BNodeIndex[x]] += ac->leaf->JA[x];
            }
        }
        isChangedAC = isChanged;
    }
    //***********************************************************************
    else if (srcCell1 != nullptr) { // nonleaf
    //***********************************************************************

        cuns s1Asiz = (uns)srcCell1->ANodeIndex.size();
        const auto& ANodeIndex1 = srcCell1->ANodeIndex;
        cuns s2Asiz = (uns)srcCell2->ANodeIndex.size();
        const auto& ANodeIndex2 = srcCell2->ANodeIndex;

        //***********************************************************************
        // sorting of admittances
        //***********************************************************************

        if (srcCell1->isChangedAC || srcCell2->isChangedAC) {

            isChangedAC = true;
            ac->YRED.zero_unsafe();
            ac->calc->XB.zero_unsafe();
            ac->calc->YB_NZB.zero_unsafe();

            if (isSymmAC) { // src1->YRED and src2->YRED => YRED (as YA) and XB and YB_NZB (XAT is not used)
                
                // src1 => this

                uns dAy = 0, dBy = 0;
                for (uns y = 0; y < s1Asiz; y++) {

                    // jump

                    while (dAy < Asiz && ANodeIndex[dAy].nodeIndex < ANodeIndex1[y].nodeIndex)
                        dAy++;
                    while (dBy < Bsiz && BNodeIndex[dBy] < ANodeIndex1[y].nodeIndex)
                        dBy++;

                    // merge

                    if (dAy < Asiz && ANodeIndex1[y].nodeIndex == ANodeIndex[dAy].nodeIndex) {
                        uns dAx = dAy, dBx = dBy;
                        for (uns x = y; x < s1Asiz; x++) {

                            // jump

                            while (dAx < Asiz && ANodeIndex[dAx].nodeIndex < ANodeIndex1[x].nodeIndex)
                                dAx++;
                            while (dBx < Bsiz && BNodeIndex[dBx] < ANodeIndex1[x].nodeIndex)
                                dBx++;

                            // merge

                            if (dAx < Asiz && ANodeIndex1[x].nodeIndex == ANodeIndex[dAx].nodeIndex) { // => YA
                                ac->YRED[dAy][dAx] += srcCell1->ac->YRED[y][x];
                                dAx++;
                            }
                            else if (ANodeIndex1[x].nodeIndex == BNodeIndex[dBx]) { // => XB
                                ac->calc->XB[dAy][dBx] += srcCell1->ac->YRED[y][x];
                                dBx++;
                            }
                            else
                                throw hmgExcept("SunredTreeNode::forwsubsAC", "nodeIndex not found / 1");
                        }
                        dAy++;
                    }
                    else if (ANodeIndex1[y].nodeIndex == BNodeIndex[dBy]) {
                        uns dAx = dAy, dBx = dBy;
                        for (uns x = y; x < s1Asiz; x++) {

                            // jump

                            while (dAx < Asiz && ANodeIndex[dAx].nodeIndex < ANodeIndex1[x].nodeIndex)
                                dAx++;
                            while (dBx < Bsiz && BNodeIndex[dBx] < ANodeIndex1[x].nodeIndex)
                                dBx++;

                            // merge

                            if (dAx < Asiz && ANodeIndex1[x].nodeIndex == ANodeIndex[dAx].nodeIndex) { // => XA
                                ac->calc->XB[dAx][dBy] += srcCell1->ac->YRED[y][x]; // XB = XAT
                                dAx++;
                            }
                            else if (ANodeIndex1[x].nodeIndex == BNodeIndex[dBx]) { // => YB
                                ac->calc->YB_NZB[dBy][dBx] += srcCell1->ac->YRED[y][x];
                                dBx++;
                            }
                            else
                                throw hmgExcept("SunredTreeNode::forwsubsAC", "nodeIndex not found / 2");
                        }
                        dBy++;
                    }
                    else
                        throw hmgExcept("SunredTreeNode::forwsubsAC", "nodeIndex not found / 3");
                }

                // src2 => this

                dAy = 0;
                dBy = 0;
                for (uns y = 0; y < s2Asiz; y++) {

                    // jump

                    while (dAy < Asiz && ANodeIndex[dAy].nodeIndex < ANodeIndex2[y].nodeIndex)
                        dAy++;
                    while (dBy < Bsiz && BNodeIndex[dBy] < ANodeIndex2[y].nodeIndex)
                        dBy++;

                    // merge

                    if (dAy < Asiz && ANodeIndex2[y].nodeIndex == ANodeIndex[dAy].nodeIndex) {
                        uns dAx = dAy, dBx = dBy;
                        for (uns x = y; x < s2Asiz; x++) {

                            // jump

                            while (dAx < Asiz && ANodeIndex[dAx].nodeIndex < ANodeIndex2[x].nodeIndex)
                                dAx++;
                            while (dBx < Bsiz && BNodeIndex[dBx] < ANodeIndex2[x].nodeIndex)
                                dBx++;

                            // merge

                            if (dAx < Asiz && ANodeIndex2[x].nodeIndex == ANodeIndex[dAx].nodeIndex) { // => YA
                                ac->YRED[dAy][dAx] += srcCell2->ac->YRED[y][x];
                                dAx++;
                            }
                            else if (ANodeIndex2[x].nodeIndex == BNodeIndex[dBx]) { // => XB
                                ac->calc->XB[dAy][dBx] += srcCell2->ac->YRED[y][x];
                                dBx++;
                            }
                            else
                                throw hmgExcept("SunredTreeNode::forwsubsAC", "nodeIndex not found / 4");
                        }
                        dAy++;
                    }
                    else if (ANodeIndex2[y].nodeIndex == BNodeIndex[dBy]) {
                        uns dAx = dAy, dBx = dBy;
                        for (uns x = y; x < s2Asiz; x++) {

                            // jump

                            while (dAx < Asiz && ANodeIndex[dAx].nodeIndex < ANodeIndex2[x].nodeIndex)
                                dAx++;
                            while (dBx < Bsiz && BNodeIndex[dBx] < ANodeIndex2[x].nodeIndex)
                                dBx++;

                            // merge

                            if (dAx < Asiz && ANodeIndex2[x].nodeIndex == ANodeIndex[dAx].nodeIndex) { // => XA
                                ac->calc->XB[dAx][dBy] += srcCell2->ac->YRED[y][x]; // XB = XAT
                                dAx++;
                            }
                            else if (ANodeIndex2[x].nodeIndex == BNodeIndex[dBx]) { // => YB
                                ac->calc->YB_NZB[dBy][dBx] += srcCell2->ac->YRED[y][x];
                                dBx++;
                            }
                            else
                                throw hmgExcept("SunredTreeNode::forwsubsAC", "nodeIndex not found / 5");
                        }
                        dBy++;
                    }
                    else
                        throw hmgExcept("SunredTreeNode::forwsubsAC", "nodeIndex not found / 6");
                }

                if (Bsiz != 1)
                    ac->calc->YB_NZB.symmetrize_from_upper();
            }
            else { // result is nonsymm (one of the inputs can be); src1->YRED and src2->YRED => YRED (as YA) and XAT and XB and YB_NZB

                ac->calc->XAT.zero_unsafe();

                // src1 => this

                uns dAy = 0, dBy = 0;
                for (uns y = 0; y < s1Asiz; y++) {

                    // jump

                    while (dAy < Asiz && ANodeIndex[dAy].nodeIndex < ANodeIndex1[y].nodeIndex)
                        dAy++;
                    while (dBy < Bsiz && BNodeIndex[dBy] < ANodeIndex1[y].nodeIndex)
                        dBy++;

                    // merge

                    if (dAy < Asiz && ANodeIndex1[y].nodeIndex == ANodeIndex[dAy].nodeIndex) {
                        uns dAx = 0, dBx = 0;
                        for (uns x = 0; x < s1Asiz; x++) {

                            // jump

                            while (dAx < Asiz && ANodeIndex[dAx].nodeIndex < ANodeIndex1[x].nodeIndex)
                                dAx++;
                            while (dBx < Bsiz && BNodeIndex[dBx] < ANodeIndex1[x].nodeIndex)
                                dBx++;

                            // merge

                            if (dAx < Asiz && ANodeIndex1[x].nodeIndex == ANodeIndex[dAx].nodeIndex) { // => YA
                                ac->YRED[dAy][dAx] += srcCell1->ac->YRED.get_elem(y, x);
                                dAx++;
                            }
                            else if (ANodeIndex1[x].nodeIndex == BNodeIndex[dBx]) { // => XB
                                ac->calc->XB[dAy][dBx] += srcCell1->ac->YRED.get_elem(y, x);
                                dBx++;
                            }
                            else
                                throw hmgExcept("SunredTreeNode::forwsubsAC", "nodeIndex not found / 7");
                        }
                        dAy++;
                    }
                    else if (ANodeIndex1[y].nodeIndex == BNodeIndex[dBy]) {
                        uns dAx = 0, dBx = 0;
                        for (uns x = 0; x < s1Asiz; x++) {

                            // jump

                            while (dAx < Asiz && ANodeIndex[dAx].nodeIndex < ANodeIndex1[x].nodeIndex)
                                dAx++;
                            while (dBx < Bsiz && BNodeIndex[dBx] < ANodeIndex1[x].nodeIndex)
                                dBx++;

                            // merge

                            if (dAx < Asiz && ANodeIndex1[x].nodeIndex == ANodeIndex[dAx].nodeIndex) { // => XA
                                ac->calc->XAT[dAx][dBy] += srcCell1->ac->YRED.get_elem(y, x);
                                dAx++;
                            }
                            else if (ANodeIndex1[x].nodeIndex == BNodeIndex[dBx]) { // => YB
                                ac->calc->YB_NZB[dBy][dBx] += srcCell1->ac->YRED.get_elem(y, x);
                                dBx++;
                            }
                            else
                                throw hmgExcept("SunredTreeNode::forwsubsAC", "nodeIndex not found / 8");
                        }
                        dBy++;
                    }
                    else
                        throw hmgExcept("SunredTreeNode::forwsubsAC", "nodeIndex not found / 9");
                }

                // src2 => this

                dAy = 0;
                dBy = 0;
                for (uns y = 0; y < s2Asiz; y++) {

                    // jump

                    while (dAy < Asiz && ANodeIndex[dAy].nodeIndex < ANodeIndex2[y].nodeIndex)
                        dAy++;
                    while (dBy < Bsiz && BNodeIndex[dBy] < ANodeIndex2[y].nodeIndex)
                        dBy++;

                    // merge

                    if (dAy < Asiz && ANodeIndex2[y].nodeIndex == ANodeIndex[dAy].nodeIndex) {
                        uns dAx = 0, dBx = 0;
                        for (uns x = 0; x < s2Asiz; x++) {

                            // jump

                            while (dAx < Asiz && ANodeIndex[dAx].nodeIndex < ANodeIndex2[x].nodeIndex)
                                dAx++;
                            while (dBx < Bsiz && BNodeIndex[dBx] < ANodeIndex2[x].nodeIndex)
                                dBx++;

                            // merge

                            if (dAx < Asiz && ANodeIndex2[x].nodeIndex == ANodeIndex[dAx].nodeIndex) { // => YA
                                ac->YRED[dAy][dAx] += srcCell2->ac->YRED.get_elem(y, x);
                                dAx++;
                            }
                            else if (ANodeIndex2[x].nodeIndex == BNodeIndex[dBx]) { // => XB
                                ac->calc->XB[dAy][dBx] += srcCell2->ac->YRED.get_elem(y, x);
                                dBx++;
                            }
                            else
                                throw hmgExcept("SunredTreeNode::forwsubsAC", "nodeIndex not found / 10");
                        }
                        dAy++;
                    }
                    else if (ANodeIndex2[y].nodeIndex == BNodeIndex[dBy]) {
                        uns dAx = 0, dBx = 0;
                        for (uns x = 0; x < s2Asiz; x++) {

                            // jump

                            while (dAx < Asiz && ANodeIndex[dAx].nodeIndex < ANodeIndex2[x].nodeIndex)
                                dAx++;
                            while (dBx < Bsiz && BNodeIndex[dBx] < ANodeIndex2[x].nodeIndex)
                                dBx++;

                            // merge

                            if (dAx < Asiz && ANodeIndex2[x].nodeIndex == ANodeIndex[dAx].nodeIndex) { // => XA
                                ac->calc->XAT[dAx][dBy] += srcCell2->ac->YRED.get_elem(y, x);
                                dAx++;
                            }
                            else if (ANodeIndex2[x].nodeIndex == BNodeIndex[dBx]) { // => YB
                                ac->calc->YB_NZB[dBy][dBx] += srcCell2->ac->YRED.get_elem(y, x);
                                dBx++;
                            }
                            else
                                throw hmgExcept("SunredTreeNode::forwsubsAC", "nodeIndex not found / 11");
                        }
                        dBy++;
                    }
                    else
                        throw hmgExcept("SunredTreeNode::forwsubsAC", "nodeIndex not found / 12");
                }
            }

        }
        else {
            isChangedAC = false;
        }

        //***********************************************************************
        // sorting of defect 1: reading common nodes defect
        //***********************************************************************

        ac->calc->JAUA.zero();

        for (uns i = 0; i < Bsiz; i++)
            ac->calc->JBUB[i] = (BNodeIndex[i] & externalNodeFlag) != 0
                ? -pSrc->externalNodes[BNodeIndex[i] & ~externalNodeFlag]->getDAC()
                : -pSrc->internalNodesAndVars[BNodeIndex[i]].getDAC();

        //***********************************************************************
        // sorting of defect 2: input cells
        //***********************************************************************

        uns dAx = 0, dBx = 0;
        for (uns x = 0; x < s1Asiz; x++) {

            // jump

            while (dAx < Asiz && ANodeIndex[dAx].nodeIndex < ANodeIndex1[x].nodeIndex)
                dAx++;
            while (dBx < Bsiz && BNodeIndex[dBx] < ANodeIndex1[x].nodeIndex)
                dBx++;

            // merge

            if (dAx < Asiz && ANodeIndex1[x].nodeIndex == ANodeIndex[dAx].nodeIndex) { // => JA
                ac->calc->JAUA[dAx] += srcCell1->ac->JRED[x];
                dAx++;
            }
            else if (ANodeIndex1[x].nodeIndex == BNodeIndex[dBx]) { // => JB
                ac->calc->JBUB[dBx] += srcCell1->ac->JRED[x];
                dBx++;
            }
            else
                throw hmgExcept("SunredTreeNode::forwsubsAC", "nodeIndex not found / 10");
        }

        dAx = 0;
        dBx = 0;
        for (uns x = 0; x < s2Asiz; x++) {

            // jump

            while (dAx < Asiz && ANodeIndex[dAx].nodeIndex < ANodeIndex2[x].nodeIndex)
                dAx++;
            while (dBx < Bsiz && BNodeIndex[dBx] < ANodeIndex2[x].nodeIndex)
                dBx++;

            // merge

            if (dAx < Asiz && ANodeIndex2[x].nodeIndex == ANodeIndex[dAx].nodeIndex) { // => JA
                ac->calc->JAUA[dAx] += srcCell2->ac->JRED[x];
                dAx++;
            }
            else if (ANodeIndex2[x].nodeIndex == BNodeIndex[dBx]) { // => JB
                ac->calc->JBUB[dBx] += srcCell2->ac->JRED[x];
                dBx++;
            }
            else
                throw hmgExcept("SunredTreeNode::forwsubsAC", "nodeIndex not found / 10");
        }

        //***********************************************************************
        // reduction
        //***********************************************************************

        if (isSymmAC) {
            if (isChangedAC) {
                if (Bsiz == 1) {
                    ac->calc->NZBXA.math_1_ninv_mulT(ac->calc->YB_NZB, ac->calc->XB); // XA should be here but internal data structure of XB is the same
                    ac->YRED.math_1_add_mul_symm(ac->YRED, ac->calc->XB, ac->calc->NZBXA);
                }
                else if (Bsiz == 2) {
                    ac->calc->NZBXA.math_2_ninv_mul_symmT(ac->calc->YB_NZB, ac->calc->XB);
                    ac->YRED.math_2_add_mul_symm(ac->YRED, ac->calc->XB, ac->calc->NZBXA);
                }
                else {
                    ac->calc->YB_NZB.math_symm_ninv_of_nonsymm();
                    ac->calc->NZBXA.math_mul_t_unsafe(ac->calc->YB_NZB, ac->calc->XB);
                    ac->calc->NZBXAT.transp(ac->calc->NZBXA);
                    ac->YRED.math_add_mul_t_symm(ac->YRED, ac->calc->XB, ac->calc->NZBXAT);
                }
            }
        }
        else {
            if (isChangedAC) {
                if (Bsiz == 1) {
                    ac->calc->NZBXA.math_1_ninv_mulT(ac->calc->YB_NZB, ac->calc->XB); // XA should be here but internal data structure of XB is the same
                    ac->YRED.math_1_add_mul(ac->YRED, ac->calc->XB, ac->calc->NZBXA);
                }
                else if (Bsiz == 2) {
                    ac->calc->NZBXA.math_2_ninv_mul(ac->calc->YB_NZB, ac->calc->XAT);
                    ac->YRED.math_2_add_mul(ac->YRED, ac->calc->XB, ac->calc->NZBXA);
                }
                else {
                    ac->calc->YB_NZB.math_ninv_np();
                    ac->calc->NZBXA.math_mul_t_unsafe(ac->calc->YB_NZB, ac->calc->XAT);
                    ac->calc->NZBXAT.transp(ac->calc->NZBXA);
                    ac->YRED.math_add_mul_t_unsafe(ac->YRED, ac->calc->XB, ac->calc->NZBXAT);
                }
            }
        }

        //***********************************************************************
        // forwsubs
        //***********************************************************************

        if (Bsiz == 1) {
            math_1x1_mul(ac->calc->NZBJB, ac->calc->YB_NZB, ac->calc->JBUB);
            math_1_add_mul_jred(ac->JRED, ac->calc->JAUA, ac->calc->XB, ac->calc->NZBJB);
        }
        else if (Bsiz == 2) {
            math_2x2_mul(ac->calc->NZBJB, ac->calc->YB_NZB, ac->calc->JBUB);
            math_2_add_mul_jred(ac->JRED, ac->calc->JAUA, ac->calc->XB, ac->calc->NZBJB);
        }
        else {
            math_mul(ac->calc->NZBJB, ac->calc->YB_NZB, ac->calc->JBUB);
            math_add_mul(ac->JRED, ac->calc->JAUA, ac->calc->XB, ac->calc->NZBJB);
        }

    } // else: empty node, belongs to a disabled component, nothing to do
}


//***********************************************************************
void SunredTreeNode::backsubsDC(ComponentSubCircuit* pSrc) {
//***********************************************************************
    cuns Asiz = (uns)ANodeIndex.size();
    cuns Bsiz = (uns)BNodeIndex.size();

    //***********************************************************************
    if(srcComponent != nullptr) { // leaf
    //***********************************************************************
        // nothing to do
    }
    //***********************************************************************
    else if (srcCell1 != nullptr) { // nonleaf
    //***********************************************************************

        //***********************************************************************
        // fill UA
        //***********************************************************************

        for (uns i = 0; i < Asiz; i++) {
            uns nodeIndex = ANodeIndex[i].nodeIndex;
            dc->calc->JAUA[i] = (nodeIndex & externalNodeFlag) != 0
                ? pSrc->externalNodes[nodeIndex & ~externalNodeFlag]->getVDC()
                : pSrc->internalNodesAndVars[nodeIndex].getVDC();
        }

        //***********************************************************************
        // backsubs
        //***********************************************************************

        if (Bsiz == 1)      math_1_add_mul_ub(dc->calc->JBUB, dc->calc->NZBJB, dc->calc->NZBXA, dc->calc->JAUA);
        else if (Bsiz == 2) math_2_add_mul_ub(dc->calc->JBUB, dc->calc->NZBJB, dc->calc->NZBXA, dc->calc->JAUA);
        else                math_add_mul(dc->calc->JBUB, dc->calc->NZBJB, dc->calc->NZBXA, dc->calc->JAUA);

        //***********************************************************************
        // v of the internal nodes
        //***********************************************************************

        for (uns i = 0; i < Bsiz; i++) {
            uns nodeIndex = BNodeIndex[i];
            if ((nodeIndex & externalNodeFlag) != 0) {
                pSrc->externalNodes[nodeIndex & ~externalNodeFlag]->setVDC(dc->calc->JBUB[i]);
            }
            else {
                pSrc->internalNodesAndVars[nodeIndex].setVDC(dc->calc->JBUB[i]);
            }
        }

    } // else: empty node, belongs to a disabled component, nothing to do
}


//***********************************************************************
void SunredTreeNode::backsubsAC(ComponentSubCircuit* pSrc) {
//***********************************************************************
    cuns Asiz = (uns)ANodeIndex.size();
    cuns Bsiz = (uns)BNodeIndex.size();

    //***********************************************************************
    if(srcComponent != nullptr) { // leaf
    //***********************************************************************
        // nothing to do
    }
    //***********************************************************************
    else if (srcCell1 != nullptr) { // nonleaf
    //***********************************************************************

        //***********************************************************************
        // fill UA
        //***********************************************************************

        for (uns i = 0; i < Asiz; i++) {
            uns nodeIndex = ANodeIndex[i].nodeIndex;
            ac->calc->JAUA[i] = (nodeIndex & externalNodeFlag) != 0
                ? pSrc->externalNodes[nodeIndex & ~externalNodeFlag]->getVAC()
                : pSrc->internalNodesAndVars[nodeIndex].getVAC();
        }

        //***********************************************************************
        // backsubs
        //***********************************************************************

        if (Bsiz == 1)      math_1_add_mul_ub(ac->calc->JBUB, ac->calc->NZBJB, ac->calc->NZBXA, ac->calc->JAUA);
        else if (Bsiz == 2) math_2_add_mul_ub(ac->calc->JBUB, ac->calc->NZBJB, ac->calc->NZBXA, ac->calc->JAUA);
        else                math_add_mul(ac->calc->JBUB, ac->calc->NZBJB, ac->calc->NZBXA, ac->calc->JAUA);

        //***********************************************************************
        // v of the internal nodes
        //***********************************************************************

        for (uns i = 0; i < Bsiz; i++) {
            uns nodeIndex = BNodeIndex[i];
            if ((nodeIndex & externalNodeFlag) != 0) {
                pSrc->externalNodes[nodeIndex & ~externalNodeFlag]->setVAC(ac->calc->JBUB[i]);
            }
            else {
                pSrc->internalNodesAndVars[nodeIndex].setVAC(ac->calc->JBUB[i]);
            }
        }

    } // else: empty node, belongs to a disabled component, nothing to do
}


//***********************************************************************
void hmgSunred::buildTree(const ReductionTreeInstructions& instr, ComponentSubCircuit* pSrc_) {
// TO PARALLEL: cells in a level can be loaded parallel
//***********************************************************************
    pSrc = pSrc_;
    pSrc->setNodesToComponents();

    levels.clear();
    levels.resize(instr.data.size() + 1);

    // Level 0

    levels[0].resize(pSrc->components.size());
    auto& level0 = levels[0];
    auto& comp = pSrc->components;
    for (size_t i = 0; i < level0.size(); i++) {
        if (comp[i]->isEnabled) // only the enabled componets are loaded !
            level0[i].loadLeafDataFromSubcircuit(comp[i].get(), pSrc);
    }

    // Levels 1..n

    for (size_t i = 0; i < instr.data.size(); i++) {
        auto& level = levels[i + 1];
        const auto& instLev = instr.data[i];
        level.resize(instLev.size());
        for (size_t j = 0; j < level.size(); j++) {
            const auto& inst = instLev[j];
            level[j].loadNodeDataFromTwoNodes(&levels[inst.cell1Level][inst.cell1Index], &levels[inst.cell2Level][inst.cell2Index]);
        }
    }
}




//***********************************************************************
}
//***********************************************************************
