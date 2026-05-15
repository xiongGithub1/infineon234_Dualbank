/**********************************************************************************************************************
 * \file    uds_dtc.c
 * \brief   DTC (Diagnostic Trouble Code) Manager - OEM Standard Implementation
 *          Compliant with ISO 14229-1 / ISO 15031-6
 * \version V2.0.0
 * \date    2026-05-14
 *********************************************************************************************************************/
#include <uds_dtc.h>
#include <uds_timer.h>
#include "App_bootloader.h"
#include "Boot_DualBank.h"
#include "Flash.h"

/*======================================================================================================================*
 *  External Variables
 *======================================================================================================================*/
extern uint8 can_node1_error;   /* From App_bootloader.c: 0=OK, 1=BusOff, 2=AckError */

/*======================================================================================================================*
 *  DFlash Snapshot Storage Configuration
 *======================================================================================================================*/
#define DFLASH_SNAPSHOT_SECTOR_ADDR   (0xAF002000U)
#define DFLASH_SNAPSHOT_SIZE          (DTC_CODE_MAX_NUM * SANP_RECORD_MAX_NUM * SANP_DATA_PER_SIZE) /* 512 bytes */

/* RAM mirror for DTC snapshot data (freeze frame).
 * Data is loaded from DFlash at init and persisted back on fault confirmation.
 */
static uint8 s_dtcSnapshotMirror[DFLASH_SNAPSHOT_SIZE];

/*======================================================================================================================*
 *  Local Function Prototypes (Fault Detection)
 *======================================================================================================================*/
static DTCTestResult DtcCheckCanBusOff(void);
static DTCTestResult DtcCheckCanAckError(void);
static uint8 dtcCollectSnapshotData(snap_data_t *p_snap_data);

/*======================================================================================================================*
 *  DTC Data Table - OEM Standard Definitions
 *======================================================================================================================*/
static dtc_data_table dtcDataTableList[DTC_DID_MAX_NUM] =
{
    /* Index 0: U0100 - CAN Bus-Off (Periodic test, 100ms) */
    {
        DTC_U0100,
        {DTC_STATUS_INIT},
        {SANP_EEPROM_BASE_ADDR + 0u * SANP_DATA_PER_SIZE, 0},
        {0, 0, 0},
        0, 100, 0,
        DtcCheckCanBusOff
    },
    /* Index 1: U0121 - CAN Ack Error (Periodic test, 100ms) */
    {
        DTC_U0121,
        {DTC_STATUS_INIT},
        {SANP_EEPROM_BASE_ADDR + 1u * SANP_DATA_PER_SIZE, 0},
        {0, 0, 0},
        0, 100, 0,
        DtcCheckCanAckError
    },
    /* Index 2: B1000 - ECU Boot Failure (One-shot, set at init) */
    {
        DTC_B1000,
        {DTC_STATUS_INIT},
        {SANP_EEPROM_BASE_ADDR + 2u * SANP_DATA_PER_SIZE, 0},
        {0, 0, 0},
        0, 0, 0,
        NULL_PTR
    },
    /* Index 3: P0601 - Internal Memory Checksum Error (One-shot, set at init) */
    {
        DTC_P0601,
        {DTC_STATUS_INIT},
        {SANP_EEPROM_BASE_ADDR + 3u * SANP_DATA_PER_SIZE, 0},
        {0, 0, 0},
        0, 0, 0,
        NULL_PTR
    },
    /* Index 4-7: Reserved */
    {DTC_RESERVED_1, {0}, {0, 0}, {0, 0, 0}, 0, 0, 0, NULL_PTR},
    {DTC_RESERVED_2, {0}, {0, 0}, {0, 0, 0}, 0, 0, 0, NULL_PTR},
    {0, {0}, {0, 0}, {0, 0, 0}, 0, 0, 0, NULL_PTR},
    {0, {0}, {0, 0}, {0, 0, 0}, 0, 0, 0, NULL_PTR}
};

/* Global DTC update enable flag (controlled by 0x85 service) */
uint8 isDtcStatuCanUpdate = ON;

/*======================================================================================================================*
 *  Fault Detection Functions
 *======================================================================================================================*/
static DTCTestResult DtcCheckCanBusOff(void)
{
    if (can_node1_error == 1) { return TEST_FAILED; }
    return TEST_PASSED;
}

static DTCTestResult DtcCheckCanAckError(void)
{
    if (can_node1_error == 2) { return TEST_FAILED; }
    return TEST_PASSED;
}

/*======================================================================================================================*
 *  Snapshot (Freeze Frame) Functions
 *======================================================================================================================*/
static uint8 dtcCollectSnapshotData(snap_data_t *p_snap_data)
{
    uint8 idx = 0;
    DualBankFlags_t flags;
    uint32 activeBank = Boot_DualBank_GetActiveBank();

    p_snap_data[idx].record_did = SNAP_DID_SYS_VOLTAGE;
    p_snap_data[idx].record_data = 0x0000; /* TODO: Integrate with ADC */
    idx++;

    p_snap_data[idx].record_did = SNAP_DID_CAN_STATUS;
    p_snap_data[idx].record_data = (uint16)can_node1_error;
    idx++;

    p_snap_data[idx].record_did = SNAP_DID_RUN_TIME;
    p_snap_data[idx].record_data = (uint16)(uds_timer_Get1msCnt() / 1000u);
    idx++;

    p_snap_data[idx].record_did = SNAP_DID_BANK_STATUS;
    p_snap_data[idx].record_data = (uint16)((activeBank == BANK_B) ? 0x000Bu : 0x000Au);
    idx++;

    p_snap_data[idx].record_did = SNAP_DID_BOOT_CNT;
    p_snap_data[idx].record_data = Boot_DualBank_ReadFlags(&flags) ? flags.main.bootAttempts : 0xFFFFu;
    idx++;

    p_snap_data[idx].record_did = SNAP_DID_SW_VERSION;
    p_snap_data[idx].record_data = 0x0001u;
    idx++;

    p_snap_data[idx].record_did = SNAP_DID_HW_VERSION;
    p_snap_data[idx].record_data = 0x0001u;
    idx++;

    p_snap_data[idx].record_did = SNAP_DID_AMB_TEMP;
    p_snap_data[idx].record_data = 0x0000; /* TODO: Integrate with temperature sensor */
    idx++;

    return idx;
}

void dtcSaveSanpData(uint16 eepromAddr)
{
    snap_data_t snap_buf[SANP_DATA_DID_NUM];
    uint8 did_count;
    uint8 i;
    uint8 data_buf[SANP_DATA_PER_SIZE] = {0};
    uint16 mirrorOffset;

    did_count = dtcCollectSnapshotData(snap_buf);
    if (did_count == 0) { return; }

    for (i = 0; i < did_count && i < SANP_DATA_DID_NUM; i++)
    {
        data_buf[i * 4 + 0] = (uint8)((snap_buf[i].record_did >> 8) & 0xFF);
        data_buf[i * 4 + 1] = (uint8)(snap_buf[i].record_did & 0xFF);
        data_buf[i * 4 + 2] = (uint8)((snap_buf[i].record_data >> 8) & 0xFF);
        data_buf[i * 4 + 3] = (uint8)(snap_buf[i].record_data & 0xFF);
    }

    /* Update RAM mirror at the corresponding offset */
    mirrorOffset = eepromAddr - SANP_EEPROM_BASE_ADDR;
    if (mirrorOffset < DFLASH_SNAPSHOT_SIZE)
    {
        tl_memcpy(&s_dtcSnapshotMirror[mirrorOffset], data_buf, SANP_DATA_PER_SIZE);
    }

    /* Persist entire RAM mirror to DFlash.
     * DFlash requires sector erase before write.
     */
    Flash_eraseDFlash_port(DFLASH_SNAPSHOT_SECTOR_ADDR);
    Flash_writeDFlash_port(DFLASH_SNAPSHOT_SECTOR_ADDR, (uint32 *)s_dtcSnapshotMirror, DFLASH_SNAPSHOT_SIZE);
}

/*======================================================================================================================*
 *  DTC Initialization
 *======================================================================================================================*/
void dtcInit(void)
{
    uint16 i;
    uint32 now = uds_timer_Get1msCnt();
    DualBankFlags_t flags;
    boolean flagsValid = Boot_DualBank_ReadFlags(&flags);

    for (i = 0; i < DTC_CODE_MAX_NUM; i++)
    {
        if (dtcDataTableList[i].dtc_code == 0) { continue; }

        dtcDataTableList[i].dtc_st.byteAll = DTC_STATUS_INIT;
        dtcDataTableList[i].debounceCnt = 0;
        dtcDataTableList[i].lastTestTime = now;
        dtcDataTableList[i].dtcSnapData.current = 0;

        if (dtcDataTableList[i].extData.agingCounter == 0xFF)
        {
            dtcDataTableList[i].extData.agingCounter = 0;
        }
    }

    /* Load snapshot data from DFlash into RAM mirror */
    tl_memcpy(s_dtcSnapshotMirror, (const void *)DFLASH_SNAPSHOT_SECTOR_ADDR, DFLASH_SNAPSHOT_SIZE);

    /* Startup one-shot checks */
    if (flagsValid && (flags.main.bootAttempts >= MAX_BOOT_ATTEMPTS))
    {
        for (i = 0; i < DTC_CODE_MAX_NUM; i++)
        {
            if (dtcDataTableList[i].dtc_code == DTC_B1000)
            {
                dtcDataTableList[i].dtc_st.bit.ConfirmedDTC = 1;
                dtcDataTableList[i].dtc_st.bit.TestFailed = 1;
                dtcDataTableList[i].dtc_st.bit.TestFailedSinceLastClear = 1;
                dtcDataTableList[i].dtc_st.bit.TestFailedThisMonitoringCycle = 1;
                dtcDataTableList[i].dtc_st.bit.TestNotCompleteSinceLastClear = 0;
                dtcDataTableList[i].dtc_st.bit.TestNotCompleteThisMonitoringCycle = 0;
                dtcDataTableList[i].extData.occurrenceCounter++;
                dtcDataTableList[i].extData.faultOccurrenceSinceClear++;
                break;
            }
        }
    }

    if (flagsValid)
    {
        BankStatus_t status = Boot_DualBank_VerifyBank(Boot_DualBank_GetActiveBank());
        if (status != BANK_STATUS_VALID)
        {
            for (i = 0; i < DTC_CODE_MAX_NUM; i++)
            {
                if (dtcDataTableList[i].dtc_code == DTC_P0601)
                {
                    dtcDataTableList[i].dtc_st.bit.ConfirmedDTC = 1;
                    dtcDataTableList[i].dtc_st.bit.TestFailed = 1;
                    dtcDataTableList[i].dtc_st.bit.TestFailedSinceLastClear = 1;
                    dtcDataTableList[i].dtc_st.bit.TestFailedThisMonitoringCycle = 1;
                    dtcDataTableList[i].dtc_st.bit.TestNotCompleteSinceLastClear = 0;
                    dtcDataTableList[i].dtc_st.bit.TestNotCompleteThisMonitoringCycle = 0;
                    dtcDataTableList[i].extData.occurrenceCounter++;
                    dtcDataTableList[i].extData.faultOccurrenceSinceClear++;
                    break;
                }
            }
        }
    }
}


/*======================================================================================================================*
 *  DTC Test Main Process - ISO 14229-1 Compliant State Machine
 *======================================================================================================================*/

/**
 * @brief Check if test period has elapsed for a given DTC entry
 * @param lastTime Last test timestamp (ms)
 * @param periodMs Test period (ms)
 * @return 1 if timeout, 0 otherwise
 */
static uint8 dtcIsTestPeriodElapsed(uint32 lastTime, uint8 periodMs)
{
    uint32 now = uds_timer_Get1msCnt();
    if (periodMs == 0) { return 0; } /* One-shot: never re-test */
    if ((now - lastTime) >= (uint32)periodMs) { return 1; }
    return 0;
}

/**
 * @brief Handle DTC state update on TEST_PASSED result
 * @param pDtc Pointer to DTC entry
 */
static void dtcHandleTestPassed(dtc_data_table *pDtc)
{
    /* Update current test result */
    pDtc->dtc_st.bit.TestFailed = 0;
    pDtc->dtc_st.bit.TestNotCompleteThisMonitoringCycle = 0;
    pDtc->dtc_st.bit.TestNotCompleteSinceLastClear = 0;

    /* FDT decrement (healing) */
    if (pDtc->extData.fdt_cnt > FDT_MIN)
    {
        pDtc->extData.fdt_cnt += FDT_STEP_PASS; /* -1 */
    }

    /* Aging counter for confirmed DTCs */
    if (pDtc->dtc_st.bit.ConfirmedDTC && (pDtc->extData.agingCounter < AGN_MAX))
    {
        pDtc->extData.agingCounter += AGN_STEP;
        if (pDtc->extData.agingCounter >= AGN_MAX)
        {
            /* Deconfirm after aging threshold reached */
            pDtc->dtc_st.bit.ConfirmedDTC = 0;
            pDtc->dtc_st.bit.WarningIndicatorRequested = 0;
            pDtc->extData.fdt_cnt = 0;
        }
    }
}

/**
 * @brief Handle DTC state update on TEST_FAILED result
 * @param pDtc Pointer to DTC entry
 */
static void dtcHandleTestFailed(dtc_data_table *pDtc)
{
    /* Update current test result */
    pDtc->dtc_st.bit.TestFailed = 1;
    pDtc->dtc_st.bit.TestFailedThisMonitoringCycle = 1;
    pDtc->dtc_st.bit.TestFailedSinceLastClear = 1;
    pDtc->dtc_st.bit.TestNotCompleteThisMonitoringCycle = 0;
    pDtc->dtc_st.bit.TestNotCompleteSinceLastClear = 0;

    /* FDT increment */
    if (pDtc->extData.fdt_cnt < FDT_MAX)
    {
        pDtc->extData.fdt_cnt += FDT_STEP_FAIL; /* +2 */
    }

    /* Debounce for PendingDTC */
    if (!pDtc->dtc_st.bit.PendingDTC)
    {
        pDtc->debounceCnt++;
        if (pDtc->debounceCnt >= DTC_DEBOUNCE_CYCLES)
        {
            pDtc->dtc_st.bit.PendingDTC = 1;
            pDtc->debounceCnt = DTC_DEBOUNCE_CYCLES;
        }
    }

    /* ConfirmedDTC when FDT reaches threshold */
    if (pDtc->extData.fdt_cnt >= FDT_CONFIRM_THRESH)
    {
        if (!pDtc->dtc_st.bit.ConfirmedDTC)
        {
            /* First time confirmed: record snapshot */
            uint16 actAddr = pDtc->dtcSnapData.base + pDtc->dtcSnapData.current * SANP_DATA_PER_SIZE;
            dtcSaveSanpData(actAddr);
            pDtc->dtcSnapData.current++;
            if (pDtc->dtcSnapData.current >= SANP_RECORD_MAX_NUM)
            {
                pDtc->dtcSnapData.current = 0;
            }

            pDtc->dtc_st.bit.ConfirmedDTC = 1;
            pDtc->dtc_st.bit.WarningIndicatorRequested = 1;
            pDtc->extData.occurrenceCounter++;
            pDtc->extData.faultOccurrenceSinceClear++;
        }
        /* Reset aging counter on new failure */
        pDtc->extData.agingCounter = 0;
    }
}

/**
 * @brief Main DTC test processing loop
 * @note Must be called periodically from main loop (e.g. every 1-10ms)
 */
void dtcTestMainProc(void)
{
    DTCTestResult currentResult = TEST_PASSED;
    dtc_data_table *pDtc = NULL_PTR;
    uint16 i;

    for (i = 0; i < DTC_CODE_MAX_NUM; i++)
    {
        pDtc = &dtcDataTableList[i];

        /* Skip empty or one-shot entries */
        if ((pDtc->dtc_code == 0) || (pDtc->testFunHandler == NULL_PTR))
        {
            continue;
        }

        /* Check test period */
        if (!dtcIsTestPeriodElapsed(pDtc->lastTestTime, pDtc->testPeriodMs))
        {
            continue;
        }
        pDtc->lastTestTime = uds_timer_Get1msCnt();

        /* Execute fault detection test */
        currentResult = pDtc->testFunHandler();

        /* If DTC update is disabled (0x85 02), only update TFTMC and TNCTMC */
        if (isDtcStatuCanUpdate == OFF)
        {
            if (currentResult == TEST_FAILED)
            {
                pDtc->dtc_st.bit.TestFailedThisMonitoringCycle = 1;
            }
            pDtc->dtc_st.bit.TestNotCompleteThisMonitoringCycle = 0;
            continue;
        }

        /* Process test result */
        switch (currentResult)
        {
            case TEST_PASSED:
                dtcHandleTestPassed(pDtc);
                break;

            case TEST_FAILED:
                dtcHandleTestFailed(pDtc);
                break;

            case TEST_NORESULT:
            default:
                break;
        }
    }
}

/*======================================================================================================================*
 *  DTC Query & Clear Functions
 *======================================================================================================================*/

void clearDTCByGroup(uint32 group)
{
    uint16 i;
    for (i = 0; i < DTC_CODE_MAX_NUM; i++)
    {
        if (dtcDataTableList[i].dtc_code == 0) { continue; }
        if (dtcDataTableList[i].dtc_code & group)
        {
            dtcDataTableList[i].dtc_st.byteAll = DTC_STATUS_INIT;
            dtcDataTableList[i].debounceCnt = 0;
            dtcDataTableList[i].extData.fdt_cnt = 0;
            dtcDataTableList[i].extData.faultOccurrenceSinceClear = 0;
            dtcDataTableList[i].extData.agingCounter = 0;
            /* occurrenceCounter is NOT cleared (lifetime counter) */
        }
    }
}

dtc_status_t getStatusByDtcCode(uint32 dtc)
{
    dtc_status_t status = {0};
    uint16 i;
    for (i = 0; i < DTC_CODE_MAX_NUM; i++)
    {
        if (dtcDataTableList[i].dtc_code == dtc)
        {
            return dtcDataTableList[i].dtc_st;
        }
    }
    return status;
}

uint8 IsFaultConfirmed(DTCStatusType status)
{
    return (status.DTCbit.ConfirmedDTC) ? TRUE : FALSE;
}

uint16 getDTCCountByStatusMask(uint8 status_mask)
{
    uint16 dtc_count = 0;
    uint16 i;
    for (i = 0; i < DTC_CODE_MAX_NUM; i++)
    {
        if (dtcDataTableList[i].dtc_code == 0) { continue; }
        if ((dtcDataTableList[i].dtc_st.byteAll & status_mask))
        {
            dtc_count++;
        }
    }
    return dtc_count;
}

uint16 getDTCByStatusMask(uint8 *p_dtc, uint8 status_mask)
{
    uint16 dtc_count = 0;
    uint16 i;

    for (i = 0; i < DTC_CODE_MAX_NUM; i++)
    {
        if (dtcDataTableList[i].dtc_code == 0) { continue; }
        if ((dtcDataTableList[i].dtc_st.byteAll & status_mask))
        {
            *p_dtc++ = GET_DTC_HIGH_BYTE(dtcDataTableList[i].dtc_code);
            *p_dtc++ = GET_DTC_MID_BYTE(dtcDataTableList[i].dtc_code);
            *p_dtc++ = GET_DTC_LOW_BYTE(dtcDataTableList[i].dtc_code);
            *p_dtc++ = dtcDataTableList[i].dtc_st.byteAll;
            dtc_count++;
        }
    }
    return dtc_count;
}

uint16 getDTCSupportedDtc(uint8 *p_dtc)
{
    uint16 dtc_count = 0;
    uint16 i;
    for (i = 0; i < DTC_CODE_MAX_NUM; i++)
    {
        uint32 did = dtcDataTableList[i].dtc_code;
        if (did)
        {
            *p_dtc++ = GET_DTC_HIGH_BYTE(did);
            *p_dtc++ = GET_DTC_MID_BYTE(did);
            *p_dtc++ = GET_DTC_LOW_BYTE(did);
            *p_dtc++ = dtcDataTableList[i].dtc_st.byteAll;
            dtc_count++;
        }
    }
    return dtc_count;
}

uint8 getDTCExtData(uint32 dtc_code, uint8 record_num, uint8 *p_data, uint8 *p_ext_len)
{
    uint16 i;

    if ((record_num != 0xFF) && (record_num != 0x01))
    {
        return FALSE;
    }

    for (i = 0; i < DTC_CODE_MAX_NUM; i++)
    {
        if (dtcDataTableList[i].dtc_code == dtc_code)
        {
            *p_data++ = record_num;
            (*p_ext_len)++;

            *p_data++ = dtcDataTableList[i].extData.occurrenceCounter;
            (*p_ext_len)++;

            *p_data++ = dtcDataTableList[i].extData.agingCounter;
            (*p_ext_len)++;

            *p_data++ = dtcDataTableList[i].extData.faultOccurrenceSinceClear;
            (*p_ext_len)++;

            *p_data++ = (uint8)((dtcDataTableList[i].extData.fdt_cnt >> 8) & 0xFF);
            (*p_ext_len)++;

            *p_data++ = (uint8)(dtcDataTableList[i].extData.fdt_cnt & 0xFF);
            (*p_ext_len)++;

            return TRUE;
        }
    }
    return FALSE;
}

uint8 getDTCSanpData(uint32 dtc_code, uint8 record_idx, uint8 *p_data, uint8 *p_snap_len)
{
    uint8 dataBuffer[SANP_DATA_PER_SIZE] = {0};
    uint16 i, j, k;

    if ((record_idx != 0xFF) && (record_idx > SANP_RECORD_MAX_NUM))
    {
        return FALSE;
    }

    for (i = 0; i < DTC_CODE_MAX_NUM; i++)
    {
        if (dtcDataTableList[i].dtc_code == dtc_code)
        {
            if (record_idx != 0xFF)
            {
                uint16 actAddr = dtcDataTableList[i].dtcSnapData.base + record_idx * SANP_DATA_PER_SIZE;

                {
                    uint16 mirrorOffset = actAddr - SANP_EEPROM_BASE_ADDR;
                    if (mirrorOffset >= DFLASH_SNAPSHOT_SIZE)
                    {
                        return FALSE;
                    }
                    tl_memcpy(dataBuffer, &s_dtcSnapshotMirror[mirrorOffset], SANP_DATA_PER_SIZE);

                    *p_data++ = record_idx;
                    (*p_snap_len)++;
                    *p_data++ = SANP_DATA_DID_NUM;
                    (*p_snap_len)++;

                    for (j = 0; j < SANP_DATA_PER_SIZE; j += 4)
                    {
                        uint16 record_did = (uint16)((dataBuffer[j+1] << 8) | dataBuffer[j]);
                        uint16 record_data = (uint16)((dataBuffer[j+3] << 8) | dataBuffer[j+2]);
                        *p_data++ = (uint8)((record_did >> 8) & 0xFF);
                        (*p_snap_len)++;
                        *p_data++ = (uint8)(record_did & 0xFF);
                        (*p_snap_len)++;
                        *p_data++ = (uint8)((record_data >> 8) & 0xFF);
                        (*p_snap_len)++;
                        *p_data++ = (uint8)(record_data & 0xFF);
                        (*p_snap_len)++;
                    }
                }
                break;
            }
            else
            {
                for (j = 0; j < SANP_RECORD_MAX_NUM; j++)
                {
                    uint16 actAddr = dtcDataTableList[i].dtcSnapData.base + j * SANP_DATA_PER_SIZE;
                    uint16 mirrorOffset = actAddr - SANP_EEPROM_BASE_ADDR;
                    if (mirrorOffset >= DFLASH_SNAPSHOT_SIZE)
                    {
                        return FALSE;
                    }
                    tl_memcpy(dataBuffer, &s_dtcSnapshotMirror[mirrorOffset], SANP_DATA_PER_SIZE);

                    *p_data++ = (uint8)j;
                    (*p_snap_len)++;
                    *p_data++ = SANP_DATA_DID_NUM;
                    (*p_snap_len)++;

                    for (k = 0; k < SANP_DATA_PER_SIZE; k += 4)
                    {
                        uint16 record_did = (uint16)((dataBuffer[k+1] << 8) | dataBuffer[k]);
                        uint16 record_data = (uint16)((dataBuffer[k+3] << 8) | dataBuffer[k+2]);
                        *p_data++ = (uint8)((record_did >> 8) & 0xFF);
                        (*p_snap_len)++;
                        *p_data++ = (uint8)(record_did & 0xFF);
                        (*p_snap_len)++;
                        *p_data++ = (uint8)((record_data >> 8) & 0xFF);
                        (*p_snap_len)++;
                        *p_data++ = (uint8)(record_data & 0xFF);
                        (*p_snap_len)++;
                    }
                }
                break;
            }
        }
    }
    return TRUE;
}

void dtcAddSnapData(uint32 dtc_code, snap_data_t *p_record, uint8 record_len)
{
    uint16 i;
    for (i = 0; i < DTC_CODE_MAX_NUM; i++)
    {
        if (dtcDataTableList[i].dtc_code == dtc_code)
        {
            (void)p_record;
            (void)record_len;
            break;
        }
    }
}
