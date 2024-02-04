/*
 * Copyright (C) 2019 ASR Micro Limited
 * All Rights Reserved
 */
#ifndef _FWDFF
#define _FWDFF

#define ISP_AE_ROUTE_MAX_NODES (16)
#define MAX_ROI_NUM            16  // AWB MAX ROI NUM
#define LUM_LEVEL_NUM          11  // AWB LUM LEVEL NUM
enum WorkMode { SinglePipe, DualPipe, HDR, RGBW, RGBIR, PreviewCapture, Unspecified };
enum AEStrategyMode { scene_auto, highlight_prior, lowlight_prior };
enum ISPModuleID {
    isp_ae,
    isp_af,
    isp_bpc,
    isp_ccm,
    isp_demosaic,
    isp_curve,
    isp_pdc,
    isp_pdf,
    isp_pdaf_soft,
    isp_eis,
    isp_ee,
    isp_bc,
    isp_formatter,
    isp_lsc,
    isp_ltm,
    isp_raw_dns,
    isp_rgb2yuv,
    isp_digi_gain,
    isp_sde,
    isp_stretch,
    isp_top,
    isp_awb,
    isp_ct_calc,
    isp_uvdns,
    isp_ae_soft,
    isp_af_soft,
    isp_awb_soft,
    isp_wbc
};
enum AESceneMode { ae_normal, ae_face };
/* define ISP control parameter structures */

typedef struct {
    int m_bEnable;
    int m_nAEStatMode;
    int m_bZSLHDRCapture;
    int m_nInitExpTime;       // unit:us, sensor initial status
    int m_nInitAnaGain;       // Q8 format, sensor initial status
    int m_nInitSnsTotalGain;  // Q8 format, sensor initial status
    int m_nInitTotalGain;     // Q8 format, sensor initial status
    int m_nStableTolerance;
    int m_nStableToleranceExternal;
    int m_bAutoCalculateAEMWindow;
    int m_nPreEndingPercentage;
    int m_bDRCGainSyncOption;
    int m_pSceneChangeSADThr[12];
    int m_pSubROIPermil[6][4];
    int m_nSubROIScaleFactor;
    int m_nFaceLumaOption;
    int m_nFaceDetFrameID;
} _isp_fw_ae_t;

typedef struct {
    int r;
    int g;
    int b;
} wb_gain;

typedef struct {
    int m_bEnable;
    int m_bSyncWB;
    int m_bAutoWindow;
    int m_nMode;
    int m_nInitMode;
    wb_gain m_pManualGain[8];
    int m_nAWBStableRange;
    int m_nAWBStableFrameNum;
    int m_nAWBUnStableRange;
    int m_nAWBUnStableFrameNum;
    int m_nAWBStep1;
    int m_nAWBStep2;
    int m_nLowThr;
    int m_nHighThr;
} _isp_fw_wb_t;

typedef struct {
    int m_bEnable;
    int m_nAFStatMode;
    int m_nWinStartXPermil;
    int m_nWinStartYPermil;
    int m_nWinEndXPermil;
    int m_nWinEndYPermil;
    int m_nMinWidthPermil;
    int m_nMinHeightPermil;
    int m_bConfigDone;
} _isp_fw_afm_t;

typedef struct {
    int m_bEnable;
    int m_bManualMode;
    int m_nCrossChnStrengthManual;
    int m_pSlopeGManual[9];
    int m_pInterceptGManual[9];
    int m_pSlopeRBManual[9];
    int m_pInterceptRBManual[9];
    int m_pSlopeCornerManual[9];
    int m_pInterceptCornerManual[9];
    int m_pBpcGainIndex[12];
    int m_pCrossChnStrength[12];
    int m_pSlopeG[9][12];
    int m_pInterceptG[9][12];
    int m_pSlopeRB[9][12];
    int m_pInterceptRB[9][12];
    int m_pSlopeCorner[9][12];
    int m_pInterceptCorner[9][12];

    int m_bDeadEnable;
    int m_bSpikeEnable;
    int m_bSameChnNum;
    int m_bGrGbNum;
    int m_nDeltaThr;
    int m_nRingGRatio;
    int m_nRingMeanRatio;
    int m_nEdgeTimes;
    int m_nNearThr;
    int m_pSegG[8];
    int m_nDeadRatioG;
    int m_nSpikeRatioG;
    int m_nDeadRatioRB;
    int m_nSpikeRatioRB;
    int m_nMinThrEn;
    int m_nMaxThrEn;
    int m_bAroundDetEn;
    int m_bCornerDetEn;
    int m_bBlockDetEn;
    int m_bEdgeDetEn;

} _isp_fw_bpc_t;

typedef struct {
    int m_bEnable;
    int m_bManualMode;
    int m_bColorFringleRemoveEnable;
    int m_bDisgardHFEnable;
    int m_nHueTransShiftNum;
    int m_nHighFreqThreshold;
    int m_nHighFreqTransShiftNum;
    int m_nColorFringRemovalStrength;
    int m_pColorFringeHueRange[2];
    int m_bUseCorrelatedCT;
    int m_pColorTemperatureIndex[4];
    int m_pCMC0[3][3];
    int m_pCMC1[3][3];
    int m_pCMC2[3][3];
    int m_pCMCManual[3][3];
    int m_pCMCSaturationList[12];
} _isp_fw_cmc_t;

typedef struct {
    int m_bManualMode;
    int m_bIfEdgeGenerate;
    int m_bIfGbGrRebalance;
    int m_bIfDNS;
    int m_nInterpOffsetManual;
    int m_nInterpOffsetHVManual;
    int m_nNoiseSTDManual;
    int m_nLowpassGLevelManual;
    int m_nGbGrThrManual;
    int m_nSharpenStrengthManual;
    int m_nShpThresholdManual;
    int m_nDenoiseThresholdManual;
    int m_nNoiseAddbackLevelManual;
    int m_nDenoiseLumaStrengthManual[3];
    int m_nChromaNoiseThresholdManual;
    int m_pUSMFilterManual[3];
    int m_pInterpOffset[12];
    int m_pInterpOffsetHV[12];
    int m_pNoiseSTD[12];
    int m_pLowpassGLevel[12];
    int m_pGbGrThr[12];
    int m_pSharpenStrength[12];
    int m_pShpThreshold[12];
    int m_pDenoiseThreshold[12];
    int m_pNoiseAddbackLevel[12];
    int m_pDenoiseLumaStrength[12][3];
    int m_pChromaNoiseThreshold[12];
    int m_pUSMFilter[12][3];
    int m_pHFSegShiftIndex[12];
    int m_pHFSegGainIndex[13];
} _isp_fw_demosaic_t;

typedef struct {
    int m_bEnable;
    int m_bOut;
    int m_nHOft;
    int m_nVOft;
    int m_bLRAdjust;
    int m_bTBAdjust;
    int m_nFullWidth;
    int m_nFullHeight;
    int m_nWindowMode;
    int m_nWindowScaleFactor;
    int m_nMinWidthPermil;
    int m_nMinHeightPermil;
    int m_pPDFirstX[2];
    int m_pPDFirstY[2];
    int m_pRatioA[4];
    int m_pPixelMask[32][32];
    int m_pPixelTypeMask[32][32];
    int m_pRatioBMap[36][9];
    int m_bSoftCompEnable;
} _isp_fw_pdc_t;

typedef struct {
    int m_bEnable;
    int m_nExtPRm;
    int m_nWA;
    int m_nWB;
    int m_nFullWidth;
    int m_nFullHeight;
    int m_nHOft;
    int m_nVOft;
    int m_bRefRB;
    int m_bRefCnr;
    int m_nRefNoiseL;
    int m_nExtPThre;
    int m_nExtPSft;
    int m_nExtPOpt;
    int m_pPDFirstX[2];
    int m_pPDFirstY[2];
    int m_pPixelMask[32][32];
} _isp_fw_pdf_t;

typedef struct {
    int m_bMirrorShift;
    int m_bShiftLutEn;
    int m_pShiftLut[129];
    int m_pLumThre[11];
    int m_pGainThre[11];
    int m_nErrorDistWeight;
    int m_nErrorDistCoef;
    int m_nErrorShpCoef;
    int m_nErrorThre1;
    int m_nErrorThre2;
    int m_pSwingThre[11];
    int m_bConfAdjust;
    int m_nConfOff;
    int m_nConfLimit;
    int m_nSearchRange;
} _pdaf_alg_param_t;

typedef struct {
    int m_bEnable;
    int m_bEbGtmAfterLinearcurve;
    int m_nCurveSelectOption;
    int m_nBacklightStrengthManual;
    int m_nContrastStrengthManual;
    int m_nBrightnessStrengthManual;
    int m_nAlpha;
    int m_pBacklightcurveManual[129];
    int m_pContrastsCurveManual[129];
    int m_pGainIndex[3];
    int m_pGTMcurve0[129];
    int m_pGTMcurve1[129];
    int m_pGTMcurve2[129];
} _isp_fw_gtm_after_t;

typedef struct {
    int m_bEnable;
    int m_bUseOTP;
    int m_bManualMode;
    int m_bAutoScale;
    int m_bEnhanceEnable;
    int m_bLSCCSCEnable;
    int m_bAdjustCSCTblMinEnable;
    int m_nProfileSelectOption;
    int m_nLSCStrengthManual;
    int m_nFOVCropRatioH;
    int m_nFOVCropRatioV;
    int m_nDifThr;
    int m_nDifThrMinPerc;
    int m_nAngleThr;
    int m_nDifThrVMF;
    int m_nAngleThrVMF;
    int m_nGradThrMin;
    int m_nGradThrMax;
    int m_nGradMaxError;
    int m_nGradThrConv;
    int m_nGradMax;
    int m_nTblAlpha;
    int m_nCSCGlobalStrength;
    int m_nEffPNumAll;
    int m_nEffPNumHalf;
    int m_nEffPNumQuarter;
    int m_pEffNumRing[3];
    int m_pCSCCTIndex[2];
    int m_pCSCLuxIndex[2];
    int m_pLSCStrength[12];
    int m_bUseCorrelatedCT;
    int m_pCTIndex[4];
    int m_pLSCProfile[3][576];
    int m_pLSCProfileManual[576];
    int m_pOTPProfile[576];
} _isp_fw_lsc_t;

typedef struct {
    int m_bEnable;
    int m_bMergeEnable;
    int m_bLocalizedEnable;
    int m_bSpacialEnable;
    int m_bSpacialAddbackEnable;
    int m_nSpacialOffCenterPercentage;
    int m_pMaxSpacialDenoiseThreGain[12];
    int m_bManualMode;
    int m_nSigmaManual;
    int m_nGNRStrengthManual;
    int m_nRBNRStrengthManual;
    int m_nL0Manual;
    int m_nL1Manual;
    int m_nL2Manual;
    int m_nL3Manual;
    int m_pSigma[12];
    int m_pGNRStrength[12];
    int m_pRBNRStrength[12];
    int m_pL0[12];
    int m_pL1[12];
    int m_pL2[12];
    int m_pL3[12];
} _isp_fw_raw_denoise_t;

typedef struct {
    int m_nOutputColorSpace;
    int m_bManualMode;
    int m_nGlobalSaturation;
    int m_nSaturationManual;
    int m_pSaturationCP[12];
} _isp_fw_rgb2yuv_t;

typedef struct {
    int m_bEnable;
    int m_nISPGlobalOffsetValue12bit;
    int m_bManualMode;
    int m_pGlobalBlackValueManual[4];
    int m_pGlobalBlackValueManualCapture[4];
    int m_pGlobalBlackValue[12][4];
    int m_pGlobalBlackValueCapture[12][4];
} _isp_fw_digital_gain_t;

typedef struct {
    int m_nCFAPattern;
    int m_bAELinkZoom;
    int m_bAFLinkZoom;
    int m_bAWBLinkZoom;
    int m_nPreviewZoomRatio;
    int m_nAEProcessPosition;
    int m_nAEProcessFrameNum;
    int m_bHighQualityPreviewZoomEnable;
} _isp_fw_top_t;

typedef struct {
    int m_bManualMode;
    int m_bZoneEb;
    int m_pRyTable[3];
    int m_nSyTable;
    int m_pRuvTable[4];
    int m_pSuvTable[2];
    int m_pMargin[6];
    int m_nManualGainWeight;
    int m_pGainWeight[11];
    int m_pYTable[2];
    int m_pHTable[2];
    int m_pSTable[2];
} _sde_zones;

typedef struct {
    int m_bEnable;
    _sde_zones m_pZones[6];
    int m_pExpLut[11];
} _isp_fw_sde_t;

typedef struct {
    int m_nTGain;     // Q8
    int m_nAGain;     // Q8
    int m_nSnsDGain;  // Q12
    int m_nISPDGain;  // Q12
    int m_nExpLine;   // in us
    int m_nExpIndex;
    int m_nPirisGain;          // Q8
    int m_pWBGainInternal[4];  // Q12
    int m_pWBGainApply[4];     // Q12
    int m_nCT;
    int m_nHDRRatio;
    int m_pEISOffset[2];
    int m_nCFAPattern;
    int m_nBLC12b;
    int m_nColorSpace;
    int m_nSwingOption;
    int m_bWriteSensorFlag;
    int m_bWriteISPFlag;
    int m_nDRCGain;
    int m_nDRCGainDark;
    int m_nDRCGainDarkReal;
    int m_pWBGoldenSignature[2];
    int m_pWBModuleSignature[2];
    int m_nLTMStrength;
    int m_pLTMCurve[48][64];
    int m_nLSCBoostFlag;
    int m_nLSCQuality;
    int m_pLSCProfile[576];
    int m_pLSCCSCProfile[384];
    int m_pCCM[3][3];
    int m_nPreviewZoomRatioInput;
    int m_nPreviewZoomRatioApply;
    int m_nBinningZoomRatioApply;
    int m_pExpLineCapture[3];   //[0]:normal exposure; [1]:long exposure; [2]:short exposure
    int m_pAGainCapture[3];     //[0]:normal again; [1]:long again; [2]:short again
    int m_pSnsDGainCapture[3];  //[0]:normal sensor dgain; [1]:long sensor dgain; [2]:short sensor dgain
    int m_pTGainCapture[3];     //[0]:normal total gain; [1]:long total gain; [2]:short total gain
    int m_nFVValue;
    int m_nAEMode;        // 0-auto,1-manual,2-lock
    int m_nAEStableFlag;  // 0 - unstable, 1 - stable
    int m_nAWBStableFlag;
    int m_nAFMode;         // 0-saf,1-caf,2-manual,3-lock_caf
    int m_nAFStatus;       // 0-idle,1-busy,2-reached,3-failed
    int m_nMotorPosition;  // inital motor position/current position
    int m_nAWBMode;
    int m_nValidInputHeightAssociated;
    int m_nSceneLuma;
    int m_nCorrelationCT;
    int m_nTint;
    int m_nSceneLux;
    int m_nGlobalSaturation;
    int m_pHist[256];
} _isp_fw_frameinfo_t;

typedef struct {
    int m_bEnable;
    int m_bManualMode;
    int m_pNoiseH[2];
    int m_nNoiseV;
    int m_nDarkRatio;
    int m_pWeightP[2];
    int m_pWeightY[2];
    int m_pWeightC[2];

    int m_pLumaH0Manual[9];
    int m_pLumaH1Manual[9];
    int m_pRegionManual[2];
    int m_nDCWeightHManual;
    int m_nLumRatioThManual;
    int m_nLumRatioRgManual;
    int m_pLumaVManual[9];
    int m_nUVRatioVManual;
    int m_nDCWeightVManual;
    int m_nPreQManual;
    int m_nVstrengthManual;

    int m_pLumaH0[9][12];
    int m_pLumaH1[9][12];
    int m_pRegion[2][12];
    int m_nDCWeightH[12];
    int m_nLumRatioTh[12];
    int m_nLumRatioRg[12];
    int m_pLumaV[9][12];
    int m_nUVRatioV[12];
    int m_nDCWeightV[12];
    int m_nPreQ[12];
    int m_nVstrength[12];
} _isp_fw_uvdenoise_t;

typedef struct {
    int m_bEnable;
    int m_bHistEnable;
    int m_nOffsetX;
    int m_nOffsetY;
    int m_nLtmStrength;
    int m_nCurveAlpha;
    int m_nSlopeThr;
    int m_nPhicBeta;
    int m_nBlendCurveFP;
    int m_nBlendCurveSP;
    int m_nSubDarkPercThrLow;
    int m_nSubDarkPercThrHigh;
    int m_nSubDarkAdjMeanThr;
    int m_nDarkPercThrLow;
    int m_nDarkPercThrHigh;
    int m_nPhCDarkMaxExtraRatio;
    int m_pDstAlphaGainIndex[12];
    int m_pDstAlphaIndex[12];
} _isp_fw_ltm_t;

typedef struct {
    int m_bEnable;
    int m_bCalcEnable;
    int m_bFilterEnable;
    int m_nRangeX;
    int m_nRangeY;
    int m_nMarginX;
    int m_nMarginY;
    int m_pFilterWeight[32];
    int m_pPeakConfLevel[12];
    int m_pPeakErrorThre[12];
    int m_bReverseDirectionX;
    int m_bReverseDirectionY;
    int m_nCenterStatRatio;
} _isp_fw_eis_t;

typedef struct {
    int m_nSwingOption;
    int m_bConvertDitheringEnable;
    int m_bCompressDitheringEnable;
} _isp_fw_formatter_t;

typedef struct {
    int m_bEnable;
    int m_pCurveLumaP[17];
    int m_pCurveLumaN[17];
    int m_pCurveFreqTxP[17];
    int m_pCurveFreqTxN[17];
    int m_nNodeFreq;

    // auto mode
    int m_pGlobal_SharpenStrengthP[12];
    int m_pGlobal_SharpenStrengthN[12];
    int m_pGlobal_SharpenStrengthPCapture[12];
    int m_pGlobal_SharpenStrengthNCapture[12];
    int m_pLPF1[4][2];
    int m_pLPF2[4][2];
    int m_pLPF1Capture[4][2];
    int m_pLPF2Capture[4][2];
    int m_pFreqExpLevel1[12];
    int m_pFreqExpLevel2[12];
    int m_pFreqOffsetTx[12];
    int m_pTxClipP[12];
    int m_pTxClipN[12];
    int m_pTxThrdP[12];
    int m_pTxThrdN[12];
    int m_pClipPos[12];
    int m_pClipNeg[12];
    int m_pHCStrength[12];
    int m_pCoffW[2][12];

    // manual mode
    int m_bManualMode;
    int m_nGlobal_SharpenStrengthP_manual;
    int m_nGlobal_SharpenStrengthN_manual;
    int m_pLPF1_manual[2];
    int m_pLPF2_manual[2];
    int m_nFreqExpLevel1_manual;
    int m_nFreqExpLevel2_manual;
    int m_nFreqOffsetTx_manual;
    int m_nTxClipP_manual;
    int m_nTxClipN_manual;
    int m_nTxThrdP_manual;
    int m_nTxThrdN_manual;
    int m_nClipPos_manual;
    int m_nClipNeg_manual;
    int m_nHCStrength_manual;
    int m_pCoffW_manual[2];
    int m_pGainIndex[4];
} _isp_ee_lite_fw_param_t;
/* end define ISP control parameter structures */

/* define AE parameter structures */

typedef struct {
    int m_nWorkMode;
    int m_nPirisGain;
    int m_pExpTime[2];       // unit:us, sensor initial status
    int m_pAnaGain[2];       // Q8 format, sensor initial status
    int m_pSnsTotalGain[2];  // Q8 format, sensor initial status
    int m_pTotalGain[2];     // Q8 format, sensor initial status
} _isp_ae_param_t;

typedef struct {
    int m_pHist[1024];
    float m_pSubRoiAvg[6][3];
    float m_pBlockAvg[12][16][3];
} _isp_ae_stat_1_t;

typedef struct {
    _isp_ae_stat_1_t pstAeStat1[2];
} _isp_ae_info_t;

typedef struct {
    int m_pExpTime[2];    // unit us
    int m_pAnaGain[2];    // Q8 format
    int m_pSnsDGain[2];   // Q12 format
    int m_pTotalGain[2];  // Q8 format
    int m_nPirisGain;
    int m_nDRCGain;
    int m_nDRCGainDark;
    int m_nDRCGainDarkReal;
} _isp_ae_result_t;

typedef struct {
    int m_pMinExpTime[2];
    int m_pMaxExpTime[2];
} ae_ctrl_cmd_hdrlimit;

typedef enum {
    AE_WORK_MODE_SET = 8000,        // pValue = WorkMode*
    AE_FPS_BASED_EXPOSURETIME_SET,  // pValue = int*
    AE_HDR_EXPOSURETIME_LIMIT_SET,  // pValue = ae_ctrl_cmd_hdrlimit*
    AE_ACUTAL_SNS_GAIN_SET,         // pValue = long long*, [63~32]:sensor digital gain, [31~0]:sensor analog gain
    AE_SCENE_MODE_SET,              // pValue = scene mode
    ISP_CTRL_CMD_BUTT,
} _isp_ae_ctrl_cmd_e;

typedef struct {
    int (*pfn_ae_init)(void *handle, int PipeID, const _isp_ae_param_t *pAeParam);
    int (*pfn_ae_run)(void *handle, int PipeID, const _isp_ae_info_t *pAeInfo, _isp_ae_result_t *pAeResult);
    int (*pfn_ae_ctrl)(void *handle, int PipeID, unsigned int cmd, void *pValue);
    int (*pfn_ae_exit)(void *handle, int PipeID);
} _isp_ae_callback_t;

typedef struct {
    int m_nExpTime;
    int m_nTotalGain;
    int m_nPirisGain;
} _isp_ae_route_node_t;

typedef struct {
    int m_bManualAEEnable;
    int m_bAdvancedAECEnable;
    int m_bAntiFlickerEnable;
    int m_bLumaCalcMode;
    int m_bLumaSelectOption;
    int m_bHDRRatioMode;
    int m_bHDRProcessOption;
    int m_bHDRCapRatioMode;
    int m_nSensitivityRatio;
    int m_nCompensation;
    int m_nSmoothness;
    int m_nAntiFlickerFreq;
    int m_nStrategyMode;
    int m_nMinHDRRatio;
    int m_nMaxHDRRatio;
    int m_nMidOffsetRatio;
    int m_nHDRRatioManual;
    int m_nHDRCapSatRefBin;
    int m_pHDRCapSatRefPerThr[2];
    float m_pHDRCapRatioManual[2];
    float m_pHDRCapRatioShortAuto[2];
    _isp_ae_route_node_t m_pRouteNode50Hz[ISP_AE_ROUTE_MAX_NODES];
    _isp_ae_route_node_t m_pRouteNode60Hz[ISP_AE_ROUTE_MAX_NODES];
    int m_pTolerance[2];
    int m_pStableRange[2];
    int m_pSatRefBin[2];
    int m_pSatRefPerThr[2][2];
    int m_pStableRefFrame[2];
    int m_pUnStableRefFrame[2];
    int m_pMinExpTime[2];    // unit is us; [0]:long frame, [1]:short frame
    int m_pMaxExpTime[2];    // unit is us
    int m_pMinAnaGain[2];    // Q8 format
    int m_pMaxAnaGain[2];    // Q8 format
    int m_pMinSnsDGain[2];   // Q12 format
    int m_pMaxSnsDGain[2];   // Q12 format
    int m_pMinTotalGain[2];  // Q8 format
    int m_pMaxTotalGain[2];  // Q8 format
    int m_pExpTimeManual[2];
    int m_pAnaGainManual[2];
    int m_pSnsDGainManual[2];
    int m_pTotalGainManual[2];
    unsigned int m_pExpIndexManual[2];
    int m_pExpIndexThre[4];
    int m_pLumaBlockWeight[4][3];
    int m_pLumaBlockThre[4];
    int m_pLuma7ZoneWeight[7];
    int m_pTargetRange[2];
    int m_pTargetDeclineRatio[16];
    int m_pMeteringMatrix[12][16];
    int m_nDualTargetBlendWeight;
    int m_pAjustSplitFrameNum[2];
    int m_pSingleStepAdjustLumaThr[2];
    int m_pFaceAjustSplitFrameNum[2];
    int m_pFaceSingleStepAdjustLumaThr[2];
    int m_pFastStep[2];
    int m_pFastStepRangePer[2];
    int m_pFastStepMinRange[2];
    int m_pSlowStep[2];
    int m_pSlowStepRangePer[2];
    int m_pSlowStepMinRange[2];
    int m_pFineStep[2];
    int m_pMaxFastRatio[2];
    int m_pMaxSlowRatio[2];
    int m_pMinFineRatio[2];
    int m_pMaxFineRatio[2];
    int m_bLumaPredict;
    int m_nDarkRefBin;
    int m_nDarkPerThrL;
    int m_nDarkPerThrH;
    int m_nDarkTarget;
    int m_nMaxDRCGain;      // Q4
    int m_nMaxDRCGainDark;  // Q4
    int m_bQuickResponseEnable;
    int m_nCalibExposureIndex;
    int m_nCalibSceneLuma;
    int m_nCalibSceneLux;
} _isp_ae_attr_t;

/* end define AE parameter structures */

/* define AWB parameter structures */
typedef struct {
    int m_nWorkMode;
} _isp_awb_param_t;

typedef struct {
    int r;
    int g;
    int b;
} _isp_awb_stat_blk_t;

typedef struct {
    _isp_awb_stat_blk_t blk[24][32];
    int CurrentLux;
} _isp_awb_stat_frame_t;

typedef struct {
    _isp_awb_stat_frame_t pstAwbStatSingle[2];
} _isp_awb_info_t;

typedef struct {
    int r_gain;
    int g_gain;
    int b_gain;
} _isp_awb_result_t;

typedef struct {
    int (*pfn_awb_init)(void *handle, int PipeID, const _isp_awb_param_t *pAwbParam);
    int (*pfn_awb_run)(void *handle, int PipeID, const _isp_awb_info_t *pAwbInfo, _isp_awb_result_t *pAwbResult);
    int (*pfn_awb_ctrl)(void *handle, int PipeID, unsigned int cmd, void *pValue);
    int (*pfn_awb_exit)(void *handle, int PipeID);
} _isp_awb_callback_t;

typedef struct {
    int x_high;
    int x_low;
    int y_high;
    int y_low;
} awb_roi_ct_bound;

#ifndef AWB_ATTR
#define AWB_ATTR
typedef struct {
    int m_bGrayWorldEn;
    awb_roi_ct_bound m_sRoiBound[MAX_ROI_NUM];
    awb_roi_ct_bound m_sRoiBoundG;
    int m_pRoiLumLutHigh[MAX_ROI_NUM][LUM_LEVEL_NUM];
    int m_pRoiLumLutLow[MAX_ROI_NUM][LUM_LEVEL_NUM];
    int m_pRoiLumLutHighG[LUM_LEVEL_NUM];
    int m_pRoiLumLutLowG[LUM_LEVEL_NUM];
    int m_pRoiEn[MAX_ROI_NUM];
    int m_pRoiWeightLut[MAX_ROI_NUM][LUM_LEVEL_NUM];  // 0~255
    int m_pRoiLumThre[LUM_LEVEL_NUM];
    int m_bRoiLimitManual;
    int m_nRoiCtHigh;
    int m_nRoiCtLow;
    int m_nRoiXMax;
    int m_nRoiXMin;
    int m_pRoiCtHighAuto[LUM_LEVEL_NUM];
    int m_pRoiCtLowAuto[LUM_LEVEL_NUM];
    int m_pRoiXMaxAuto[LUM_LEVEL_NUM];
    int m_pRoiXMinAuto[LUM_LEVEL_NUM];
    int m_pAWBGainLimit[4];
    int m_nAWBCTShift;
    int m_pAWBCTShiftThr[2];
    int m_pLowCtThr[8];
    int m_pLowCtNumThr[2];
    int m_pLowCtProtectRatio[7];  // 0~256, 256 means no protection
    awb_roi_ct_bound m_sRoiBoundDayLight;
    int m_pDayLightNumThr[2];
    int m_bGreenShiftEn;
    int m_pGreenShiftWeight[LUM_LEVEL_NUM];  // Q5, 0~32
    int m_nGreenShiftMax;                    // Q5, 0~32
    int m_pGreenNumThre[2];                  // 0~BLK_NUM_W*BLK_NUM_H
    _isp_awb_result_t m_sOutdoorGain;
    int m_nValidNum;
    int m_bWeightOnSum;  // weight on sum or gain
    int m_nLog2CwtOverA;
} _isp_awb_attr_t;
#endif
/* end define AWB parameter structures */

/* define AF parameter structures */
typedef enum {
    AE_LUMA_STABLE_FLAG_SET = 8000,  // pValue = ae stable flag
    AWB_STABLE_FLAG_SET,             // pValue = awb stable flag
    AF_CTRL_CMD_BUTT,
} _isp_af_ctrl_cmd_e;

typedef struct {
    unsigned int m_pCtrVector[12];
    unsigned int m_nAvg;
} _isp_afm_stat_unit_t;

typedef struct {
    _isp_afm_stat_unit_t afm_unit[25];
} _isp_afm_stat_frame_t;

typedef struct {
    int m_bLRAdjust;
    int m_bTBAdjust;
    int m_nBlkNumber;
    int m_nPDShift;
    int m_nPDConf;
    int m_nGainQ4;
    int m_nExpIndex;
    int m_nColorTemperature;
    int m_pAFMWindowPosition[4];
    int m_pPDAFWindowPosition[4];
    _isp_afm_stat_frame_t pstAfmStatSingle;
} _isp_af_info_t;

typedef struct {
    int m_nInitMotorPosition;
} _isp_af_param_t;

typedef struct {
    int m_nAFMode;
    int m_bHybridAFEnable;
    int m_bAFTrigger;
    int m_bMotorManualTrigger;
    int m_nManualMotorPosition;
    int m_nMinMotorPosition;
    int m_nMaxMotorPosition;
    int m_nFirstStepDirectionJudgeRatio;
    int m_bPreMoveMode;
    int m_bPreMoveToInfMode;
    int m_bStartFromTrueCurrent;
    int m_nBackTimeRatio;
    int m_nBackTimeDivisor;
    int m_nPreMoveTimeRatio;
    int m_nFrameRate;
    int m_nMotorResponseTime;
    int m_nMaxSkipFrame;
    int m_nSoftwareMotorCtrlMode;
    int m_nMinSafePosition;
    int m_nMinStepRatio;
    int m_nMaxSafeStep;
    int m_nMinSafePositionMacro;
    int m_nMinStepRatioMacro;
    int m_nMaxSafeStepMacro;
    int m_nMinMacroReverseStepNum;
    int m_nMinReverseStepNum;
    int m_nFineStepSearchNum;
    int m_bAdaptiveStep;
    int m_nCoarseStep;
    int m_nFineStep;
    int m_nVCMStep1Ratio;
    int m_nVCMStep2Ratio;
    int m_nVCMCoarseStep1;
    int m_nVCMCoarseStep2;
    int m_nVCMFineStep1;
    int m_nVCMFineStep2;
    int m_nFailMotorPositionOption;
    int m_nHyperFocalMotorPosition;
    int m_pFVDropPercentage[12][6];
    int m_pCurrentStartFVDropPercentage[12][6];
    int m_pFVFailPercentage[12][6];
    int m_pCurrentStartFVFailPercentage[12][6];
    int m_pLastStepChangePercentage[12][6];
    int m_pWindowWeightMatrix[5][5];
    int m_bPDReverseDirectionFlag;
    int m_nPDDirectConfThrRatio;
    int m_nPDTryStepRatio;
    int m_nPDCoarseStep;
    int m_nPDFineStep;
    int m_nPDStepDiscountRatio;
    int m_pPDConfThr[12];
    int m_pPDFVIncreaseRatio[6];
    int m_pPDFVDropPercentage[12][6];
    int m_pPDShiftPositionLUT[25][2][31];
    int m_bCAFForce;
    int m_bCAFHold;
    int m_nCAFHoldPDShiftThr;
    int m_nCAFHoldPDConfThr;
    int m_nPreFocusMode;
    int m_nPFPDStepDiscountRatio;
    int m_nLumaCalcOpt;
    int m_nFlatSceneVarThr;
    int m_nFlatSceneLumaThr;
    int m_bReFocusEnable;
    int m_pRefocusLumaSADThr[12];
    int m_bStableJudgeOpt;
    int m_nRefStableFrameNum;
    int m_nDetStableFrameNum;
    int m_pStableExpIndexPercentage[12];
    int m_pStableFVSADPercentage[12];
    int m_pStableLumaSADThr[12];
    int m_bChangeJudgeOpt;
    int m_nChangeFrameNum;
    int m_nChangeStatAreaPercentage;
    int m_nChangeStatCenterPercentage;
    int m_pChangeExpIndexPercentage[12];
    int m_pChangeFVSADPercentage[12];
    int m_pChangeLumaSADThr[12];
    int m_pChangePDShiftThr[12];
    int m_bForeGroundTrackInCoarse;
    int m_bForeGroundTrackInFine;
    int m_nForeGroundTrackWindow;
    int m_pFVBlkDropPercentage[12][6];
    int m_pFVBlkIncreasePercentage[12][6];
    int m_pFineStepStartPosSelectConf[12];
    int m_pFineStepEndPosSelectConf[12];
} _isp_af_attr_t;

typedef struct {
    int WinNum;
    int Contrast[25][6];
    int ContrastAvg[6];
    int Thumbnail[25][3];
    int LensPosition;
    int PDShift;
    int PDConf;
    int Status;  // 0-idle,1-busy,2-reached,3-failed
} _isp_af_result_t;

typedef struct {
    int (*pfn_af_init)(void *handle, int PipeID, const _isp_af_param_t *pAfParam);
    int (*pfn_af_run)(void *handle, int PipeID, const _isp_af_info_t *pAfInfo, _isp_af_result_t *pAfResult);
    int (*pfn_af_ctrl)(void *handle, int PipeID, unsigned int cmd, void *pValue);
    int (*pfn_af_exit)(void *handle, int PipeID);
} _isp_af_callback_t;

/* end define AF parameter structures */

typedef struct {
    int m_nIterateNumber;
    int m_nColorTemperatureLow;
    int m_nColorTemperatureHigh;
    float m_pCTMatrixLow[3][3];
    float m_pCTMatrixHigh[3][3];
} _isp_ct_calc_t;

#endif