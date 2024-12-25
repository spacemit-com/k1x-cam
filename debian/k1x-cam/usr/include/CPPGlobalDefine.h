/*
 * Copyright (C) 2023 Spacemit Limited
 * All Rights Reserved.
 */
#ifndef _CPPGLOBALDEFINE_H_
#define _CPPGLOBALDEFINE_H_

#define LAYERNUM 5
#define GAIN_NUM 12

typedef struct {
    // int m_nCpp_nr_en;
    //  ynr
    int m_pMax_diff_thr[LAYERNUM][GAIN_NUM];
    int m_pSig_gain[LAYERNUM][GAIN_NUM];
    // int m_pAlpha[LAYERNUM][GAIN_NUM];
    int m_pAback_alpha[LAYERNUM][GAIN_NUM];

    // cnr
    int m_pCnrSig_y[LAYERNUM][GAIN_NUM];
    int m_pCnrSig_uv[LAYERNUM][GAIN_NUM];
    int m_pCnrAback_alpha[LAYERNUM][GAIN_NUM];
    int m_pCnrSig_uv_shift[LAYERNUM][GAIN_NUM];

    int m_pGainIndex[GAIN_NUM];

    // Luma
    int m_bDynamic_en[LAYERNUM];
    int m_bLuma_en;
    int m_pLuma_sig[17][GAIN_NUM];

    int m_bCnrLuma_en;
    int m_pCnrLuma_sig[17][GAIN_NUM];
    // Radial
    int m_bRadial_en;
    int m_pRadial_ratio[LAYERNUM][GAIN_NUM];
    int m_bCnrRadial_en;
    int m_pCnrRadial_ratio[LAYERNUM][GAIN_NUM];

    // FD
    int eb;
    int uv_mode;  // eb

    // pfc
    int m_ppf_en[LAYERNUM];
    int m_nwp_en;
    int m_nwp_th;
    int m_nnum_wp_min;
    int m_nnum_wp_step;
    int m_nhue_pf_min;
    int m_nhue_pf_max;
    int m_nhue_pf_tr_step;
    int m_nuv_wp_gain[GAIN_NUM];
    int m_nuv_pf_gain[GAIN_NUM];

    int m_nGlobalYnrStrength;
    int m_nGlobalCnrStrength;

    // tnr
    int m_nTnrEb;
    int m_pTnrDAvg[LAYERNUM][GAIN_NUM];
    int m_pTnrWAvg[LAYERNUM][GAIN_NUM];
    int m_pTnrQWeightAvg[LAYERNUM][GAIN_NUM];
    int m_pTnrDSad[LAYERNUM][GAIN_NUM];
    int m_pTnrWSad[LAYERNUM][GAIN_NUM];
    int m_pTnrQWeightSad[LAYERNUM][GAIN_NUM];
    int m_pTnrDuv[LAYERNUM][GAIN_NUM];
    int m_pTnrWuv[LAYERNUM][GAIN_NUM];
    int m_pTnrQWeightUV[LAYERNUM][GAIN_NUM];
    // int m_pTnrPreQ[LAYERNUM][GAIN_NUM];
    // int m_nTnrPreR;
    // int m_nTnrPreD;
    // int m_nTnrPreRWeight;
    // int m_nTnrFirstFrame;
    // int m_nTnrConstP;
    // int m_nTnrConstR;
    int m_nTnrLumaWeight[GAIN_NUM];
    int m_pTnrBlockWeight0[9][3];
    int m_pTnrBlockWeight1[9][3];
    int m_pTnrBlockWeight234[9][3];
    int m_pTnrLumaBase[LAYERNUM][GAIN_NUM];
    // int m_pTnrRdBase[LAYERNUM][GAIN_NUM];
    int m_pTnrLuma[16][GAIN_NUM];
    // int m_nTnrGlbY;
    // int m_nTnrGlbU;
    // int m_nTnrGlbV;
    int m_pTnr3dYEb[LAYERNUM][GAIN_NUM];
    // int m_nTnrYA1[GAIN_NUM];
    // int m_nTnrYA2[GAIN_NUM];
    int m_pTnr3dUVEb[LAYERNUM][GAIN_NUM];
    // int m_nTnrUVA1[GAIN_NUM];
    // int m_nTnrUVA2[GAIN_NUM];
    int m_pTnrYStrengthQ6[LAYERNUM][GAIN_NUM];
    int m_pTnrUVStrengthQ6[LAYERNUM][GAIN_NUM];
    int m_pTnr2dYEb[LAYERNUM][GAIN_NUM];
    int m_pTnr2dMappingCurve[17];
    int m_pStrongSig_gain[LAYERNUM][GAIN_NUM];
    int m_pStrongAback_alpha[LAYERNUM][GAIN_NUM];
    int m_nClipMode;

    // manual mode ynr
    int m_bManualMode;
    int m_nCpp_nr_en_Manual;
    int m_pMax_diff_thr_Manual[LAYERNUM];
    int m_pSig_gain_Manual[LAYERNUM];
    int m_pAlpha_Manual[LAYERNUM];
    int m_pAback_alpha_Manual[LAYERNUM];

    // manual mode cnr
    int m_pCnrSig_y_Manual[LAYERNUM];
    int m_pCnrSig_uv_Manual[LAYERNUM];
    int m_pCnrAback_alpha_Manual[LAYERNUM];
    int m_pCnrSig_uv_shift_Manual[LAYERNUM];

    // night mode param
    int m_pSig_gainRatio[GAIN_NUM];
    int m_pAback_alphaRatio[GAIN_NUM];
    int m_pCnrSig_yRatio[GAIN_NUM];
    int m_pCnrSig_uvRatio[GAIN_NUM];
    int m_pCnrAback_alphaRatio[GAIN_NUM];
    int m_pTnrYStrengthQ6Ratio[GAIN_NUM];
    int m_pTnrUVStrengthQ6Ratio[GAIN_NUM];
    int m_pStrongSig_gainRatio[GAIN_NUM];
    int m_pStrongAback_alphaRatio[GAIN_NUM];

} _cpp_3dnr_fw_t;

#define SEGNUM 9
typedef struct {
    int m_nWrapEnable;
    int m_n4x4BlockEnable;
    int m_nFalseMatchEnable;
    int m_nLastRansacEnable;
    int m_nKeyPointMode;
    int m_nBlockOverlap;
    int m_nKeyPointPercent[SEGNUM];
    int m_nKeyPointThreshold[SEGNUM];
    int m_nKeyPointDistanceThreshold;
    int m_nBlockMaxDesCount;
    int m_nMaxMatchNumber;
    int m_nInnerPointThreshold;
    int m_nFalseMatchThreshold[SEGNUM];
} _nightmode_param_t;

typedef struct {
    int revisionNumber;
} cpp_firmware_version_s;

typedef struct {
    cpp_firmware_version_s version;
    _cpp_3dnr_fw_t params_3dnr;
} cpp_tuning_params_t;
#endif
