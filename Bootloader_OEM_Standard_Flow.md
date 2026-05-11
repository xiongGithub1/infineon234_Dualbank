# Bootloader 车企标准流程化文档（OEM Standard Flow）

> **适用范围**：TC234 Dual-Bank Bootloader  
> **基准文件**：`Cpu0_Main.c` / `App_bootloader.c` / `Boot_DualBank.c` / UDS Stack  
> **规范依据**：ISO 14229 (UDS)、ISO 15765 (CAN-TP)、OEM Bootloader 通用安全规范

---

## 1. 总体流程概览

```
┌─────────────────┐
│     RESET       │
└────────┬────────┘
         ▼
┌─────────────────────────────┐
│  Phase 1: 系统启动 (Startup) │  ← 关看门狗、初始化时钟
└────────┬────────────────────┘
         ▼
┌─────────────────────────────┐
│  Phase 2: 标志初始化 (Flags) │  ← DFlash 双份冗余校验
└────────┬────────────────────┘
         ▼
┌─────────────────────────────┐
│  Phase 3: Bank 有效性验证    │  ← CRC32 全 Bank 校验
└────────┬────────────────────┘
         ▼
    ┌────────┐
    │ Valid? │
    └────┬───┘
   YES   │   NO
    ┌────┘    └────┐
    ▼              ▼
┌──────────┐  ┌──────────────┐
│ Phase 4A │  │  Phase 4B    │
│ 跳转决策  │  │ 回滚/容错处理 │
└────┬─────┘  └──────┬───────┘
     │               │
     ▼               ▼
┌──────────┐  ┌──────────────┐
│跳转到APP │  │ 进入Bootloader│
│(永不返回)│  │   主循环      │
└──────────┘  └──────┬───────┘
                     ▼
            ┌────────────────┐
            │  Phase 5: 诊断  │
            │  与刷写会话     │
            └───────┬────────┘
                    ▼
            ┌────────────────┐
            │  Phase 6: 跳转  │
            │  至新 APP      │
            └────────────────┘
```

---

## 2. 详细阶段定义

### Phase 1: 系统启动（System Startup）
**入口条件**：CPU Reset 向量表生效，PC 指向 `_START`  
**目标**：建立最小可运行系统环境

| 步骤 | 操作 | OEM 要求 | 代码位置 |
|------|------|---------|---------|
| 1.1 | **禁用 CPU 看门狗** | 防止启动过程中意外复位 | `IfxScuWdt_disableCpuWatchdog()` |
| 1.2 | **禁用 Safety 看门狗** | 同上，覆盖 Safety 域 | `IfxScuWdt_disableSafetyWatchdog()` |
| 1.3 | **系统时钟初始化** | PLL 锁定，CPU/System/SPB/STM 频率就绪 | `IfxScuClock_init()` |
| 1.4 | **延时子系统初始化** | 供后续 Flash 操作时序使用 | `delay_init()` |
| 1.5 | **板级指示初始化** | LED 等诊断指示可用 | `BrdLed_init()` |

**出口条件**：时钟稳定、看门狗禁用、板级指示可用  
**异常处理**：若 PLL 未锁定，应进入安全状态（死循环或复位）

---

### Phase 2: 双 Bank 标志初始化（Dual-Bank Flag Init）
**入口条件**：Phase 1 完成  
**目标**：确保 DFlash 中的管理标志有效且一致

| 步骤 | 操作 | OEM 要求 | 代码位置 |
|------|------|---------|---------|
| 2.1 | **读取 DFlash Main 副本** | 地址 `0xAF000000` | `Boot_DualBank_ReadFlags()` |
| 2.2 | **读取 DFlash Shadow 副本** | 冗余备份 | `Boot_DualBank_ReadFlags()` |
| 2.3 | **Magic 校验** | `FLAG_MAGIC == 0x5A5AA5A5` | `Boot_DualBank_ReadFlags()` |
| 2.4 | **Sequence 仲裁** | 取 Sequence 号更大的有效副本 | `Boot_DualBank_ReadFlags()` |
| 2.5 | **首次启动/损坏恢复** | 若均无效，写入默认值并返回 FALSE | `Boot_DualBank_Init()` |

**默认标志值**：
- `activeBank = BANK_A`
- `bankA_valid = 0` (INVALID)
- `bankB_valid = 0` (INVALID)
- `targetWriteBank = BANK_B`
- `bootAttempts = 0`

**出口条件**：DFlash 标志可读且结构合法  
**异常处理**：若 DFlash 损坏且无法恢复，进入 Bootloader 等待重刷

---

### Phase 3: Bank 有效性验证（Bank Validity Verification）
**入口条件**：Phase 2 完成，已知 `activeBank`  
**目标**：确认目标 Bank 内的 APP 固件完整未被篡改

| 步骤 | 操作 | OEM 要求 | 代码位置 |
|------|------|---------|---------|
| 3.1 | **确定目标 Bank** | 取 `flags.main.activeBank` | `Boot_DualBank_SelectAndJump()` |
| 3.2 | **CRC32 全量计算** | 覆盖 Bank 全部有效区域（如 384KB） | `Boot_DualBank_VerifyBank()` |
| 3.3 | **CRC 比对** | 与 `flags.main.bankX_valid` 存储值比较 | `Boot_DualBank_VerifyBank()` |
| 3.4 | **防超时机制** | 每 1KB 喂一次看门狗（若已使能） | `Boot_CRC32()` |

**判定结果**：
- `VALID` → 进入 Phase 4A（跳转决策）
- `INVALID` → 进入 Phase 4B（回滚/容错）

> **OEM 安全要求**：CRC 必须覆盖完整 Bank 区域，不能只校验部分；CRC 多项式需与上位机约定一致（通常 IEEE-802.3）。

---

### Phase 4A: 跳转决策与执行（Jump to APP）
**入口条件**：目标 Bank CRC 校验通过  
**目标**：安全地将控制权交给 APP

| 步骤 | 操作 | OEM 要求 | 代码位置 |
|------|------|---------|---------|
| 4A.1 | **启动尝试计数 +1** | `bootAttempts++` 写入 DFlash | `Boot_DualBank_SelectAndJump()` |
| 4A.2 | **检查启动尝试阈值** | `MAX_BOOT_ATTEMPTS = 3` | `Boot_DualBank_SelectAndJump()` |
| 4A.3 | **若超限**：标记目标 Bank INVALID，尝试 Fallback Bank | 防止反复跳转至损坏固件 | `Boot_DualBank_SelectAndJump()` |
| 4A.4 | **若未超限**：执行裸跳转 | 见 Phase 4A-Jump | `Boot_DualBank_JumpToBank()` |

#### Phase 4A-Jump: 裸跳转详细流程
**要求**：跳转函数永不返回

| 步骤 | 操作 | OEM 要求 | 代码位置 |
|------|------|---------|---------|
| J1 | **入口地址 Sanity Check** | `startAddr + 0x20` 不得为 `0xFFFFFFFF` 或 `0x00000000` | `Boot_DualBank_JumpToBank()` |
| J2 | **全局禁用中断** | `__disable_interrupt()` | `IfxCpu_disableInterrupts()` |
| J3 | **外设去初始化** | `CAN_deinit()` + `TMR_deinit()` | `CAN_deinit()` / `TMR_deinit()` |
| J4 | **禁用 ECC Trap** | PFLSH / DFLSH 均禁用，防止新写 Flash 触发陷阱 | `FLASH0_MARP/MARD.TRAPDIS = 1` |
| J5 | **禁用 Data/Program Cache** | 避免 Cache 一致性问题 | `IfxCpu_setDataCache(0)` / `setProgramCache(0)` |
| J6 | **Memory & Instruction Barrier** | `__dsync()` + `__isync()` | `__dsync()` / `__isync()` |
| J7 | **计算 Uncached 入口地址** | `(cached & 0x00FFFFFF) \| 0xA0000000 + 0x20` | `Boot_DualBank_JumpToBank()` |
| J8 | **切断上下文链表** | `PCXI = 0`，`A11 = 0` | `__mtcr(CPU_PCXI, 0)` |
| J9 | **执行裸跳转 (`ji`)** | 禁止用 `call`，避免保存 Upper Context | `ji a15` |

> **OEM 关键要求**：必须使用 `ji`（Jump Indirect）而非 `call`。TriCore 的 `call` 会隐式保存 Upper Context 到 CSA，若 Bootloader 的上下文残留，APP 函数返回时会破坏 CSA 链表，导致 Bus Error Trap（如访问 `0x466` 等无效 CSA 地址）。

**出口条件**：CPU PC 已指向 APP Reset_Handler，MSP 由 APP 向量表决定  
**失败处理**：若 Sanity Check 失败，函数返回，进入 Phase 4B

---

### Phase 4B: 回滚与容错处理（Rollback & Fallback）
**入口条件**：目标 Bank CRC 无效，或启动尝试超限  
**目标**：尝试启动备用 Bank，或安全降级到 Bootloader

| 步骤 | 操作 | OEM 要求 | 代码位置 |
|------|------|---------|---------|
| 4B.1 | **验证 Fallback Bank** | 对非 activeBank 执行 CRC 校验 | `Boot_DualBank_SelectAndJump()` |
| 4B.2 | **Fallback VALID** | 切换 `activeBank` → Fallback，执行 `SW_Reset()` | `Boot_DualBank_SelectAndJump()` |
| 4B.3 | **Fallback INVALID** | 两 Bank 均损坏，返回至 Bootloader 主循环 | `Boot_DualBank_SelectAndJump()` |

**SW_Reset 流程**：
1. 等待 PFlash 和 DFlash 就绪（`IfxFlash_waitUnbusy`）
2. 清除 Safety Endinit
3. 触发软件复位 `IfxCpu_triggerSwReset()`
4. 重新置位 Safety Endinit

---

### Phase 5: Bootloader 模式进入（Bootloader Mode Entry）
**入口条件**：Phase 3/4B 判定无法跳转，或收到 APP 显式请求进入 Bootloader（RAM 标志 `0xF0B1`）  
**目标**：初始化诊断通信环境，等待上位机指令

| 步骤 | 操作 | OEM 要求 | 代码位置 |
|------|------|---------|---------|
| 5.1 | **获取系统频率** | CPU/PLL/System/STM 频率，供超时计算 | `Mcu_getCpuFreq()` |
| 5.2 | **读取 RAM 启动标志** | `0x7002DFFC`，检测 APP 请求进入 Bootloader | `AppBL_init()` |
| 5.3 | **ECC Trap 禁用** | Bootloader 阶段禁用，避免调试/刷写时误触发 | `FLASH0_MARP/MARD.TRAPDIS = 1` |
| 5.4 | **CAN 硬件初始化** | 配置 CAN Node、Message Object、中断 | `Multican_init()` |
| 5.5 | **UDS 协议栈初始化** | 功能寻址 + 物理寻址 + 响应寻址配置 | `UdsInit()` |
| 5.6 | **Flash 驱动初始化** | 将擦除/写入例程拷贝到 PSPR（RWW 约束） | `Flash_init()` |
| 5.7 | **定时器初始化** | GPT12，用于 UDS 超时 / 喂狗 | `TMR_init()` |
| 5.8 | **全局使能中断** | CAN RX 中断、定时器中断就绪 | `IfxCpu_enableInterrupts()` |

> **OEM 要求**：Bootloader 的 CAN ID 需符合项目DBC/诊断调查表定义；UDS 物理寻址与功能寻址必须严格区分。

---

### Phase 6: 诊断主循环（Diagnostic Main Loop）
**入口条件**：Phase 5 完成  
**目标**：持续处理 CAN 报文、UDS 诊断请求、维持通信超时管理

```
while (TRUE)
{
    ├─ AppBL_main()
    │   ├─ UdsMainProcess()        ← 1ms 周期
    │   │   ├─ TP_SystemTickCtl()  ← CAN-TP 超时
    │   │   ├─ UDS_SystemTickCtl() ← S3 / Security 超时
    │   │   ├─ CANTP_MainFun()     ← 帧重组
    │   │   ├─ UDS_MainFun()       ← 服务分发
    │   │   └─ SendMsgMainFun()    ← 发送队列
    │   └─ CanMainProcess()        ← CAN TX/RX + BusOff 恢复
    │
    └─ BrdLed_main()               ← 心跳指示
}
```

**BusOff 自动恢复**：
- 检测到 `BOFF=1 && LEC=0x5` 或 `EWRN=1 && LEC=0x3`
- 自动复位 `CCE` 和 `INIT` 位，恢复通信

---

### Phase 7: 刷写会话（Programming Session）
**入口条件**：收到 `0x10 02`（Programming Session）且安全访问已通过（如需要）  
**目标**：将新固件写入非激活 Bank

#### 7.1 预编程条件检查（RID 0xFFFD）
| 检查项 | 说明 |
|--------|------|
| 目标 Bank 计算 | 始终选择 **非 activeBank** 作为写入目标 |
| 返回给上位机 | `0xXX0A` = Bank A 可刷，`0xXX0B` = Bank B 可刷 |

#### 7.2 请求下载（0x34）
| 检查项 | OEM 要求 |
|--------|---------|
| 地址合法性 | 必须在目标 Bank 地址范围内 |
| 防自刷保护 | 禁止刷写当前 `activeBank` → `NRC 0x22` |
| CRC 流初始化 | `gs_DownloadCRC = 0xFFFFFFFF` |
| 块序号初始化 | `gs_RxBlockNum = 1` |

#### 7.3 数据传输（0x36）
| 检查项 | OEM 要求 |
|--------|---------|
| 序列号校验 | 必须严格递增，否则 `NRC 0x24` |
| Flash 写入 | 32 字节页对齐，不足部分缓存在 `s_remainBuffer` |
| 写入后校验 | 每页写入后立即回读验证 |
| 流式 CRC | 实时更新 `gs_DownloadCRC` |

#### 7.4 传输退出（0x37）
- 验证当前步为 `FL_EXIT_TRANSFER_STEP`
- 进入待校验状态 `FL_CHECKSUM_STEP`

#### 7.5 擦除内存（RID 0xFF00）
| 保护项 | OEM 要求 |
|--------|---------|
| Bootloader 保护区 | Sector 0~7 禁止擦除 → `NRC 0xFC` |
| 非目标 Bank 保护 | 禁止擦除非 `targetWriteBank` 的 Sector → `NRC 0xFD` |
| RWW 约束 | 擦除例程必须从 PSPR 执行 |

#### 7.6 CRC 校验与标记有效（RID 0xDFFF）
| 步骤 | 操作 | OEM 要求 |
|------|------|---------|
| 7.6.1 | 解析上位机期望 CRC（大端） | 与下载流式 CRC 独立 |
| 7.6.2 | 计算 Bank 实际 CRC32 | 覆盖完整 Bank 区域 |
| 7.6.3 | 比对 CRC | 不一致 → 失败 |
| 7.6.4 | 标记 Bank VALID | `Boot_DualBank_MarkBankValid()` |
| 7.6.5 | 切换 Active Bank | `Boot_DualBank_SetActiveBank()` |
| 7.6.6 | 写 DFlash 双份冗余 | Main + Shadow，Sequence +1 |

---

### Phase 8: 跳转至新 APP（Post-Programming Jump）
**入口条件**：刷写完成、CRC 校验通过、Bank 已标记有效且激活  
**目标**：从 Bootloader 安全跳转到新刷写的 APP

| 触发方式 | 说明 |
|----------|------|
| UDS 0x11 复位 | ECUReset，复位后从新的 activeBank 启动 |
| UDS 0x31 02 JumpToApp | 发送正响应后，通过回调执行跳转 |

**JumpToApp 回调流程**：
```c
// uds_app.c
static void DoJumpToActiveBank(uint8 status)
{
    if (TX_MSG_SUCCESSFUL == status)
        Boot_DualBank_JumpToBank(Boot_DualBank_GetActiveBank());
}
```

> **注意**：JumpToApp 必须等正响应发完后再执行，否则上位机收不到响应会认为超时。

---

## 3. 异常处理矩阵

| 场景 | 处理策略 | 用户可见行为 |
|------|---------|-------------|
| DFlash 标志损坏 | 初始化默认值，进入 Bootloader | 等待重刷 |
| 目标 Bank CRC 失败 | 尝试 Fallback Bank | 若成功则复位后启动；若失败则进 Bootloader |
| 连续 3 次启动失败 | 标记当前 Bank INVALID，切换 Bank | 自动回滚至旧版本 |
| 刷写目标 = Active Bank | 拒绝下载，`NRC 0x22` | 上位机报错 |
| 擦除 Bootloader 区 | 拒绝擦除，`NRC 0xFC` | 上位机报错 |
| CRC 校验不匹配 | 不标记 VALID，保持旧版本激活 | 需重新刷写 |
| CAN BusOff | 自动恢复 | 通信可能短暂中断 |

---

## 4. 关键时序要求

| 项目 | 要求 | 说明 |
|------|------|------|
| 启动到跳转判定 | `< 100ms`（建议） | 避免 APP 侧看门狗超时（若 APP 已集成） |
| CRC32 计算 384KB | 需喂狗或关狗 | 防止计算过程中复位 |
| UDS S3 超时 | 通常 `5000ms` | 非编程会话超时应回 Default Session |
| PFlash 擦除单 Sector | `~0.5~2s` | 擦除期间需维持 CAN 通信或允许 NRC 0x78 |
| PFlash 写入 32B | `~50~100us` | 快速操作，不影响 TP 时序 |

---

## 5. 关键地址与常量映射

| 符号 | 地址/值 | 说明 |
|------|---------|------|
| `BANK_A_START_ADDR` | `0x80020000` | Bank A 起始（cached，S8） |
| `BANK_B_START_ADDR` | `0x80100000` | Bank B 起始（cached，S23） |
| `DFLASH_FLAG_ADDR` | `0xAF000000` | DFlash 管理标志区（uncached） |
| `RAM_BOOT_MODE_Addr` | `0x7002DFFC` | RAM 标志，APP 请求进 Bootloader |
| `FL_APP_FLAG_Addr` | `0xA0018000` | 遗留单 Bank 标志（PF0 S6） |
| `FLAG_MAGIC` | `0x5A5AA5A5` | 标志结构魔数 |
| `MAX_BOOT_ATTEMPTS` | `3` | 启动尝试上限 |
| `PFLASH_PAGE_LENGTH` | `32` | PFlash 页大小 |
| `BMHD_SIZE` | `0x20` | Boot Mode Header 偏移，APP 入口 = Base + 0x20 |

---

## 6. 文件职责对照表

| 文件 | 职责 |
|------|------|
| `Cpu0_Main.c` | 启动入口、初始化顺序、主循环框架 |
| `App_bootloader.c` | Bootloader 初始化、SW_Reset、APP 跳转封装 |
| `Boot_DualBank.c/h` | 双 Bank 标志管理、CRC 校验、Bank 跳转、回滚逻辑 |
| `uds_main.c` | UDS 栈主循环（1ms Tick、TP、UDS、TX） |
| `uds_app.c` | UDS 服务实现（0x10~0x37、0x31、0x85 等） |
| `fls_app.c` | Flash 下载状态机、数据缓冲 |
| `Flash.c` | 底层 PFlash/DFlash 擦除写入、PSPR 拷贝 |
| `CANRxTxInterface.c` | CAN 队列、中断、BusOff 恢复 |
| `Can.c / Tmr.c` | CAN / GPT12 模块初始化和反初始化 |

---

## 7. 车企合规检查清单（Checklist）

- [x] **启动阶段关闭看门狗**，避免初始化未完成前复位
- [x] **DFlash 标志双份冗余**，支持掉电安全
- [x] **Bank 切换前 CRC32 全量校验**，防止跳转至损坏固件
- [x] **启动尝试计数 + 自动回滚**，防止变砖
- [x] **跳转前外设去初始化**（CAN / TMR），避免 APP 初始化冲突
- [x] **跳转前禁用 ECC Trap**，防止新写 Flash 触发陷阱
- [x] **跳转前关闭 Cache + Barrier**，保证 Cache 一致性
- [x] **裸跳转（`ji`）切断 CSA 上下文**，避免 APP 返回时 Bus Error
- [x] **刷写时禁止自刷**（Active Bank 保护）
- [x] **擦除时保护 Bootloader 区**（Sector 0~7）
- [x] **Flash 操作例程在 PSPR 执行**，满足 RWW 约束
- [x] **UDS 0x34 流式 CRC**，实时校验下载数据完整性
- [x] **UDS 0x31 0xDFFF 独立 CRC 校验**，与流式 CRC 双重确认
- [x] **Bank 标记有效后切换 Active Bank**，确保下次启动正确
