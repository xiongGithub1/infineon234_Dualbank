# UDS A/B Dual Bank 刷写改造说明

## 1. 改造概述

本次改造将 Bootloader 的 UDS 诊断刷写流程从**单分区覆盖写**升级为**车企标准 A/B 双分区滚动升级**。

### 核心变化

| 项目 | 改造前（单分区） | 改造后（A/B 双分区） |
|------|------------------|----------------------|
| 刷写目标 | 只能刷 Bank A（S8-S22） | 可刷 Bank A 或 Bank B |
| 运行 Bank 保护 | 无 | `0x34` 拒绝覆盖当前运行 Bank |
| 刷写后激活 | 写 S6 标志 `0xA55A` + `0x11 03` 复位 | `0x31 01 0203` 校验+标记+切 Bank + `0x11 03` 复位 |
| 版本回滚 | 需从 Bank B 恢复数据到 Bank A | 自动：启动失败 3 次回滚到旧 Bank |
| APP 跳转 | 固定跳 `0x80020020` | 动态跳 `activeBank` |

---

## 2. 车企标准 UDS 刷写流程（A/B 双分区）

### 2.1 标准刷写时序（上位机视角）

```
阶段 1：预编程（扩展会话）
├── 10 03              → 进入 Extended Session
├── 85 02              → 关闭 DTC 记录（ControlDTCSetting - off）
├── 27 01              → 请求 Seed
├── 27 02 Key          → 解锁安全访问
├── 2E F15A ...        → 写入刷写设备指纹

阶段 2：编程（擦除 + 下载）
├── 31 01 FF00 HH LL   → 擦除目标 Bank 的 Sector（如 S23-S26）
│                        HH LL = Block Number（如 00 17 = S23）
│                        返回：71 01 FF00 Block Result
├── 34 00 44 addr len  → RequestDownload
│                        addr = 目标 Bank 起始地址（如 0x80100000）
│                        返回：74 10 80（允许下载，maxBlock=0x80）
├── 36 01 data...      → TransferData（第 1 帧）
├── 36 02 data...      → TransferData（第 2 帧）
│   ...
├── 37                 → RequestTransferExit
│                        返回：77

阶段 3：后编程（校验 + 激活 + 复位）
├── 31 01 0203         → CheckProgrammingDependency
│                        内部执行：VerifyBank CRC → MarkValid → SetActiveBank
│                        返回：71 01 0203 01（01=成功，00=失败）
├── 11 03              → SoftReset
│                        Bootloader 复位后读取 DFlash 标志
│                        自动跳转到新激活的 Bank
```

> **关键点**：`0x31 01 0203` 现在是一个"原子操作"：校验通过 → 标记目标 Bank 有效 → 将 `activeBank` 设为目标 Bank。随后 `0x11 03` 软复位，Bootloader 自动启动新 Bank。

---

## 3. 文件修改详情

### 3.1 `Boot_DualBank.h` / `Boot_DualBank.c`（新增接口）

#### 新增函数：`Boot_DualBank_SetActiveBank()`

```c
/**
 * @brief Set active bank flag without triggering reset.
 * @note  Used in UDS CheckProgrammingDependency (0x31 01 0203) to prepare
 *        for subsequent ECU reset (0x11 03). The new bank takes effect
 *        after the next reset.
 */
void Boot_DualBank_SetActiveBank(uint32 targetBank);
```

**用途**：
- `Boot_DualBank_SwitchBank()` 会**立即触发软复位**，适合在需要立即重启的场景使用
- `Boot_DualBank_SetActiveBank()` **只修改 DFlash 标志，不复位**，适合在 UDS `0x31 01 0203` 中使用（先发送 UDS 响应，再由上位机发 `0x11 03` 复位）

**实现**：
```c
void Boot_DualBank_SetActiveBank(uint32 targetBank)
{
    DualBankFlags_t flags;
    if (Boot_DualBank_ReadFlags(&flags) == TRUE)
    {
        flags.main.activeBank   = targetBank;
        flags.main.bootAttempts = 0u;        /* 新 Bank，启动计数清零 */
        flags.main.sequence++;
        Boot_DualBank_WriteFlags(&flags);
        g_activeBank = targetBank;
    }
}
```

---

### 3.2 `uds_app.c`（核心改造）

#### 修改点 1：包含双分区头文件 + 全局变量

```c
#include "Boot_DualBank.h"

/* Target bank for current UDS download session (A/B Dual Bank support) */
static uint8 g_udsTargetBank = BANK_A;
```

**用途**：
- 在 `0x34 RequestDownload` 时根据下载地址判断目标 Bank
- 在 `0x36 TransferData` 时继续写入该 Bank
- 在 `0x31 01 0203` 时对该 Bank 做校验和激活

---

#### 修改点 2：`RequestDownload0x34()` —— 目标 Bank 判断 + 运行 Bank 保护

```c
static void RequestDownload0x34(struct UDSServiceInfo *i_pstUDSServiceInfo,
        tUdsAppMsgInfo *m_pstPDUMsg)
{
    /* ... 原有地址解析逻辑 ... */
    
    gs_stDowloadDataInfo.StartAddr = 
        (gs_stDowloadDataInfo.StartAddr & 0x00FFFFFF) | 0xA0000000;

    /* ===== 新增：Dual Bank 目标判断 ===== */
    {
        uint32 cachedAddr = gs_stDowloadDataInfo.StartAddr - 0x20000000u;
        if ((cachedAddr >= BANK_B_START_ADDR) &&
            (cachedAddr < BANK_B_END_ADDR))
        {
            g_udsTargetBank = BANK_B;
        }
        else
        {
            g_udsTargetBank = BANK_A;
        }
    }

    /* Safety check: refuse to overwrite the currently running bank */
    if (g_udsTargetBank == Boot_DualBank_GetActiveBank())
    {
        SetNegativeErroCode(i_pstUDSServiceInfo->SerNum, 
                            NRC_CONDITIONS_NOT_CORRECT, m_pstPDUMsg);
        Ret = FALSE;
    }
    /* =================================== */
    
    /* ... 原有地址合法性检查 ... */
}
```

**关键逻辑**：
1. `StartAddr` 被强制转换为 uncached（`0xA0...`），因此比较时需要转回 cached：`cachedAddr = StartAddr - 0x20000000`
2. 若地址落在 Bank B 范围（`0x80100000 ~ 0x80200000`）→ `g_udsTargetBank = BANK_B`
3. **安全保护**：若目标 Bank == 当前运行 Bank（`Boot_DualBank_GetActiveBank()`），返回 NRC `0x22`（Conditions Not Correct）

> **为什么必须保护运行中的 Bank？**  
> A/B 双分区的核心原则是"永远只刷写非运行 Bank"。如果允许覆盖当前运行 Bank，一旦刷写过程中断（断电/通信失败），ECU 将无可用固件，变砖。

---

#### 修改点 3：`RoutineControl0x31()` —— `0x0203` 校验+激活改造

```c
case 0x0203: /* Check Programming Dependency (A/B Dual Bank) */
{
    BankStatus_t status = Boot_DualBank_VerifyBank(g_udsTargetBank);
    if (status == BANK_STATUS_VALID)
    {
        /* 1. Mark target bank as valid (compute & store CRC32) */
        Boot_DualBank_MarkBankValid(g_udsTargetBank, 0x00010000u);
        
        /* 2. Set active bank to target (takes effect after next reset) */
        Boot_DualBank_SetActiveBank(g_udsTargetBank);
        
        routineResult = 0x01; /* success */
    }
    else
    {
        routineResult = 0x00; /* fail: CRC mismatch or empty bank */
    }
    
    /* Positive response: 71 01 0203 Result */
    m_pstPDUMsg->aDataBuf[0] = 0x71;
    m_pstPDUMsg->aDataBuf[1] = 0x01;
    m_pstPDUMsg->aDataBuf[2] = 0x02;
    m_pstPDUMsg->aDataBuf[3] = 0x03;
    m_pstPDUMsg->aDataBuf[4] = (uint8)routineResult;  /* 01=OK, 00=FAIL */
    m_pstPDUMsg->xDataLen = 5;
    break;
}
```

**三步原子操作**：
1. **VerifyBank**：计算目标 Bank 实际 384KB 内容的 CRC32，与标志区存储值比对
2. **MarkBankValid**：计算并存储 CRC32，写入 `bankX_valid` 和版本号
3. **SetActiveBank**：修改 `activeBank` 标志，清 `bootAttempts`，递增 `sequence`

**响应格式**：
- `71 01 0203 01` → 校验通过，Bank 已激活，请发送 `11 03` 复位
- `71 01 0203 00` → 校验失败（CRC 不匹配或 Bank 为空），上位机应终止刷写流程

---

#### 修改点 4：`RoutineControl0x31()` —— `0x31 02 jumpToApp` 改造

```c
case jumpToApp: /* Jump to active APP bank (Dual Bank) */
{
    m_pstPDUMsg->aDataBuf[0] = 0x71;
    m_pstPDUMsg->aDataBuf[1] = 0x02;
    m_pstPDUMsg->aDataBuf[2] = jumpToApp;
    m_pstPDUMsg->xDataLen = 3;

    /* Send positive response first, then jump via callback */
    m_pstPDUMsg->pfUDSTxMsgServiceCallBack = &DoJumpToActiveBank;
    break;
}
```

**关键设计**：
- 旧逻辑：设置 RAM 标志 `RAM_BOOT_MODE_NORMAL`，依赖 `AppBL_main()` 中的轮询跳转
- 新逻辑：直接注册 UDS 发送完成回调 `DoJumpToActiveBank`，响应发出后立即跳转
- 跳转目标：由 `Boot_DualBank_GetActiveBank()` 动态决定（Bank A 或 Bank B）

**回调函数**：
```c
static void DoJumpToActiveBank(uint8 status)
{
    if (TX_MSG_SUCCESSFUL == status)
    {
        Boot_DualBank_JumpToBank(Boot_DualBank_GetActiveBank());
    }
}
```

---

#### 修改点 5：`DoResetMCU0x11()` —— SoftReset 改造

```c
case SOFT_RESET:
    /* Dual Bank: no need to write legacy S6 flag; 
     * ECU reset will trigger Boot_DualBank_SelectAndJump() */
    SW_Reset();
    break;
```

**变化**：
- 旧逻辑：`readFlagS6()` 写入 PFlash S6 标志 `0xA55A`，然后复位
- 新逻辑：直接 `SW_Reset()`，复位后 `Boot_DualBank_SelectAndJump()` 读取 DFlash 标志决定启动 Bank
- 旧单分区标志 `FL_APP_FLAG_Addr`（S6）不再使用

---

### 3.3 `uds_app_cfg.c` —— `DoCheckProgrammingDependency()` 说明

当前实现：
```c
uint8 DoCheckProgrammingDependency(void)
{
    return TRUE;  /* 已废弃，真正的校验逻辑移到 uds_app.c RoutineControl0x31 中 */
}
```

**说明**：
- 原有 `DoCheckProgrammingDependency()` 被 `0x31 01 0203` 调用
- 改造后，`0x0203` 直接调用 `Boot_DualBank_VerifyBank()` + `MarkBankValid()` + `SetActiveBank()`
- 保留该函数仅用于兼容旧代码和编译链接，实际不再被调用

---

## 4. 上位机（诊断仪）刷写脚本示例

### 4.1 刷写 Bank B（当前运行 Bank A）

```python
# 伪代码：诊断仪刷写流程（车企标准三阶段）

# ===== 阶段 1：预编程（扩展会话） =====
uds.send([0x10, 0x03])                    # 进入 Extended Session
uds.send([0x85, 0x02])                    # 关闭 DTC 记录
uds.send([0x27, 0x01]); key = calc_key(uds.send([0x27, 0x02, ...]))  # 解锁
uds.send([0x2E, 0xF1, 0x5A] + fingerprint)  # 写入刷写设备指纹

# ===== 阶段 2：编程（擦除 + 下载） =====
# 擦除 Bank B 的 Sector S23-S26
for block in [0x17, 0x18, 0x19, 0x1A]:   # S23, S24, S25, S26
    resp = uds.send([0x31, 0x01, 0xFF, 0x00, 0x00, block])
    assert resp[5] == 0x01                 # 擦除成功

# 请求下载到 Bank B（地址 0x80100000，长度 0x00060000 = 384KB）
resp = uds.send([0x34, 0x00, 0x44, 
                 0x80, 0x10, 0x00, 0x00,   # addr = 0x80100000
                 0x00, 0x06, 0x00, 0x00])  # len = 384KB
assert resp[0] == 0x74

# 传输数据（循环发送 0x36）
block_num = 1
for chunk in firmware_chunks:
    resp = uds.send([0x36, block_num] + chunk)
    assert resp[0] == 0x76
    block_num = (block_num + 1) & 0xFF

# 传输退出
resp = uds.send([0x37])
assert resp[0] == 0x77

# ===== 阶段 3：后编程（校验 + 复位） =====
# 校验并激活新 Bank
resp = uds.send([0x31, 0x01, 0x02, 0x03])
assert resp[0:4] == [0x71, 0x01, 0x02, 0x03]
assert resp[4] == 0x01                    # 01 = 校验通过且已激活

# 软复位，启动新固件
resp = uds.send([0x11, 0x03])
assert resp[0:2] == [0x51, 0x03]
```

### 4.2 刷写 Bank A（当前运行 Bank B）

流程完全相同，只是 `0x34` 的地址改为 `0x80020000`：
```python
resp = uds.send([0x34, 0x00, 0x44, 
                 0x80, 0x02, 0x00, 0x00,   # addr = 0x80020000 (Bank A)
                 0x00, 0x06, 0x00, 0x00])
```

> **注意**：若诊断仪错误地发送了当前运行 Bank 的地址，`0x34` 将返回 NRC `0x22`（Conditions Not Correct）。上位机应解析此错误并提示用户"不能覆盖当前运行分区"。

---

## 5. 异常处理与负响应码

| 场景 | NRC | 说明 |
|------|-----|------|
| 刷写地址指向当前运行 Bank | `0x22` | Conditions Not Correct |
| 刷写地址超出 PFlash 范围 | `0x31` | Request Out Of Range |
| `0x31 01 0203` 校验失败 | `71 01 0203 00` | 正响应中携带失败标志 |
| 传输序列错误（0x36/0x37 乱序）| `0x24` | Request Sequence Error |
| 刷写会话未解锁 | `0x33` | Security Access Denied |

---

## 6. 调试与验证

### 6.1 验证刷写 Bank B 流程

1. **确保当前运行 Bank A**：
   - 调试器查看 `0xAF000000`：`activeBank` 应为 `0`

2. **发送 `0x34` 请求下载到 Bank B**：
   - 地址：`0x80100000`（cached）→ Bootloader 内部转为 `0xA0100000`
   - 预期：`g_udsTargetBank = BANK_B`（1）
   - 预期：正响应 `74 10 80`

3. **发送 `0x36` 传输数据**：
   - 预期：数据写入 `0xA0100000` 起（Bank B 物理地址）

4. **发送 `0x31 01 0203`**：
   - 预期：Bootloader 计算 Bank B 的 CRC32
   - 预期：写入 DFlash `bankB_valid` = CRC32
   - 预期：写入 DFlash `activeBank` = 1
   - 正响应：`71 01 0203 01`

5. **发送 `0x11 03`**：
   - 预期：ECU 软复位
   - 复位后：`Boot_DualBank_SelectAndJump()` 读取 `activeBank=1`
   - 预期：跳转到 Bank B（`0x80100000`）

### 6.2 验证运行 Bank 保护

1. 当前运行 Bank A（`activeBank = 0`）
2. 发送 `0x34` 地址 `0x80020000`（Bank A）
3. 预期：负响应 `7F 34 22`（NRC Conditions Not Correct）

### 6.3 验证回滚

1. 刷写 Bank B 并激活
2. 运行 Bank B
3. 手动破坏 Bank B（如擦除 Sector 23 的前几个 bytes）
4. 复位
5. 预期：`Boot_DualBank_VerifyBank(BANK_B)` 检测到 CRC 不匹配
6. 预期：自动回滚到 Bank A（若 Bank A 仍有效）

---

## 7. 与 ZCANPRO / ZXDoc 工具的兼容性

### 7.1 已有流程兼容性

| 步骤 | ZCANPRO 标准流程 | 兼容性 |
|------|------------------|--------|
| 10 02 | 进入编程会话 | ✅ 不变 |
| 31 01 FF00 | 擦除内存 | ✅ 不变 |
| 34/36/37 | 下载固件 | ✅ 不变（地址由上位机决定） |
| **31 01 0203** | 检查依赖性 | ⚠️ **改造后行为变化**：现在会自动激活新 Bank |
| 11 03 | 软复位 | ✅ 不变 |

**对 ZCANPRO 的影响**：
- ZCANPRO 的刷写脚本无需修改！它仍然发送 `31 01 0203` 和 `11 03`
- 但 `31 01 0203` 现在会额外执行：标记 Bank 有效 + 切换 activeBank
- 刷写完成后复位，自动启动新 Bank

### 7.2 刷写 Bank B 的注意事项

- ZCANPRO 默认刷写地址通常是 `0x80020000`（Bank A）
- 若要刷 Bank B，需在 ZCANPRO 的刷写配置中将起始地址改为 `0x80100000`
- 擦除步骤中，Block Number 需改为 Bank B 对应的 Sector：S23=23, S24=24, S25=25, S26=26

---

## 8. 后续可选扩展

### 8.1 添加 DID 读取 Bank 状态

可在 `0x22 ReadDataByIdentifier` 中添加：
- `F181`：Bank A 版本号
- `F182`：Bank B 版本号
- `F183`：Active Bank + BootAttempts + BankA/B 有效状态

### 8.2 添加独立的 Bank 切换 Routine

如需支持"不刷写只切换 Bank"：
- 新增 `0x31 01 0204`：SwitchToAlternateBank
- 内部调用 `Boot_DualBank_SwitchBank(fallbackBank)` 直接复位

### 8.3 APP 启动成功后清 bootAttempts

当前 `bootAttempts` 在 Bootloader 跳转前递增，但 APP 成功运行后不会自动清零。

**建议方案**：在 APP 的 `main()` 中，初始化稳定后写 DFlash 清零：
```c
/* APP 侧代码 */
void APP_ClearBootAttempts(void)
{
    DualBankFlags_t flags;
    if (Boot_DualBank_ReadFlags(&flags) == TRUE)
    {
        flags.main.bootAttempts = 0;
        flags.main.sequence++;
        Boot_DualBank_WriteFlags(&flags);
    }
}
```

或者通过 UDS 服务（如 `0x3E TesterPresent` 在 APP 会话中定期清零）。

---

## 9. 修改文件汇总

| 文件 | 修改内容 |
|------|----------|
| `Boot_DualBank.h` | 声明 `Boot_DualBank_SetActiveBank()` |
| `Boot_DualBank.c` | 实现 `Boot_DualBank_SetActiveBank()` |
| `uds_app.c` | 核心 UDS 改造： |
| | ① `0x27` 会话配置：`PROGRAM_SESSION` → `EXTEND_SESSION \| PROGRAM_SESSION`（支持预编程阶段解锁） |
| | ② `0x2E` 会话配置：`DEFALUT_SESSION` → `EXTEND_SESSION \| PROGRAM_SESSION`（支持预编程阶段写指纹） |
| | ③ `0x34` Bank 判断 + 运行 Bank 保护 |
| | ④ `0x31 0x0203` 校验激活（VerifyBank → MarkValid → SetActiveBank） |
| | ⑤ `0x31 02 jumpToApp` 动态跳转 |
| | ⑥ `0x11 SoftReset` 移除旧 S6 标志逻辑 |
| `uds_app_cfg.c` | `DoCheckProgrammingDependency()` 已废弃（保留兼容） |

---

## 10. 总结

本次 UDS 改造实现了**车企标准 A/B 双分区刷写激活流程**：

1. **运行 Bank 保护**：`0x34` 自动识别目标 Bank，拒绝覆盖当前运行 Bank
2. **刷写后一键激活**：`0x31 01 0203` 完成校验→标记→切换三步原子操作
3. **动态跳转**：`0x31 02 jumpToApp` 跳转到 `activeBank`，不再固定地址
4. **复位即生效**：`0x11 03` 复位后 Bootloader 自动读取 DFlash 标志启动新 Bank

**与现有上位机工具的兼容性**：
- ZCANPRO/ZXDoc 标准刷写流程**无需修改**
- 只需将刷写地址从 `0x80020000` 改为 `0x80100000` 即可刷 Bank B
- `0x31 01 0203` 和 `0x11 03` 的行为与旧流程一致，但内部实现了双分区激活
