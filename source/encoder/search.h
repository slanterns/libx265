/*****************************************************************************
* Copyright (C) 2013 x265 project
*
* Authors: Steve Borho <steve@borho.org>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111, USA.
*
* This program is also available under a commercial proprietary license.
* For more information, contact us at license @ x265.com.
*****************************************************************************/

#ifndef X265_SEARCH_H
#define X265_SEARCH_H

#include "common.h"
#include "predict.h"
#include "quant.h"
#include "bitcost.h"

#include "TLibCommon/TComYuv.h"
#include "TLibCommon/TComMotionInfo.h"
#include "TLibCommon/TComPattern.h"

#include "rdcost.h"
#include "motion.h"

#define MVP_IDX_BITS 1
#define NUM_LAYERS 4

namespace x265 {
// private namespace

class Encoder;
class Entropy;

struct MotionData
{
    MV  mv;
    MV  mvp;
    int mvpIdx;
    int ref;
    uint32_t cost;
    int bits;
};

struct MergeData
{
    /* merge candidate data, cached between calls to xMergeEstimation */
    TComMvField mvFieldNeighbours[MRG_MAX_NUM_CANDS][2];
    uint8_t     interDirNeighbours[MRG_MAX_NUM_CANDS];
    uint32_t    maxNumMergeCand;

    /* data updated for each partition */
    uint32_t    absPartIdx;
    int         width;
    int         height;

    /* outputs */
    TComMvField mvField[2];
    uint32_t    interDir;
    uint32_t    index;
    uint32_t    bits;
};

inline int getTUBits(int idx, int numIdx)
{
    return idx + (idx < numIdx - 1);
}

class Search : public Predict
{
public:

    static const pixel zeroPel[MAX_CU_SIZE];

    MotionEstimate  m_me;
    Quant           m_quant;
    RDCost          m_rdCost;
    x265_param*     m_param;

    Entropy*        m_entropyCoder;
    Entropy       (*m_rdEntropyCoders)[CI_NUM];

    TComYuv         m_predTempYuv;
    ShortYuv*       m_qtTempShortYuv;
    coeff_t*        m_qtTempCoeff[3][NUM_LAYERS];
    uint8_t*        m_qtTempCbf[3];
    uint8_t*        m_qtTempTransformSkipFlag[3];

    bool            m_bFrameParallel;
    bool            m_bEnableRDOQ;
    int             m_numLayers;
    int             m_refLagPixels;

    Search();
    ~Search();

    /* TODO: Don't pass top-level encoder all the way down here */
    bool initSearch(Encoder& top);

    void estIntraPredQT(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, ShortYuv* resiYuv, TComYuv* reconYuv, uint32_t* depthRange);

    void getBestIntraModeChroma(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv);
    void estIntraPredChromaQT(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, ShortYuv* resiYuv, TComYuv* reconYuv);

    // estimation inter prediction (non-skip)
    bool predInterSearch(TComDataCU* cu, TComYuv* predYuv, bool bMergeOnly, bool bChroma);

    // encode residual and compute rd-cost for inter mode
    void encodeResAndCalcRdInterCU(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, ShortYuv* resiYuv, ShortYuv* bestResiYuv, TComYuv* reconYuv);
    void encodeResAndCalcRdSkipCU(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, TComYuv* reconYuv);

    void generateCoeffRecon(TComDataCU* cu, TComYuv* fencYuv, TComYuv* predYuv, ShortYuv* resiYuv, TComYuv* reconYuv);
    void residualTransformQuantInter(TComDataCU* cu, uint32_t absPartIdx, TComYuv* fencYuv, ShortYuv* resiYuv, uint32_t depth, uint32_t* depthRange);
    void offsetSubTUCBFs(TComDataCU* cu, TextType ttype, uint32_t trDepth, uint32_t absPartIdx);

protected:

    void xRecurIntraCodingQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, TComYuv* fencYuv, TComYuv* predYuv,
                             ShortYuv* resiYuv, uint32_t& distY, bool bCheckFirst, uint64_t& dRDCost, uint32_t* depthRange);

    void xEstimateResidualQT(TComDataCU* cu, uint32_t absPartIdx, TComYuv* fencYuv, TComYuv* predYuv, ShortYuv* resiYuv, uint32_t depth,
                             uint64_t &rdCost, uint32_t &outBits, uint32_t &outDist, uint32_t *puiZeroDist, uint32_t* tuDepthRange);
    void xSetResidualQTData(TComDataCU* cu, uint32_t absPartIdx, ShortYuv* resiYuv, uint32_t depth, bool bSpatial);
    void xSetIntraResultQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, TComYuv* reconYuv);

    void xEncSubdivCbfQTLuma(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, uint32_t* depthRange);
    void xEncSubdivCbfQTChroma(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx,  uint32_t absPartIdxStep, uint32_t width, uint32_t height);

    void xEncCoeffQTLuma(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx);
    void xEncCoeffQTChroma(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, TextType ttype);
    void xEncIntraHeaderLuma(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx);
    void xEncIntraHeaderChroma(TComDataCU* cu, uint32_t absPartIdx);
    uint32_t xGetIntraBitsQTLuma(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, uint32_t* depthRange);
    uint32_t xGetIntraBitsQTChroma(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, uint32_t absPartIdxStep);
    uint32_t xGetIntraBitsLuma(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, uint32_t log2TrSize, coeff_t* coeff, uint32_t* depthRange);
    uint32_t xGetIntraBitsChroma(TComDataCU* cu, uint32_t absPartIdx, uint32_t log2TrSizeC, uint32_t chromaId, coeff_t* coeff);
    uint32_t xIntraCodingLumaBlk(TComDataCU* cu, uint32_t absPartIdx, uint32_t log2TrSize, TComYuv* fencYuv, TComYuv* predYuv, ShortYuv* resiYuv,
                                 int16_t* reconQt, uint32_t reconQtStride, coeff_t* coeff, uint32_t& cbf);

    uint32_t xRecurIntraChromaCodingQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, TComYuv* fencYuv, TComYuv* predYuv, ShortYuv* resiYuv);

    void xIntraCodingChromaBlk(TComDataCU* cu, uint32_t absPartIdx, TComYuv* fencYuv, TComYuv* predYuv, ShortYuv* resiYuv,
                               int16_t* reconQt, uint32_t reconQtStride, coeff_t* coeff,
                               uint32_t& cbf, uint32_t& outDist, uint32_t chromaId, uint32_t log2TrSizeC);

    void residualTransformQuantIntra(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, TComYuv* fencYuv,
                                     TComYuv* predYuv, ShortYuv* resiYuv, TComYuv* reconYuv, uint32_t* depthRange);
    void residualQTIntrachroma(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, TComYuv* fencYuv,
                               TComYuv* predYuv, ShortYuv* resiYuv, TComYuv* reconYuv);

    void xSetIntraResultChromaQT(TComDataCU* cu, uint32_t trDepth, uint32_t absPartIdx, TComYuv* reconYuv);

    void xLoadIntraResultQT(TComDataCU* cu, uint32_t absPartIdx, uint32_t log2TrSize, int16_t* reconQt, uint32_t reconQtStride);
    void xLoadIntraResultChromaQT(TComDataCU* cu, uint32_t absPartIdx, uint32_t log2TrSizeC, uint32_t chromaId, int16_t* reconQt, uint32_t reconQtStride);

    void     xCheckBestMVP(MV* amvpCand, MV cMv, MV& mvPred, int& mvpIdx, uint32_t& outBits, uint32_t& outCost);
    void     xGetBlkBits(PartSize cuMode, bool bPSlice, int partIdx, uint32_t lastMode, uint32_t blockBit[3]);
    uint32_t xSymbolBitsInter(TComDataCU* cu, uint32_t* depthRange);
    uint32_t xModeBitsIntra(TComDataCU* cu, uint32_t mode, uint32_t partOffset, uint32_t depth);
    uint32_t xModeBitsRemIntra(TComDataCU * cu, uint32_t partOffset, uint32_t depth, uint32_t preds[3], uint64_t & mpms);
    uint32_t xUpdateCandList(uint32_t mode, uint64_t cost, uint32_t fastCandNum, uint32_t* CandModeList, uint64_t* CandCostList);

    uint32_t xMergeEstimation(TComDataCU* cu, int partIdx, MergeData& m);
    void xSetSearchRange(TComDataCU* cu, MV mvp, int merange, MV& mvmin, MV& mvmax);

    void xEncodeResidualQT(TComDataCU* cu, uint32_t absPartIdx, uint32_t depth, bool bSubdivAndCbf, TextType ttype, uint32_t* depthRange);
};
}

#endif // ifndef X265_SEARCH_H
