# uds_app.c 修改记录

## 修改概述

本次修改对 `uds_app.c` 进行了安全等级、NRC 返回值、算法调用、会话控制及 0x3E 服务的全面完善，确保符合 OEM 刷写流程和 UDS 标准。

---

## 1. NRC 返回值修正（Dispatcher 主循环）

**位置**：`UDS_MainFun()` 服务分发器

**问题**：Dispatcher 中使用硬编码数字 `1/2/3/4` 作为 NRC，不符合 UDS 标准。

**修改**：

| 检查项 | 旧值 | 新值（标准 UDS NRC） |
|---|---|---|
| 寻址模式不支持 | `1` | `NRC_SERVICE_NOT_SUPPORTED` (0x11) |
| 会话模式不支持 | `2` | `NRC_SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION` (0x7F) |
| 安全等级不足 | `3` | `NRC_SECURITY_ACCESS_DENIED` (0x33) |

---

## 2. 服务配置表安全等级更新

**位置**：`gs_astUDSService[]`

**修改**：

| 服务 | 会话模式 | 旧安全等级 | 新安全等级 |
|---|---|---|---|
| `0x28` CommunicationControl | `DEFALUT \| PROGRAM \| EXTEND` | 注释掉 / `NONE_SECURITY` | `SECURITY_LEVEL_1`（已取消注释） |
| `0x2E` WriteDataByIdentifier | `EXTEND \| PROGRAM` | `NONE_SECURITY` | `SECURITY_LEVEL_1` |
| `0x31` RoutineControl | `DEFALUT \| PROGRAM \| EXTEND` | `NONE_SECURITY` | `SECURITY_LEVEL_1` |
| `0x34` RequestDownload | `DEFALUT \| PROGRAM \| EXTEND` | `NONE_SECURITY` | `SECURITY_LEVEL_2` |
| `0x36` TransferData | `DEFALUT \| PROGRAM \| EXTEND` | `NONE_SECURITY` | `SECURITY_LEVEL_2` |
| `0x37` RequestTransferExit | `DEFALUT \| PROGRAM \| EXTEND` | `NONE_SECURITY` | `SECURITY_LEVEL_2` |
| `0x85` ControlDTCSetting | `DEFALUT \| PROGRAM \| EXTEND` | `NONE_SECURITY` | `SECURITY_LEVEL_1` |

---

## 3. 0x31 服务内部按例程细分安全等级

**位置**：`RoutineControl0x31()`

**设计**：配置表统一门槛为 `SECURITY_LEVEL_1`，内部对刷写相关例程额外检查 `SECURITY_LEVEL_2`。

| 例程 | Subfunction | 所需安全等级 | 修改内容 |
|---|---|---|---|
| 擦除 P-Flash | `0x01` + `0xFF00` | `LEVEL_2` | 新增 `IsCurSecurityLevelRequet(SECURITY_LEVEL_2)` 检查 |
| 擦除 D-Flash | `0x01` + `0xFF01` | `LEVEL_2` | 新增 `IsCurSecurityLevelRequet(SECURITY_LEVEL_2)` 检查 |
| 检查编程依赖 | `0x01` + `0x0203` | `LEVEL_2` | 新增 `IsCurSecurityLevelRequet(SECURITY_LEVEL_2)` 检查 |
| 跳转到 APP | `0x02` + `jumpToApp` | `LEVEL_2` | 新增 `IsCurSecurityLevelRequet(SECURITY_LEVEL_2)` 检查 |
| 其他例程（如 `jumpToBL`） | — | `LEVEL_1` | 受表级门槛控制 |

**不满足 LEVEL_2 时返回**：`NRC_SECURITY_ACCESS_DENIED` (0x33)

---

## 4. 切换到编程会话需要 SECURITY_LEVEL_1

**位置**：`DigSession0x10()`

**修改**：在 `case 0x02u`（进入 Programming Session）入口处增加安全等级检查：

```c
case 0x02u: /* Program mode */
    if (TRUE != IsCurSecurityLevelRequet(SECURITY_LEVEL_1))
    {
        SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_SECURITY_ACCESS_DENIED, m_pstPDUMsg);
        break;
    }
case 0x82u: /* Suppress positive response variant */
    SetCurrentSession(PROGRAM_SESSION);
```

**说明**：`0x82u` 作为 fall-through，同样受 `0x02u` 的检查保护。

---

## 5. Security Access 算法调用修正

**位置**：`IsReceivedKeyRight()` + `SecurityAccess0x27()`

### 5.1 `IsReceivedKeyRight()` 重构

- **旧实现**：调用 `UDS_ALG_HAL_DecryptData(i_pReceivedKey, KeyLen, aPlainText)`，然后与 Seed 比对
- **新实现**：根据 `i_SecurityLevel` 调用对应算法从 **Seed 计算 Key**，再与接收到的 Key 比对

```c
if (1u == i_SecurityLevel)
    UDS_ALG_HAL_ComputeKey_Level1(i_pTxSeed, aComputedKey);
else if (2u == i_SecurityLevel)
    UDS_ALG_HAL_ComputeKey_Level2(i_pTxSeed, aComputedKey);
// 逐字节比对 i_pReceivedKey vs aComputedKey
```

### 5.2 `SecurityAccess0x27()` 调用修正

| Subfunction | 旧调用 | 新调用 | 说明 |
|---|---|---|---|
| `0x01` 请求 Seed (Level 1) | `UDS_ALG_HAL_ComputeKey_Level1` | `UDS_ALG_HAL_GetRandom` | 请求种子应生成随机数 |
| `0x02` 发送 Key (Level 1) | `IsReceivedKeyRight(..., SA_ALGORITHM_SEED_LEN)` | `IsReceivedKeyRight(..., 1u)` | 传递安全等级而非长度 |
| `0x03` 请求 Seed (Level 2) | `UDS_ALG_HAL_ComputeKey_Level2` | `UDS_ALG_HAL_GetRandom` | 请求种子应生成随机数 |
| `0x04` 发送 Key (Level 2) | `IsReceivedKeyRight(..., SA_ALGORITHM_SEED_LEN)` | `IsReceivedKeyRight(..., 2u)` | 传递安全等级而非长度 |

---

## 6. 安全等级兼容性修正

**位置**：`IsCurSecurityLevelRequet()`

**问题**：旧逻辑 `(i_SerSecurityLevel & SecurityLevel) == SecurityLevel` 方向相反，导致解锁到 LEVEL_2 后无法访问 LEVEL_1 的服务。

**修改**：

```c
// 旧逻辑（错误）
if ((i_SerSecurityLevel & gs_stUdsInfo.SecurityLevel) == gs_stUdsInfo.SecurityLevel)

// 新逻辑（正确）
if ((gs_stUdsInfo.SecurityLevel & i_SerSecurityLevel) == i_SerSecurityLevel)
```

**安全等级定义**（已具备累加兼容性）：
- `NONE_SECURITY = 0x01`
- `SECURITY_LEVEL_1 = 0x03` (bit0+bit1)
- `SECURITY_LEVEL_2 = 0x07` (bit0+bit1+bit2)

**验证**：
- LEVEL_2 (0x07) 访问 LEVEL_1 (0x03) → `(0x07 & 0x03) == 0x03` → **TRUE** ✅
- LEVEL_1 (0x03) 访问 LEVEL_2 (0x07) → `(0x03 & 0x07) == 0x07` → **FALSE** ✅

---

## 7. 0x3E TesterPresent 车企标准完善

**位置**：`TesterPresent0x3E()`

### 7.1 新增消息长度校验

```c
if (m_pstPDUMsg->xDataLen != 2u)
{
    SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, NRC_INVALID_MESSAGE_LENGTH_OR_FORMAT, m_pstPDUMsg);
    return;
}
```

### 7.2 子功能处理

| Subfunction | 行为 | NRC |
|---|---|---|
| `0x00` | 返回肯定响应 `7E 00` | — |
| `0x80` | 抑制肯定响应，仅重置 S3Server | — |
| 其他 | — | `NRC_SUBFUNCTION_NOT_SUPPORTED` (0x12) |

### 7.3 服务表配置

- 会话：`DEFALUT_SESSION | PROGRAM_SESSION | EXTEND_SESSION`
- 地址：`SUPPORT_PHYSICAL_ADDR | SUPPORT_FUNCTION_ADDR`
- 安全：`NONE_SECURITY`

---

## 最终服务权限总览

| 服务 | 所需安全等级 | 说明 |
|---|---|---|
| `0x10` 会话控制 | `NONE` | — |
| `0x11` ECU 复位 | `NONE` | — |
| `0x23` 按地址读取 | `NONE` | — |
| `0x27` 安全访问 | `NONE` | 解锁服务本身不设限 |
| `0x28` 通信控制 | `LEVEL_1` | 诊断操作 |
| `0x2E` 写 DID | `LEVEL_1` | 写指纹等 |
| `0x31` 例程控制 | `LEVEL_1` / `LEVEL_2` | 刷写相关例程内部需 `LEVEL_2` |
| `0x34` 请求下载 | `LEVEL_2` | 刷写核心 |
| `0x36` 传输数据 | `LEVEL_2` | 刷写核心 |
| `0x37` 传输退出 | `LEVEL_2` | 刷写核心 |
| `0x85` DTC 控制 | `LEVEL_1` | 诊断操作 |
| `0x3E` 诊断仪在线 | `NONE` | 维持会话，车企标准 |

---

## 刷写流程标准时序

```
10 03 扩展会话 → 28 控制通信（需 LEVEL_1）
27 01/02 解锁 LEVEL_1 → 2E 写指纹（需 LEVEL_1）
10 02 编程会话（需 LEVEL_1） → 27 03/04 解锁 LEVEL_2
31 01 FF00 擦除 + 34/36/37 下载（需 LEVEL_2）
31 01 0203 依赖检查 + 31 02 跳转（需 LEVEL_2）
```


---

## 8. A/B 双区刷写流程 — APP 跳转 Bootloader 机制

### 8.1 问题背景

双区 Bootloader 架构下，刷写流程需要在 **APP 环境** 中完成预操作（写指纹、DTC 控制等），然后切换到 **Bootloader 环境** 执行实际的擦除/下载/校验。需要解决三个核心问题：

1. APP 收到 `10 02` 后如何可靠跳转到 Bootloader
2. Bootloader 启动时如何区分"正常启动"和"APP 请求刷写"
3. 下载时如何确定目标 Bank（A 或 B）

### 8.2 通信机制：RAM 标志

| 地址 | 标志值 | 含义 |
|---|---|---|
| `0x7002Dffc` (`RAM_BOOT_MODE_Addr`) | `0xa4a5` (`RAM_BOOT_MODE_APP`) | APP 请求进入 Bootloader 刷写模式 |
| `0x7002Dffc` | `0xb2b3` (`RAM_BOOT_MODE_NORMAL`) | 正常上电，Bootloader 尝试跳转 APP |
| `0x7002Dffc` | `0xf0b1` (`RAM_BOOT_MODE_KEEP`) | 强制停在 Bootloader（调试用） |

**关键特性**：RAM 在软复位后保持数据（TC234 DSRAM0 不掉电），因此 APP 设置标志 → 软复位 → Bootloader 读取标志 的跨程序通信是可靠的。

### 8.3 修改 1：APP 端 `10 02` 触发跳转

**位置**：`uds_app.c` → `DigSession0x10()` → `case 0x02u`

**修改内容**：

在 APP 编译条件（`DIAGNOSTIC_MODE_FOR_APP`）下，`10 02` 处理不再是简单的会话切换，而是设置 RAM 标志并通过 UDS Tx 回调在肯定响应发送完成后触发软复位。

```c
case 0x02u: /* Program mode */
    if (TRUE != IsCurSecurityLevelRequet(SECURITY_LEVEL_1))
    {
        SetNegativeErroCode(..., NRC_SECURITY_ACCESS_DENIED, ...);
        break;
    }
    else
    {
        m_pstPDUMsg->aDataBuf[0u] = i_pstUDSServiceInfo->SerNum + 0x40u;  /* 0x50 */
        m_pstPDUMsg->aDataBuf[1u] = RequestSubfunction;                    /* 0x02 */
        m_pstPDUMsg->xDataLen = 2u;
        SetCurrentSession(PROGRAM_SESSION);
#ifdef DIAGNOSTIC_MODE_FOR_APP
        /* APP mode: set bootloader flag and reset after positive response is sent */
        *(uint16*)RAM_BOOT_MODE_Addr = RAM_BOOT_MODE_APP;
        m_pstPDUMsg->pfUDSTxMsgServiceCallBack = &DoResetToBootloader;
#endif
    }
```

**新增 Tx 回调**（`uds_app.c`）：

```c
#ifdef DIAGNOSTIC_MODE_FOR_APP
static void DoResetToBootloader(uint8 status)
{
    if (TX_MSG_SUCCESSFUL == status)
    {
        SW_Reset();  /* CAN 发送完成后触发软复位 */
    }
}
#endif
```

**时序**：
```
APP 收到 10 02 → 设置 RAM_BOOT_MODE_APP → 发送肯定响应 50 02
     ↓
CAN 总线发送完成 → 调用 DoResetToBootloader() → SW_Reset()
     ↓
Bootloader 启动
```

### 8.4 修改 2：Bootloader 启动时 RAM 标志检测

**位置**：`Cpu0_Main.c` → `core0_main()`

**修改内容**：

在 `Boot_DualBank_Init()` 之后、`Boot_DualBank_SelectAndJump()` 之前，插入 RAM 标志检测逻辑。

```c
/* Dual Bank: initialize flag system */
Boot_DualBank_Init();

/* Check if APP requested bootloader mode via RAM flag */
{
    uint16 ramBootMode = *(uint16 *)RAM_BOOT_MODE_Addr;
    if (ramBootMode == RAM_BOOT_MODE_APP)
    {
        /* APP requested bootloader: clear flag and stay in bootloader for flashing */
        *(uint16 *)RAM_BOOT_MODE_Addr = RAM_BOOT_MODE_NORMAL;
    }
    else
    {
        /* Normal boot: attempt to jump to active bank */
        Boot_DualBank_SelectAndJump();
        /* If SelectAndJump() returns, both banks are invalid -> stay in bootloader */
    }
}
```

**行为**：
- RAM 标志 = `0xa4a5` → 清标志 → **跳过 `SelectAndJump()`** → 留在 Bootloader
- RAM 标志 = 其他值 → 执行 `SelectAndJump()` → 尝试跳转当前 active Bank 的 APP

### 8.5 修改 3：下载目标 Bank 自动判定

**位置**：`uds_app.c` → `RequestDownload0x34()`

**已有逻辑**（无需额外修改，已具备）：

```c
gs_stDowloadDataInfo.StartAddr = (addr & 0x00FFFFFF) | 0xA0000000;

uint32 cachedAddr = gs_stDowloadDataInfo.StartAddr - 0x20000000u;
if ((cachedAddr >= BANK_B_START_ADDR) && (cachedAddr < BANK_B_END_ADDR))
    g_udsTargetBank = BANK_B;
else
    g_udsTargetBank = BANK_A;
```

**判定规则**：

| 诊断仪发送的下载地址 | 实际 PFlash 地址 | 目标 Bank |
|---|---|---|
| `80 02 00 00` | `0x80020000` | **Bank A**（S8~S22） |
| `80 10 00 00` | `0x80100000` | **Bank B**（S23~S26） |

**安全保护**：`RequestDownload0x34()` 中已加入"禁止覆盖当前运行区"检查：

```c
if (g_udsTargetBank == Boot_DualBank_GetActiveBank())
{
    SetNegativeErroCode(..., NRC_CONDITIONS_NOT_CORRECT, ...);
    Ret = FALSE;
}
```

### 8.6 双区刷写完整时序

```
【APP 阶段】
10 01 默认会话
10 03 扩展会话
27 01/02 解锁 LEVEL_1
31 01 DF FD  检查编程依赖（APP 自定义例程）
85 02        关闭 DTC
10 02 编程会话 → APP 设置 RAM_BOOT_MODE_APP → 发送 50 02 → SW_Reset()

【Bootloader 阶段】
Bootloader 启动 → 检测到 RAM_BOOT_MODE_APP → 留在 Bootloader
27 03/04 解锁 LEVEL_2
2E F1 5A 写指纹
31 01 FF 00 擦除 P-Flash（目标 Bank）
34/36/37   下载 APP 到目标 Bank
31 01 FF 01 检查 APP 有效性（计算 CRC）
31 01 0203 依赖检查 → MarkBankValid + SetActiveBank
11 01 ECU 硬复位

【下次启动】
Bootloader 启动 → 无 RAM 标志 → SelectAndJump() → 跳转到新 Bank
```

### 8.7 DFlash 标志结构（双区管理）

`Boot_DualBank.c` 使用 DFlash 持久化存储双区状态：

```c
typedef struct
{
    uint32 magic;           /* 0x5A5AA5A5 */
    uint32 activeBank;      /* BANK_A (0) or BANK_B (1) */
    uint32 bankA_valid;     /* CRC32 of Bank A, 0 = invalid */
    uint32 bankB_valid;     /* CRC32 of Bank B, 0 = invalid */
    uint32 bankA_version;
    uint32 bankB_version;
    uint16 bootAttempts;    /* 连续启动失败计数 */
    uint16 flags;
    uint32 sequence;        /* 写操作序列号 */
    uint32 crc32;           /* 标志区自身 CRC */
} BootFlagMain_t;
```

**关键接口**：
- `Boot_DualBank_MarkBankValid(bank, version)` — 刷写完成后计算并写入 CRC
- `Boot_DualBank_SetActiveBank(bank)` — 设置下次启动的 active bank（不立即复位）
- `Boot_DualBank_VerifyBank(bank)` — 计算实际 CRC 并与存储值比对
- `Boot_DualBank_SelectAndJump()` — 启动时选择有效 Bank 并跳转

---

## 修改文件清单

| 文件 | 修改内容 |
|---|---|
| `App_UDS/uds_app.c` | NRC 修正、服务表安全等级、0x31 内部 LEVEL_2 检查、10 02 APP 跳转、0x3E 完善、算法调用修正、安全等级兼容性 |
| `Main/Cpu0_Main.c` | 启动时 RAM 标志检测，区分正常启动与 APP 请求刷写 |
| `App_bootloader/Boot_DualBank.c/h` | 双区管理、CRC 校验、Bank 切换、启动跳转（已有，未修改） |
| `App_UDS/uds_app_changes.md` | 本修改说明文档 |


---

## 9. A/B 双区 Bootloader bootAttempts 机制修复

### 9.1 问题背景

`Boot_DualBank_SelectAndJump()` 在跳转 APP **之前**会递增 `bootAttempts` 计数器：

```c
flags.main.bootAttempts++;
Boot_DualBank_WriteFlags(&flags);
Boot_DualBank_JumpToBank(targetBank);  /* 跳转后不再返回 Bootloader */
```

**缺失的环节**：APP 成功启动后没有任何接口来清零该计数器。导致每次复位都会再 +1，达到 `MAX_BOOT_ATTEMPTS`（3 次）后，Bootloader 会误判为"该 Bank 连续启动失败"，触发误回滚。

### 9.2 修复内容

| 文件 | 修改 |
|---|---|
| `App_bootloader/Boot_DualBank.h` | 新增 `Boot_DualBank_ClearBootAttempts(void)` 声明 |
| `App_bootloader/Boot_DualBank.c` | 新增实现：读取 DFlash flags → `bootAttempts = 0` → 写回 DFlash |

**新增接口声明**（`Boot_DualBank.h`）：

```c
/**
 * @brief Clear boot attempt counter after successful APP startup.
 * @note  Must be called by APP in its early initialization phase.
 *        If APP crashes (trap/WDT) before calling this, the counter
 *        remains non-zero and Bootloader will increment it on next boot.
 *        After MAX_BOOT_ATTEMPTS consecutive failures, Bootloader rolls
 *        back to the fallback bank.
 */
void Boot_DualBank_ClearBootAttempts(void);
```

**新增接口实现**（`Boot_DualBank.c`）：

```c
void Boot_DualBank_ClearBootAttempts(void)
{
    DualBankFlags_t flags;
    if (Boot_DualBank_ReadFlags(&flags) == TRUE)
    {
        flags.main.bootAttempts = 0u;
        flags.main.sequence++;
        Boot_DualBank_WriteFlags(&flags);
    }
}
```

### 9.3 APP 调用要求

**调用位置**：APP 工程 `main()` 或 `AppInit()` 的**最早期**，必须在任何可能导致崩溃的代码之前。

```c
void app_main(void)
{
    /* 1. 最基本硬件初始化（时钟、中断等） */
    McuInit();
    
    /* 2. 立即通知 Bootloader：本次启动成功 */
    Boot_DualBank_ClearBootAttempts();
    
    /* 3. 后续初始化（CAN、UDS、DTC、外设等） */
    CanInit();
    UdsInit();
    ...
}
```

### 9.4 监控机制说明

Bootloader **不直接监控** APP（跳转后 Bootloader 代码不再运行），而是通过"计数器是否被清零"**事后推断**：

| 场景 | bootAttempts 状态 | Bootloader 下次行为 |
|---|---|---|
| APP 启动成功并调用 `ClearBootAttempts()` | 0 | 0 → +1 → 正常跳转 |
| APP 启动后崩溃（trap / 看门狗） | 1（或更高） | 1 → +1 = 2 → 跳转 |
| APP 再次崩溃 | 2 | 2 → +1 = 3 → **达到阈值，回滚到另一 Bank** |

**阈值**：`MAX_BOOT_ATTEMPTS = 3`，即连续 **3 次启动失败** 后自动回滚。

### 9.5 bootAttempts 完整生命周期

```
Bootloader 启动
    │
    ▼
读取 flags → bootAttempts = ?
    │
    ├── bootAttempts >= 3 ──► 判定该 Bank 不稳定
    │                          无效化当前 Bank
    │                          切换 activeBank 到另一 Bank
    │                          bootAttempts = 0
    │                          SW_Reset()
    │
    └── bootAttempts < 3 ──► bootAttempts++
                              写回 DFlash
                              Boot_DualBank_JumpToBank()
                                  │
                                  ▼
                              APP 初始化成功
                                  │
                                  ▼
                              Boot_DualBank_ClearBootAttempts()
                              bootAttempts = 0（写回 DFlash）
```

---

## 10. 修改文件总清单

| 文件 | 修改内容 |
|---|---|
| `App_UDS/uds_app.c` | NRC 修正、服务表安全等级、0x31 内部 LEVEL_2 检查、10 02 APP 跳转 Bootloader、0x3E 完善、算法调用修正、安全等级兼容性 |
| `Main/Cpu0_Main.c` | 启动时 RAM 标志检测，区分正常启动与 APP 请求刷写 |
| `App_bootloader/Boot_DualBank.h` | 新增 `Boot_DualBank_ClearBootAttempts()` 声明 |
| `App_bootloader/Boot_DualBank.c` | 实现 `Boot_DualBank_ClearBootAttempts()`，修复 bootAttempts 机制 |
| `App_UDS/uds_app_changes.md` | 本修改说明文档 |
