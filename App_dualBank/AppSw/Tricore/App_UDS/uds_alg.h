/*
 * @ File   : UDS_alg_hal.h
 * @ Brief  :
 * @ Author : Tomy
 * @ Date   : 2021-02-05
 * @ Version: V1.0
 * @ History: V1.0 2021-02-05 Summary
 *
 * MIT License. Copyright (c) 2021 SummerFalls.
 */

#ifndef UDS_ALG_HAL_H_
#define UDS_ALG_HAL_H_

#include "uds_common.h"

/* Security Access Level 1: Enhanced Diagnosis Algorithm */
/* Security Access Level 2: Bootloader Algorithm */

#define EN_CUSTOM_SA_ALGORITHM

void UDS_ALG_HAL_Init(void);

/*!
 * @brief Compute Key for Security Access Level 1.
 *
 * Algorithm: Key1[i] = Seed[i] ^ Mask[i]
 *            Key2[i] = (Seed[i] << 1) ^ Mask[i]
 *            Key[i]  = Key1[i] + Key2[i] (per-byte 8-bit, discard carry)
 *
 * @param[in]  i_pSeed   4-byte seed buffer (Seed[0] is MSB)
 * @param[out] o_pKey    4-byte key output buffer
 */
void UDS_ALG_HAL_ComputeKey_Level1(const uint8 *i_pSeed, uint8 *o_pKey);

/*!
 * @brief Compute Key for Security Access Level 2.
 *
 * Algorithm: Tmp[i] = Seed[i] ^ Mask[i]
 *            Key[0] = Tmp[2]
 *            Key[1] = ((Tmp[0] & 0x0F) << 4) | ((Tmp[1] & 0xF0) >> 4)
 *            Key[2] = (Tmp[1] & 0xF0) | ((Tmp[3] & 0xF0) >> 4)
 *            Key[3] = ((Tmp[0] & 0x0F) << 4) | (Tmp[3] & 0x0F)
 *
 * @param[in]  i_pSeed   4-byte seed buffer (Seed[0] is MSB)
 * @param[out] o_pKey    4-byte key output buffer
 */
void UDS_ALG_HAL_ComputeKey_Level2(const uint8 *i_pSeed, uint8 *o_pKey);

/*!
 * @brief To UDS encrypt data.
 *
 * This function returns encrypt data status.
 *
 * @param[in] i_pPlainText point plaintext
 * @param[in] i_dataLen point plaintext data lenght
 * @param[out]  o_pCipherText point ciphertext
 * @return encrypt data status.
 */
uint8 UDS_ALG_HAL_EncryptData(const uint8 *i_pPlainText, const uint32 i_dataLen, uint8 *o_pCipherText);

/*!
 * @brief To UDS decrypt data.
 *
 * This function returns decrypt data status.
 *
 * @param[in] i_pCipherText point ciphertext
 * @param[in] i_dataLen point ciphertext data lenght
 * @param[out]  o_pPlainText point plaintext
 * @return decrypt data status.
 */
uint8 UDS_ALG_HAL_DecryptData(const uint8 *i_pCipherText, const uint32 i_dataLen, uint8 *o_pPlainText);

/*!
 * @brief To UDS get random data.
 *
 * This function returns get random data status.
 *
 * @param[in] i_needRandomLen need random data len
 * @param[out]  o_pRandomBuf point random data buff
 * @return get random data status.
 */
uint8 UDS_ALG_HAL_GetRandom(const uint32 i_needRandomDataLen, uint8 *o_pRandomDataBuf);

/* UDS software timer tick */
void UDS_ALG_HAL_AddSWTimerTickCnt(void);

void UDS_ALG_HAL_Deinit(void);

#endif /* UDS_ALG_HAL_H_ */

/* -------------------------------------------- END OF FILE -------------------------------------------- */
