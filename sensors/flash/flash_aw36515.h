/******************************************************************************
 * (C) Copyright 2023 Spacemit International Ltd.
 * All Rights Reserved
 ******************************************************************************/
/*************** struct CCFlashTuningData
*************************************************************************************** char name[64];    //name for
this flash tuning data int flashType;    //0:signal flash, 1:dual flash, 2,qual flash int torch_index;  //torch led
index, typically, we set white led index.
    //flash
    int flashDataNum; //number of flash, should consider flashType.
    {//[4]
        {//[10] pre-flash
            int index;       //index of table
            int current;     //flash work at xx mA
            int luminance;   //spm defined lux at xx mA
            int code;        //value which is set to driver
            {
                int RGain;      //flash R gain
                int GGain;      //flash G gain
                int BGain;      //flash B gain
            }
            char ccmEnable;      //ccm enable
            int32_t ccm[3][3];  //ccm data
            char lscEnable;      //lsc enable
            int32_t lsc[768];    //lsc data
        }
        ...
        int mainflashNum;    //number of main-flash table
        {//[20] main-flash
            int index;       //index of table
            int current;     //flash work at xx mA
            int luminance;   //spm defined lux at xx mA
            int code;        //value which is set to driver
            {
                int RGain;      //flash R gain
                int GGain;      //flash G gain
                int BGain;      //flash B gain
            }
            char ccmEnable;     //ccm enable
            int32_t ccm[3][3]; //ccm data
            char lscEnable;     //lsc enable
            int32_t lsc[768];   //lsc data
        }
                ...
        int maxPreCurrent;       //max pre-flash current for this led
        int maxMainCurrent;  //max main-flash current for this led
        int default_code;    //code when power off state
    }
    ...
    //sensor
    int32_t luxThreshold;  // lux threshold, use for auto flash detection
    int32_t targetY;  // targetmeanY
*********************************************************************************************************************************/
{
    "aw36515",  // name for this flash tuning data
        0,      // 0:signal flash, 1:dual flash, 2,qual flash
        0,      // torch led index, means which led is used for torch mode.
        // flash
        1,  // number of flash, should consider flashType.
        {
            {
                1,  // number of pre-flash table
                {
                    {
                        0,    // index of table, pre-flash must be 0 always
                        40,   // flash work at xx mA
                        892,  // spm defined lux at xx mA
                        20,    // value which is set to driver
                        {
                            148,  // flash R gain
                            128,  // flash G gain
                            363,  // flash B gain
                        },
                        0,   // ccm enable
                        {},  // ccm data
                        0,   // lsc enable
                        {},  // lsc data
                    },
                },
                1,  // number of main-flash table (5000K)
                {
                    {
                        0,     // index of table
                        630,   // flash work at xx mA
                        1673,  // spm defined lux at xx mA
                        80,     // value which is set to driver
                        {
                            8133,  // flash R gain
                            4096,  // flash G gain
                            7032,  // flash B gain
                        },
                        1,  // ccm enable
                        {
                            // ccm data
                            {7268, -2314, -858},
                            {-591, 5278, -591},
                            {-139, -2226, 6461},
                        },
                        1,  // lsc enable
                        {   // lsc data
                         117, 123, 93,  87,  79,  76,  75,  79,  85,  93,  116, 106, 116, 87,  75,  64,  56,  52,  52,
                         55,  63,  73,  85,  112, 86,  70,  55,  45,  40,  37,  37,  40,  45,  54,  68,  82,  74,  53,
                         41,  34,  29,  26,  26,  29,  34,  41,  52,  71,  59,  42,  32,  25,  19,  17,  16,  19,  25,
                         32,  42,  55,  49,  35,  25,  17,  11,  8,   8,   11,  17,  25,  35,  49,  43,  30,  20,  12,
                         6,   3,   3,   6,   12,  20,  31,  43,  42,  28,  18,  9,   3,   1,   0,   3,   9,   18,  29,
                         41,  41,  29,  18,  9,   3,   1,   1,   3,   9,   18,  29,  41,  44,  31,  21,  12,  6,   3,
                         3,   6,   12,  21,  31,  44,  51,  36,  26,  18,  11,  8,   8,   11,  17,  26,  36,  49,  60,
                         44,  34,  26,  20,  17,  17,  20,  26,  34,  43,  57,  79,  55,  44,  35,  30,  28,  28,  30,
                         35,  42,  54,  75,  90,  75,  59,  48,  42,  39,  39,  42,  47,  57,  72,  88,  128, 95,  81,
                         68,  60,  56,  56,  59,  66,  78,  91,  128, 128, 128, 109, 97,  89,  84,  84,  88,  96,  104,
                         128, 128, 136, 115, 124, 121, 122, 119, 121, 120, 122, 120, 124, 110, 117, 120, 119, 120, 120,
                         120, 120, 120, 119, 120, 118, 126, 120, 119, 119, 120, 121, 121, 121, 120, 120, 119, 119, 119,
                         120, 119, 119, 120, 121, 122, 121, 121, 120, 119, 118, 121, 119, 119, 120, 121, 123, 123, 123,
                         122, 121, 120, 118, 122, 120, 119, 122, 122, 125, 126, 125, 124, 122, 121, 120, 123, 120, 121,
                         122, 124, 128, 127, 128, 126, 124, 122, 121, 122, 119, 121, 122, 126, 128, 129, 128, 127, 126,
                         123, 121, 125, 120, 121, 122, 126, 128, 129, 129, 128, 126, 123, 121, 124, 120, 121, 122, 125,
                         127, 129, 127, 127, 125, 123, 122, 121, 121, 120, 121, 123, 125, 126, 125, 125, 123, 122, 120,
                         125, 123, 118, 121, 121, 123, 124, 123, 123, 122, 120, 120, 121, 120, 118, 120, 120, 122, 122,
                         122, 122, 121, 120, 118, 122, 118, 119, 120, 120, 121, 121, 121, 120, 120, 119, 119, 122, 116,
                         122, 120, 120, 120, 120, 120, 120, 120, 119, 119, 121, 129, 119, 122, 123, 122, 123, 122, 122,
                         122, 123, 122, 130, 133, 125, 126, 122, 125, 121, 124, 124, 128, 122, 126, 132, 124, 124, 124,
                         124, 123, 123, 124, 124, 123, 125, 125, 124, 126, 124, 123, 123, 123, 123, 123, 123, 124, 123,
                         125, 123, 123, 123, 122, 123, 124, 123, 123, 123, 123, 123, 124, 125, 123, 122, 123, 123, 124,
                         124, 125, 124, 124, 123, 123, 124, 122, 122, 123, 124, 124, 126, 126, 125, 124, 124, 123, 123,
                         120, 122, 123, 124, 125, 127, 127, 126, 125, 124, 123, 124, 122, 122, 123, 125, 126, 128, 129,
                         127, 126, 124, 122, 122, 119, 122, 123, 124, 126, 128, 129, 127, 126, 124, 123, 122, 121, 121,
                         122, 123, 124, 126, 127, 126, 125, 123, 122, 122, 121, 122, 121, 122, 123, 126, 126, 125, 124,
                         124, 123, 122, 122, 121, 122, 121, 124, 124, 125, 125, 124, 123, 122, 124, 122, 122, 121, 123,
                         122, 124, 124, 124, 124, 124, 123, 122, 122, 123, 123, 123, 124, 124, 123, 124, 125, 124, 125,
                         124, 128, 123, 124, 124, 124, 124, 124, 124, 126, 125, 125, 125, 125, 125, 123, 124, 124, 123,
                         124, 126, 123, 122, 127, 124},
                    },
                },
                350,   // max pre-flash current for this led
                1000,  // max main-flash current for this led
                0xff,  // code when power off state
                16.0,  // float vsPreflashRatio
            },
        },
        // sensor
        {
            10,   // lux threshold, use for auto flash detection
            40,    // targetmeanY
        },
}
