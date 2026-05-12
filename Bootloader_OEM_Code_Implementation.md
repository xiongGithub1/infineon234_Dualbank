# Bootloader 车企标准代码修改对照文档

> **文档目的**：将 7 个 OEM 标准阶段逐项落实到代码中，明确每个要求的文件位置、函数、行号及修改内容。

---

## 修改文件清单

| 序号 | 文件路径 | 修改类型 |
|------|---------|---------|
| 1 | `BootLoader_dualBank/AppSw/Tricore/App_bootloader/Boot_DualBank.h` | 新增阶段枚举、向量表偏移常量、全局变量声明 |
| 2 | `BootLoader_dualBank/AppSw/Tricore/App_bootloader/Boot_DualBank.c` | 新增阶段标识、BIV/BTV 设置函数、裸跳转前向量表恢复 |
| 3 | `BootLoader_dualBank/AppSw/Tricore/Main/Cpu0_Main.c` | 启用复位原因记录、RAM 标志检查、启动阶段标识 |
| 4 | `BootLoader_dualBank/AppSw/Tricore/App_bootloader/App_bootloader.c` | Bootloader 阶段标识、主循环阶段标识 |
| 5 | `BootLoader_dualBank/AppSw/Tricore/App_UDS/uds_app.c` | 刷写会话阶段标识、CRC 校验阶段标识、跳转决策阶段标识 |

---

## Phase 1: 系统启动（System Startup）

### OEM 标准要求
- 记录复位原因，用于故障追溯
- 关闭看门狗，防止启动过程中复位
- 初始化系统时钟、延时、板级指示
- 设置启动阶段标识，便于调试和故障分析
- 支持 APP 通过 RAM 标志请求进入 Bootloader

### 代码落实

**文件**：`Cpu0_Main.c`（行 69~128）

| 要求 | 代码实现 | 行号 |
|------|---------|------|
| 复位原因记录 | `resetReason = ResetStatus_Previous(); isPowerOnReset = ((resetReason & 0x01u) != 0u) ? TRUE : FALSE;` | 92~93 |
| 阶段标识 STARTUP | `g_bootPhase = BOOT_PHASE_STARTUP;` | 82 |
| 看门狗关闭 | `IfxScuWdt_disableCpuWatchdog(...); IfxScuWdt_disableSafetyWatchdog(...);` | 89~90 |
| RAM 标志检查 | 启用原先被注释掉的 RAM 标志检查逻辑，`if (ramBootMode == RAM_BOOT_MODE_APP)` | 108~116 |

**关键修改对比**：

```c
// 修改前：复位原因未使用，RAM 标志检查被注释掉
uint32 resetReason;
boolean isPowerOnReset;
void core0_main(void)
{
    IfxScuWdt_disableCpuWatchdog(...);
    ...
    Boot_DualBank_Init();
    /* RAM 标志检查被注释 */
//   {
//       uint16 ramBootMode = *(uint16 *)RAM_BOOT_MODE_Addr;
//       ...
//   }
    Boot_DualBank_SelectAndJump();
    AppBL_init();
}

// 修改后：完整 OEM 启动流程
void core0_main(void)
{
    g_bootPhase = BOOT_PHASE_STARTUP;          // <-- 新增
    IfxScuWdt_disableCpuWatchdog(...);
    resetReason = ResetStatus_Previous();       // <-- 启用
    isPowerOnReset = ((resetReason & 0x01u) != 0u) ? TRUE : FALSE; // <-- 启用
    ...
    Boot_DualBank_Init();
    {
        uint16 ramBootMode = *(uint16 *)RAM_BOOT_MODE_Addr;
        if (ramBootMode == RAM_BOOT_MODE_APP)   // <-- 启用
        {
            *(uint16 *)RAM_BOOT_MODE_Addr = RAM_BOOT_MODE_NORMAL;
            g_bootPhase = BOOT_PHASE_BL_ENTRY;  // <-- 新增
        }
        else
        {
            Boot_DualBank_SelectAndJump();
        }
    }
    AppBL_init();
}
```

---

## Phase 2: 双 Bank 标志初始化（Dual-Bank Flag Init）

### OEM 标准要求
- DFlash 标志双份冗余（Main + Shadow）
- Magic 校验 + CRC 校验 + Sequence 仲裁
- 首次启动或损坏时初始化默认值
- 标志初始化阶段可追溯

### 代码落实

**文件**：`Boot_DualBank.c`（行 253~278）

| 要求 | 代码实现 | 行号 |
|------|---------|------|
| 阶段标识 FLAG_INIT | `g_bootPhase = BOOT_PHASE_FLAG_INIT;` | 259 |
| 双份冗余读取 | `Boot_DualBank_ReadFlags()` 同时读取 Main 和 Shadow | 284~345 |
| Magic + CRC + Sequence | `Boot_CalcFlagCRC()` + `memcmp` + `sequence` 仲裁 | 296~328 |
| 损坏恢复 | `Boot_DualBank_Init()` 中若返回 FALSE，写入默认值 | 257~275 |

**新增全局变量**：

```c
// Boot_DualBank.h 行 63~83
typedef enum
{
    BOOT_PHASE_STARTUP          = 0x00u,
    BOOT_PHASE_FLAG_INIT        = 0x01u,
    BOOT_PHASE_BANK_VERIFY      = 0x02u,
    BOOT_PHASE_JUMP_DECISION    = 0x03u,
    BOOT_PHASE_JUMP_EXEC        = 0x04u,
    BOOT_PHASE_ROLLBACK         = 0x05u,
    BOOT_PHASE_BL_ENTRY         = 0x10u,
    BOOT_PHASE_BL_MAIN          = 0x11u,
    BOOT_PHASE_PROG_SESSION     = 0x20u,
    BOOT_PHASE_PROG_VERIFY      = 0x21u,
    BOOT_PHASE_ERROR            = 0xFFu
} BootPhase_t;

extern volatile BootPhase_t g_bootPhase;
```

```c
// Boot_DualBank.c 行 89
volatile BootPhase_t g_bootPhase = BOOT_PHASE_STARTUP;
```

---

## Phase 3: Bank 有效性验证（Bank Validity Verification）

### OEM 标准要求
- CRC32 全 Bank 计算，覆盖完整 Bank 区域
- CRC 计算期间需喂狗或关狗防超时
- 验证阶段可追溯

### 代码落实

**文件**：`Boot_DualBank.c`（行 360~414）

| 要求 | 代码实现 | 行号 |
|------|---------|------|
| 阶段标识 BANK_VERIFY | `g_bootPhase = BOOT_PHASE_BANK_VERIFY;` | 651（SelectAndJump 中） |
| CRC32 全量计算 | `Boot_DualBank_CalculateCRC()` 使用 uncached 地址 | 360~365 |
| Cache 同步 | `__dsync()` 在 CRC 计算前执行，确保看到最新 Flash 数据 | 363 |
| CRC 比对 | `Boot_DualBank_VerifyBank()` 中 `actualCRC == validFlag` | 406~410 |

---

## Phase 4A: 跳转决策与执行（Jump to APP）

### OEM 标准要求
- 启动尝试计数 + 阈值检查（防变砖）
- 跳转前外设去初始化（CAN / TMR）
- 跳转前禁用 ECC Trap
- 跳转前关闭 Data/Program Cache + Barrier
- **跳转前设置 APP 的 BIV/BTV（向量表）**
- 裸跳转（`ji`）切断 CSA 上下文链表
- 入口地址 Sanity Check
- 各子阶段可追溯

### 代码落实

**文件**：`Boot_DualBank.c`（行 532~620）

#### 4A.1 新增 BIV/BTV 设置函数

```c
// 行 532~548：新增 Boot_DualBank_SetAppVectors()
void Boot_DualBank_SetAppVectors(uint32 bankStartAddr)
{
    uint32 appBiv;
    uint32 appBtv;

    /* TriCore BIV for VSS=0 (32-byte vector spacing):
     * BIV = (INTTAB_base | 0x1FE0).
     */
    appBiv = (bankStartAddr + APP_INTTAB_OFFSET) | 0x1FE0u;
    appBtv = (bankStartAddr + APP_TRAPTAB_OFFSET);

    __mtcr(CPU_BIV, appBiv);
    __isync();
    __mtcr(CPU_BTV, appBtv);
    __isync();
}
```

**新增常量**（`Boot_DualBank.h` 行 36~37）：
```c
#define APP_INTTAB_OFFSET               0x0000C000u
#define APP_TRAPTAB_OFFSET              0x0000D000u
```

> **注意**：`APP_INTTAB_OFFSET` 和 `APP_TRAPTAB_OFFSET` 必须与 APP 工程的 LSL 链接器配置中的 `INTTAB0` / `TRAPTAB0` 位置一致。若 APP 向量表位置不同，需同步修改此处。

#### 4A.2 跳转函数 OEM 化

| 步骤 | OEM 要求 | 代码位置 | 行号 |
|------|---------|---------|------|
| J1 | 阶段标识 JUMP_EXEC | `g_bootPhase = BOOT_PHASE_JUMP_EXEC;` | 555 |
| J2 | Sanity Check | `if ((*(volatile uint32 *)(startAddr + 0x20u) == 0xFFFFFFFFu) ...)` | 567~568 |
| J3 | 全局禁用中断 | `IfxCpu_disableInterrupts();` | 575 |
| J4 | 外设去初始化 | `CAN_deinit(); TMR_deinit();` | 578~579 |
| J5 | 禁用 ECC Trap | `FLASH0_MARP.B.TRAPDIS = 1; FLASH0_MARD.B.TRAPDIS = 1;` | 585~586 |
| J6 | 关闭 Cache | `IfxCpu_setDataCache(0); IfxCpu_setProgramCache(0);` | 591~592 |
| J7 | Barrier | `__dsync(); __isync();` | 594~595 |
| **J8** | **设置 APP BIV/BTV** | `Boot_DualBank_SetAppVectors(startAddr);` | **605** |
| J9 | 切断 CSA | `__mtcr(CPU_PCXI, 0);` | 617 |
| J10 | 裸跳转 | `ji a15` | 625 |

**关键新增**：第 J8 步（行 605）是本次修改最核心的安全增强。在 OEM 标准中，跳转前必须将 CPU 的向量表指针切换到 APP 的向量表，否则 APP 中发生的任何中断或 Trap 都会落入 Bootloader 的向量表，导致不可预期的行为。

---

## Phase 4B: 回滚与容错处理（Rollback & Fallback）

### OEM 标准要求
- 目标 Bank 无效时尝试 Fallback Bank
- 连续启动失败（MAX_BOOT_ATTEMPTS=3）后自动回滚
- 回滚阶段可追溯
- Fallback 无效时安全降级到 Bootloader

### 代码落实

**文件**：`Boot_DualBank.c`（行 651~740）

| 场景 | 阶段标识 | 代码 | 行号 |
|------|---------|------|------|
| 目标 Bank 有效，但 bootAttempts >= 3 | ROLLBACK | `g_bootPhase = BOOT_PHASE_ROLLBACK;` | 684 |
| 目标 Bank 无效，尝试 Fallback | ROLLBACK | `g_bootPhase = BOOT_PHASE_ROLLBACK;` | 712 |
| Fallback 成功，准备复位 | — | `SW_Reset();` | 693, 718 |
| 两 Bank 均无效 | BL_ENTRY | `g_bootPhase = BOOT_PHASE_BL_ENTRY;` | 700, 727 |

---

## Phase 5: Bootloader 模式进入（Bootloader Mode Entry）

### OEM 标准要求
- 初始化 CAN / UDS / Flash / Timer
- 读取并处理 RAM/Flash 标志
- 全局使能中断
- Bootloader 进入阶段可追溯

### 代码落实

**文件**：`App_bootloader.c`（行 327~365）

| 要求 | 代码实现 | 行号 |
|------|---------|------|
| 阶段标识 BL_ENTRY | `g_bootPhase = BOOT_PHASE_BL_ENTRY;` | 330 |
| RAM 标志处理 | `gAppData.ramFlag = *(uint16 *)RAM_BOOT_MODE_Addr;` | 333 |
| ECC Trap 禁用 | `FLASH0_MARP.B.TRAPDIS = 1; FLASH0_MARD.B.TRAPDIS = 1;` | 338~339 |
| CAN 初始化 | `Multican_init();` | 353 |
| UDS 初始化 | `UdsInit(...);` | 354 |
| Flash 初始化 | `Flash_init();` | 359 |
| Timer 初始化 | `TMR_init();` | 362 |

---

## Phase 6: 诊断主循环（Diagnostic Main Loop）

### OEM 标准要求
- 持续处理 UDS 诊断请求
- 维持 CAN 通信和 BusOff 自动恢复
- Bootloader 主循环阶段可追溯

### 代码落实

**文件**：`App_bootloader.c`（行 380~421）

| 要求 | 代码实现 | 行号 |
|------|---------|------|
| 阶段标识 BL_MAIN | `g_bootPhase = BOOT_PHASE_BL_MAIN;` | 393 |
| UDS 主处理 | `UdsMainProcess();` | 397 |
| CAN 主处理 | `CanMainProcess();` | 398 |
| BusOff 恢复 | `CAN_NCR1.U &= ~((1<<6) | 1);` | 415 |

---

## Phase 7: 刷写会话（Programming Session）

### OEM 标准要求
- 进入 Programming Session 时记录阶段
- 刷写完成后 CRC 校验阶段可追溯
- 预编程条件检查（RID 0xFFFD）
- 擦除保护（Bootloader 区 + 非目标 Bank）
- 防自刷（禁止刷写 activeBank）

### 代码落实

**文件**：`uds_app.c`

| 子阶段 | 阶段标识 | 代码位置 | 行号 |
|--------|---------|---------|------|
| 进入 Programming Session (0x10 02) | PROG_SESSION | `g_bootPhase = BOOT_PHASE_PROG_SESSION;` | 837 |
| CRC 校验 + 标记有效 (RID 0xDFFF) | PROG_VERIFY | `g_bootPhase = BOOT_PHASE_PROG_VERIFY;` | 1814 |
| 请求下载 (0x34) | — | 防自刷检查：`if (targetWriteBank == activeBank)` → NRC 0x22 | 1450~1455 |
| 擦除内存 (RID 0xFF00) | — | Bootloader 区保护（Sector 0~7）→ NRC 0xFC | 1726~1731 |

---

## Phase 8: 跳转至新 APP（Post-Programming Jump）

### OEM 标准要求
- JumpToApp 需等正响应发送完成后再执行
- 跳转决策阶段可追溯

### 代码落实

**文件**：`uds_app.c`（行 1854~1868）

| 要求 | 代码实现 | 行号 |
|------|---------|------|
| 阶段标识 JUMP_DECISION | `g_bootPhase = BOOT_PHASE_JUMP_DECISION;` | 1856 |
| 正响应后回调跳转 | `m_pstPDUMsg->pfUDSTxMsgServiceCallBack = &DoJumpToActiveBank;` | 1867 |

**回调函数**：
```c
static void DoJumpToActiveBank(uint8 status)
{
    if (TX_MSG_SUCCESSFUL == status)
        Boot_DualBank_JumpToBank(Boot_DualBank_GetActiveBank());
}
```

---

## 全局阶段标识使用说明

### 调试与故障分析

`g_bootPhase` 是一个全局可见的枚举变量，可在调试时通过内存窗口查看：

| 值 | 阶段 | 典型场景 |
|----|------|---------|
| 0x00 | STARTUP | 刚复位，看门狗/时钟初始化中 |
| 0x01 | FLAG_INIT | DFlash 标志读取/初始化中 |
| 0x02 | BANK_VERIFY | CRC32 校验 Bank 中 |
| 0x03 | JUMP_DECISION | 判定是否跳转到 APP |
| 0x04 | JUMP_EXEC | 正在执行裸跳转到 APP |
| 0x05 | ROLLBACK | 启动失败，正在回滚 |
| 0x10 | BL_ENTRY | 进入 Bootloader 诊断模式 |
| 0x11 | BL_MAIN | Bootloader 主循环运行中 |
| 0x20 | PROG_SESSION | 0x10 02 编程会话中 |
| 0x21 | PROG_VERIFY | RID 0xDFFF CRC 校验中 |
| 0xFF | ERROR | 不可恢复错误 |

### 典型故障追溯示例

- **如果 `g_bootPhase == 0x04` 后系统死机**：说明裸跳转执行了，但 APP 未能正常运行。检查 APP 向量表偏移（`APP_INTTAB_OFFSET` / `APP_TRAPTAB_OFFSET`）是否与 APP LSL 一致。
- **如果 `g_bootPhase == 0x05` 后反复复位**：说明回滚机制触发，检查两 Bank 的 CRC 是否均无效。
- **如果 `g_bootPhase == 0x21` 后停留在 Bootloader**：说明 CRC 校验通过但后续跳转未执行，检查 UDS 0x31 02 JumpToApp 是否被调用。

---

## OEM 合规检查清单（代码级）

| 检查项 | 状态 | 代码位置 |
|--------|------|---------|
| 启动阶段关闭看门狗 | ✅ | `Cpu0_Main.c:89~90` |
| 记录复位原因 | ✅ | `Cpu0_Main.c:92~93` |
| DFlash 标志双份冗余 | ✅ | `Boot_DualBank.c:284~345` |
| Bank 切换前 CRC32 全量校验 | ✅ | `Boot_DualBank.c:360~414` |
| 启动尝试计数 + 自动回滚 | ✅ | `Boot_DualBank.c:639~740` |
| 跳转前外设去初始化（CAN / TMR） | ✅ | `Boot_DualBank.c:578~579` |
| 跳转前禁用 ECC Trap | ✅ | `Boot_DualBank.c:585~586` |
| 跳转前关闭 Cache + Barrier | ✅ | `Boot_DualBank.c:591~595` |
| **跳转前设置 APP BIV/BTV** | ✅ **新增** | `Boot_DualBank.c:532~548, 605` |
| 裸跳转（`ji`）切断 CSA 上下文 | ✅ | `Boot_DualBank.c:617~625` |
| 刷写时禁止自刷（Active Bank 保护） | ✅ | `uds_app.c:1450~1455` |
| 擦除时保护 Bootloader 区（Sector 0~7） | ✅ | `uds_app.c:1726~1731` |
| UDS 0x34 流式 CRC | ✅ | `uds_app.c:1386~` |
| UDS 0x31 0xDFFF 独立 CRC 校验 | ✅ | `uds_app.c:1757~1825` |
| Bank 标记有效后切换 Active Bank | ✅ | `Boot_DualBank.c:478~514` |
| 全局启动阶段标识（可追溯） | ✅ **新增** | 5 个文件，14 处赋值 |
| RAM 标志支持 APP 请求进 Bootloader | ✅ **启用** | `Cpu0_Main.c:108~116` |

---

## 后续建议（可选增强）

1. **看门狗在 Bootloader 主循环中使能**：当前 Bootloader 启动时关闭了看门狗。若 OEM 要求 Bootloader 模式下也有看门狗保护，可在 `AppBL_init()` 后重新使能 CPU 看门狗，并在 `AppBL_main()` 中周期性喂狗。
2. **NRC 0x78（ResponsePending）支持**：擦除大 Sector（如 128KB）耗时较长，若超过 UDS P2* 超时，应在擦除前发送 NRC 0x78。
3. **版本兼容性检查**：在 `Boot_DualBank_VerifyBank()` 中增加 APP 版本号与 Bootloader 版本号的兼容性检查，防止跳转至不兼容的 APP。
4. **DFlash 回滚事件记录**：在 `Boot_DualBank_SelectAndJump()` 中触发回滚时，将回滚原因（`ROLLBACK_REASON_VERIFY_FAIL` 等）写入 DFlash 预留区域，便于售后诊断。
