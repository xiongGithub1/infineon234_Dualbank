/*
 * @ 锟斤拷锟斤拷: UDS_alg_hal.c
 * @ 锟斤拷锟斤拷:
 * @ 锟斤拷锟斤拷:
 * @ 锟斤拷锟斤拷: 2021锟斤拷2锟斤拷5锟斤拷
 * @ 锟芥本: V1.0
 * @ 锟斤拷史: V1.0 2021锟斤拷2锟斤拷5锟斤拷 Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#include <uds_timer.h>
#include <uds_alg.h>
#include "uds_app.h"
/* Random value, seed is 0x12345678 */
static uint32 uint32RandVal = 0x12345678U;

/* ============================================================
 *  Security Access Algorithm Masks
 *  Level 1: Enhanced Diagnosis Mask {0xA9, 0xC6, 0x13, 0x91}
 *  Level 2: Bootloader Mask         {0x99, 0xCD, 0x43, 0x95}
 * ============================================================ */
/* Level 1 Mask */
static const uint8 gs_aLevel1Mask[SA_ALGORITHM_SEED_LEN] =
{
    0xA9u, 0xC6u, 0x13u, 0x91u
};

static const uint8 gs_aLevel2Mask[SA_ALGORITHM_SEED_LEN] =
{
    0x99u, 0xCDu, 0x43u, 0x95u
};

static uint32 gs_UDS_SWTimerTickCnt;

/* ============================================================
 *  Security Access Level 1 Algorithm
 *  Key1[i] = Seed[i] ^ Mask[i]
 *  Key2[i] = (Seed[i] << 1) ^ Mask[i]
 *  Key[i]  = Key1[i] + Key2[i] (per-byte 8-bit, discard carry)
 * ============================================================ */
void UDS_ALG_HAL_ComputeKey_Level1(const uint8 *i_pSeed, uint8 *o_pKey)
{
    uint8 i;
    uint8 seed1;
    uint8 key1;
    uint8 key2;

    for (i = 0u; i < SA_ALGORITHM_SEED_LEN; i++)
    {
        seed1 = i_pSeed[i] << 1u;
        key1  = i_pSeed[i] ^ gs_aLevel1Mask[i];
        key2  = seed1 ^ gs_aLevel1Mask[i];
        o_pKey[i] = key1 + key2; /* 8-bit addition, overflow wraps (carry discarded) */
    }
}

/* ============================================================
 *  Security Access Level 2 Algorithm
 *  Tmp[i] = Seed[i] ^ Mask[i]
 *  Key[0] = Tmp[2]
 *  Key[1] = ((Tmp[0] & 0x0F) << 4) | ((Tmp[1] & 0xF0) >> 4)
 *  Key[2] = (Tmp[1] & 0xF0) | ((Tmp[3] & 0xF0) >> 4)
 *  Key[3] = ((Tmp[0] & 0x0F) << 4) | (Tmp[3] & 0x0F)
 * ============================================================ */
void UDS_ALG_HAL_ComputeKey_Level2(const uint8 *i_pSeed, uint8 *o_pKey)
{
    uint8 tmp[SA_ALGORITHM_SEED_LEN];
    uint8 i;

    for (i = 0u; i < SA_ALGORITHM_SEED_LEN; i++)
    {
        tmp[i] = i_pSeed[i] ^ gs_aLevel2Mask[i];
    }

    o_pKey[0] = (tmp[2] & 0x0Fu) | (tmp[2] & 0xF0u);
    o_pKey[1] = ((tmp[0] & 0x0Fu) << 4u) | ((tmp[1] & 0xF0u) >> 4u);
    o_pKey[2] = ((tmp[1] & 0xF0u)) | ((tmp[3] & 0xF0u) >> 4u);
    o_pKey[3] = ((tmp[0] & 0x0Fu) << 4u) | (tmp[3] & 0x0Fu);
}

/**
* @brief Sets the seed for the random numbers generator
* @details Writes the given seed into the random numbers generator
* @param[in] uint32Seed9 Seed to be used.
* @note Seed 0 would block the generator therefore if 0 is passed, 1 is used
*       as the seed.
* @note Default seed is 0x12345678.
*/
void fsl_srand(uint32 uint32Seed9)
{
    /* Value 0 is forbidden because it would block the LFSR */
    if (0U == uint32Seed9)
    {
        /* Forbidden value */
        uint32Seed9++; /* Correct the value */
    }

    uint32RandVal = uint32Seed9; /* Set the seed */
}

/**
* @brief Returns pseudo-random number
* @details Function generates pseudo-random number using the LFSR algorithm.
* @return Pseudo-random number in the interval from 0 to 0xFFFFFFFF
*/
uint32 fsl_rand(void)
{
    /* Generate the next value in the sequence */
    uint32RandVal = (uint32RandVal >> 1U) ^ ((0U - (uint32RandVal & 1U)) & 0x80200003U);
    /* Return the value */
    return uint32RandVal;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : UDS_ALG_HAL_Init
 * Description   : This function initial this module.
 *
 * Implements : UDS_ALG_hal_Init_Activity
 *END**************************************************************************/
void UDS_ALG_HAL_Init(void)
{
}

/*FUNCTION**********************************************************************
 *
 * Function Name : UDS_ALG_HAL_EncryptData
 * Description   : This function is encrypt data.
 *                 (Not used for Security Access in this project)
 *END**************************************************************************/
uint8 UDS_ALG_HAL_EncryptData(const uint8 *i_pPlainText, const uint32 i_dataLen, uint8 *o_pCipherText)
{
    (void)i_pPlainText;
    (void)i_dataLen;
    (void)o_pCipherText;
    return FALSE;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : UDS_ALG_HAL_DecryptData
 * Description   : This function is decrypt data.
 *                 (Kept for backward compatibility; not used for SA)
 *END**************************************************************************/
uint8 UDS_ALG_HAL_DecryptData(const uint8 *i_pCipherText, const uint32 i_dataLen, uint8 *o_pPlainText)
{
    (void)i_pCipherText;
    (void)i_dataLen;
    (void)o_pPlainText;
    return FALSE;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : UDS_ALG_HAL_GetRandom
 * Description   : This function is get random 锟斤拷取锟斤拷锟街�.
 *
 * Implements : UDS_ALG_hal_Init_Activity
 *END**************************************************************************/
uint8 UDS_ALG_HAL_GetRandom(const uint32 i_needRandomDataLen, uint8 *o_pRandomDataBuf)
{
    uint32 index;
    uint32 random = uds_timer_GetTimerTickCnt();
    random ^= (uint32)IfxScuCcu_getCpuFrequency(IfxCpu_getCoreIndex());  // CPU 频率
    random ^= (uint32)SCU_RSTSTAT.U;                              // 复位状态
    random ^= (gs_UDS_SWTimerTickCnt << 8);                       // 软件计数器
    
    for (index = 0; index < i_needRandomDataLen; index++)
    {
        o_pRandomDataBuf[index] = (random >> (index * 8)) & 0xFF;
        random = (random * 1103515245 + 12345) & 0xFFFFFFFF;      // LCG 迭代
    }
    return TRUE;
}

/* UDS software timer tick */
void UDS_ALG_HAL_AddSWTimerTickCnt(void)
{
#ifdef EN_CUSTOM_SA_ALGORITHM
    gs_UDS_SWTimerTickCnt++;
#endif
}

/*FUNCTION**********************************************************************
 *
 * Function Name : UDS_ALG_HAL_Deinit
 * Description   : This function init this module.
 *
 * Implements : UDS_ALG_Deinit_Activity
 *END**************************************************************************/
void UDS_ALG_HAL_Deinit(void)
{
}

/* -------------------------------------------- END OF FILE -------------------------------------------- */
