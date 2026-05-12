# TC234 A/B 双分区 Bootloader 开发文档

> **文档编号**: IEBS-BL-DDD-v3.0  
> **版本**: V3.0.0  
> **日期**: 2026-05-11  
> **平台**: Infineon AURIX TC234 (TriCore) + Tasking 编译器  
> **适用标准**: ISO 14229 (UDS)、ISO 15765-2 (CAN TP)、OEM 刷写规范

---

## 目录

1. [概述](#1-概述)
2. [系统架构](#2-系统架构)
3. [存储器布局](#3-存储器布局)
4. [双分区启动管理](#4-双分区启动管理)
5. [CAN 通信与传输层](#5-can-通信与传输层)
6. [UDS 诊断服务](#6-uds-诊断服务)
7. [Flash 驱动与刷写](#7-flash-驱动与刷写)
8. [启动流程](#8-启动流程)
9. [刷写流程](#9-刷写流程)
10. [安全机制](#10-安全机制)
11. [接口说明](#11-接口说明)
12. [附录](#12-附录)

---

## 1. 概述

### 1.1 项目背景

本 Bootloader 基于 **Infineon AURIX TC234** 微控制器开发，采用 **车企标准 A/B 双分区（Dual Bank）启动方案**。TC234 仅具备单 PFlash Bank（PF0），无硬件 A/B Swap 机制（区别于 TC3xx 系列），因此双分区切换完全由软件通过 DFlash 标志区实现。

### 1.2 核心特性

| 特性 | 说明 |
|------|------|
| **A/B 双分区启动** | Bank A (S8~S22) 与 Bank B (S23~S26) 互为备份，独立可启动 |
| **DFlash 双备份标志区** | 主区 + 影子区冗余存储，防掉电损坏 |
| **启动失败自动回滚** | 连续启动失败 ≥ 3 次自动切换到另一 Bank |
| **CRC32 全镜像校验** | IEEE 802.3 标准 CRC32，标志区 + Bank 镜像双重校验 |
| **UDS 标准诊断服务** | 支持 0x10/0x11/0x27/0x28/0x22/0x2E/0x31/0x34/0x36/0x37/0x3E/0x85 |
| **CAN TP 分段传输** | ISO 15765-2 标准，支持 SF/FF/CF/FC |
| **PFlash RWW 安全** | 擦写操作从 PSPR (RAM) 执行，避免 RWW 冲突 |
| **防刷写覆盖运行区** | 0x34 请求地址与 activeBank 比对，禁止刷写当前运行 Bank |

### 1.3 参考文档

- ISO 14229-1:2020 — 统一诊断服务 (UDS)
- ISO 15765-2:2016 — CAN 传输层协议
- Infineon TC23x User's Manual
- Infineon iLLD (Low Level Driver) 文档

---

## 2. 系统架构

### 2.1 软件分层架构

```
┌─────────────────────────────────────────────────────────────┐
│  UDS Application Layer (ISO 14229)                          │
│  uds_app.c / uds_main.c / uds_alg.c / uds_dtc.c            │
│  诊断服务分发、安全访问、数据读写、刷写控制                  │
├─────────────────────────────────────────────────────────────┤
│  CAN Transport Layer (ISO 15765-2)                          │
│  uds_tp.c / uds_fifo.c                                      │
│  单帧/首帧/连续帧/流控帧处理，分段重组与发送                 │
├─────────────────────────────────────────────────────────────┤
│  CAN Interface & Hardware Abstraction                       │
│  CANRxTxInterface.c / MultiCAN.c / Can.c / CanIf.c          │
│  CAN 帧收发、消息对象配置、TLE9252 收发器控制                │
├─────────────────────────────────────────────────────────────┤
│  Bootloader Core                                            │
│  App_bootloader.c / Boot_DualBank.c                         │
│  启动管理、Bank 选择、跳转、标志区管理                       │
├─────────────────────────────────────────────────────────────┤
│  Flash Driver                                               │
│  Flash.c / fls_app.c                                        │
│  PFlash/DFlash 擦除、写入、PSPR 重定位执行                   │
├─────────────────────────────────────────────────────────────┤
│  HAL & Board Support                                        │
│  uds_hal.c / ScuClock.c / Stm.c / Tmr.c / Brd_led.c        │
│  复位控制、时钟、定时器、LED 指示                            │
├─────────────────────────────────────────────────────────────┤
│  MCU Startup                                                │
│  Cpu0_Main.c / IfxCpu_CStart0.c                             │
│  启动代码、BMHD、向量表、看门狗初始化                        │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 模块依赖关系

```
Cpu0_Main.c
    ├── Boot_DualBank.c ←──→ Flash.c
    │       └── App_bootloader_cfg.h
    ├── App_bootloader.c
    │       ├── CANRxTxInterface.c
    │       ├── uds_main.c
    │       │       ├── uds_app.c ←──→ fls_app.c
    │       │       ├── uds_tp.c  ←──→ uds_fifo.c
    │       │       └── uds_timer.c
    │       └── uds_hal.c
    └── ScuClock.c / custom_delay.c / Brd_led.c
```

---

## 3. 存储器布局

### 3.1 PFlash 布局（程序存储）

| 区域 | Sector | 起始地址 | 结束地址 | 大小 | 说明 |
|------|--------|----------|----------|------|------|
| Bootloader | S0~S5 | `0x80000000` | `0x8001FFFF` | 128 KB | Bootloader 代码 |
| Reserved | S6~S7 | `0x80020000` | `0x8003FFFF` | 128 KB | 保留（未使用） |
| **APP Bank A** | S8~S22 | `0x80040000` | `0x8013FFFF` | 896 KB | 主应用区（独立启动） |
| **APP Bank B** | S23~S26 | `0x80140000` | `0x801FFFFF` | 1 MB | 备份应用区（独立启动） |

> **注**: 实际 Bank 起始地址在 LSL 中定义为 `0x80020000` (Bank A) 和 `0x80100000` (Bank B)。此处为硬件 Sector 边界描述。

### 3.2 DFlash 布局（数据标志区）

| 区域 | 起始地址 | 大小 | 说明 |
|------|----------|------|------|
| 主标志区 (Main) | `0xAF000000` | 56 bytes | 双分区管理主标志 |
| 影子标志区 (Shadow) | `0xAF000100` | 56 bytes | 冗余备份，不同物理页 |
| 保留 | `0xAF000150` | ~8 KB | 未使用 |
| Sector 1 | `0xAF002000` | 8 KB | 可用于刷写临时缓冲 |

### 3.3 RAM 布局

| 区域 | 地址 | 大小 | 说明 |
|------|------|------|------|
| DSRAM0 (Local) | `0x70000000` | 184 KB | 数据 RAM |
| PSPR (CPU0) | `0x70100000` | 8 KB | 程序暂存，Flash 擦写函数重定位 |
| RAM Boot Flag | `0x7002DFFC` | 4 bytes | APP 请求进入 Bootloader 标志 |

---

## 4. 双分区启动管理

### 4.1 核心设计思想

TC234 **无硬件 A/B Swap**，因此采用纯软件方案：

1. **DFlash 标志区** 记录当前激活的 Bank、版本号、CRC、启动尝试计数。
2. **主/影子双备份** 防止写入过程中掉电导致标志损坏。
3. **启动时验证目标 Bank** 的 CRC 和入口合法性，失败则自动回滚。
4. **Bank 跳转** 时执行完整的反初始化（关闭中断、CAN、定时器、看门狗、Cache、清栈），通过裸跳转 (`ji`) 避免上下文污染。

### 4.2 标志区数据结构

```c
typedef struct
{
    uint32 magic;              /* 0x5A5AA5A5 — 标志区有效性魔数 */
    uint32 activeBank;         /* 0=Bank A, 1=Bank B */
    uint32 bankA_valid;        /* BANK_VALID_MAGIC(0x55AA55AA) 表示有效 */
    uint32 bankB_valid;        /* 同上 */
    uint32 bankA_version;      /* Bank A 固件版本 */
    uint32 bankB_version;      /* Bank B 固件版本 */
    uint16 bootAttempts;       /* 当前 Bank 连续启动尝试计数 */
    uint16 flags;              /* [7:0]=rollbackReason, [15:8]=reserved */
    uint32 sequence;           /* 单调递增序列号，主/影子冲突裁决 */
    uint32 crc32;              /* 本结构体前 36 bytes 的 CRC32 */
    uint32 targetWriteBank;    /* UDS 刷写目标 Bank */
    uint32 bankA_codeSize;     /* Bank A 实际代码大小 */
    uint32 bankB_codeSize;     /* Bank B 实际代码大小 */
    uint32 bankA_crc32;        /* Bank A 镜像 CRC32 */
    uint32 bankB_crc32;        /* Bank B 镜像 CRC32 */
} BootFlagMain_t;  /* 56 bytes */
```

影子结构 `BootFlagShadow_t` 字段与主区一一对应，命名前缀为 `shadow_`。

### 4.3 标志区读取策略（防掉电/坏块）

```
读取 DFlash 主区 + 影子区
    │
    ├── 两者均有效 → 比较 sequence，取较大者（最新写入）
    │
    ├── 仅主区有效 → 使用主区，自动修复影子区
    │
    ├── 仅影子区有效 → 使用影子区，自动修复主区
    │
    └── 两者均无效 → 返回 FALSE，触发首次初始化
```

### 4.4 启动选择与自动回滚（核心）

```
上电
  │
  ▼
Boot_DualBank_Init()
  │─ 尝试读取 DFlash 主/影子标志区
  │─ 若有效：加载到内存全局变量 g_activeBank
  │─ 若无效：初始化默认标志（activeBank=A, bootAttempts=0, sequence=1）
  ▼
Boot_DualBank_SelectAndJump()
  │
  ├── 读取标志区 ──→ 失败 ──→ 返回 Bootloader（等重刷）
  │
  ├── 验证 targetBank CRC + 入口点
  │       │
  │       ├─ 无效 ──→ 验证 fallbackBank
  │       │              ├─ 有效 → 切 activeBank=fallback → SW_Reset()
  │       │              └─ 无效 → 返回 Bootloader（双 Bank 均坏）
  │       │
  │       └─ 有效 ──→ 检查 bootAttempts
  │                      │
  │                      ├─ >= MAX_BOOT_ATTEMPTS(3)
  │                      │       → 标记 target 无效
  │                      │       → 切到 fallback
  │                      │       → SW_Reset()（自动回滚）
  │                      │
  │                      └─ < 3  → bootAttempts++
  │                               → 写标志
  │                               → Boot_DualBank_JumpToBank()
  │                               → （永不返回）
  ▼
若返回 → AppBL_init() → UDS 主循环（等待刷写）
```

### 4.5 Bank 跳转（裸跳转，防止上下文污染）

```c
void Boot_DualBank_JumpToBank(uint32 bank)
{
    /* Step 1: Disable interrupts */
    IfxCpu_disableInterrupts();
    __dsync(); __isync();

    /* Step 2: De-initialize peripherals */
    CAN_deinit();
    TMR_deinit();

    /* Step 3: Disable watchdogs */
    IfxScuWdt_disableCpuWatchdog(...);
    IfxScuWdt_disableSafetyWatchdog(...);

    /* Step 4: Disable ECC Trap */
    FLASH0_MARP.B.TRAPDIS = 1;
    FLASH0_MARD.B.TRAPDIS = 1;

    /* Step 5: Clear sensitive stack data */
    /* memset stack region to 0x00 */

    /* Step 6: Disable Cache */
    IfxCpu_setDataCache(0);
    IfxCpu_setProgramCache(0);

    /* Step 7: Raw jump (ji) — no context save */
    __asm("mov   d15, %0" : : "d"(entryAddr) : "d15");
    __asm("mov.a a15, d15" : : : "a15");
    __mtcr(CPU_PCXI, 0);          /* Cut context chain */
    __isync();
    __asm("mov.a a11, #0" : : : "a11");  /* Clear RA */
    __asm("ji    a15" : : : "a15");      /* Jump indirect */
    while(1) {}  /* Should never reach */
}
```

> **关键设计**：不使用 C 函数调用（`call` 会保存 upper context 到 CSA），而是使用 `ji` 指令裸跳转，并手动清除 `PCXI` 和 `A11`（Return Address），确保 APP 的 `_start` 运行如同从复位启动。

### 4.6 回滚原因记录

| 原因码 | 定义 | 触发条件 |
|--------|------|----------|
| `0x01` | `ROLLBACK_REASON_VERIFY_FAIL` | 标志区校验失败 |
| `0x02` | `ROLLBACK_REASON_BOOT_TIMEOUT` | 连续启动失败 ≥ 3 次 |
| `0x03` | `ROLLBACK_REASON_TRAP` | APP 触发 Trap |
| `0x04` | `ROLLBACK_REASON_WATCHDOG` | 看门狗复位 |
| `0x05` | `ROLLBACK_REASON_ERASED` | Bank 处于擦除状态 |
| `0x06` | `ROLLBACK_REASON_ENTRY_INVALID` | 入口点无效（0xFFFFFFFF/0x00000000） |
| `0x07` | `ROLLBACK_REASON_CRC_MISMATCH` | Bank CRC 校验失败 |
| `0x08` | `ROLLBACK_REASON_MAGIC_FAIL` | 魔数校验失败 |

---

## 5. CAN 通信与传输层

### 5.1 CAN ID 配置

| 消息对象 | CAN ID | 方向 | 用途 |
|----------|--------|------|------|
| MsgObj 20 | `0x7DF` | RX | 功能寻址（广播） |
| MsgObj 21 | `0x74C` | RX | 物理寻址（点对点） |
| MsgObj 10 | `0x75C` | TX | ECU 响应发送 |

### 5.2 CAN TP 状态机

本项目实现 ISO 15765-2 标准 CAN TP 层，支持四种 N_PDU 类型：

| 类型 | PCI 高 4 位 | 名称 | 用途 |
|------|-------------|------|------|
| SF | `0x0` | Single Frame | 数据 ≤ 7 字节，一帧发完 |
| FF | `0x1` | First Frame | 数据 > 7 字节，首帧 |
| CF | `0x2` | Consecutive Frame | 连续帧，SN 循环 0~15 |
| FC | `0x3` | Flow Control | 流控帧，CTS/WT/OVFLW |

### 5.3 超时参数配置

| 定时器 | 方向 | 含义 | 配置值 |
|--------|------|------|--------|
| N_As | TX | 发送方发送任意帧最大时间 | 25 ms |
| N_Ar | RX | 接收方发送 FC 帧最大时间 | 25 ms |
| N_Bs | TX | 发送方等待 FC 帧最大时间 | 75 ms |
| N_Br | RX | 接收方发送 FC 前最大等待 | 0 ms（立即回复） |
| N_Cs | TX | 发送方发送下一 CF 前最大时间 | 25 ms |
| N_Cr | RX | 接收方等待下一 CF 最大时间 | 150 ms |

### 5.4 四重 FIFO 数据流

```
CAN 硬件接收中断
    │
    ▼
RX_BUS_FIFO ('r')  ← 原始 CAN 帧（8 字节/帧），300 bytes
    │ CANTP_MainFun()
    ▼
TP 状态机 (SF/FF/CF/FC 分段重组)
    │
    ▼
RX_TP_QUEUE ('R')  ← 完整 UDS 请求报文，150 bytes
    │ UDS_MainFun()
    ▼
UDS 服务处理 (uds_app.c)
    │ 构造响应
    ▼
TX_TP_QUEUE ('T')  ← 完整 UDS 响应报文，150 bytes
    │ CANTP_MainFun()
    ▼
TP 状态机 (分段发送)
    │
    ▼
TX_BUS_FIFO ('t')  ← 原始 CAN 帧（8 字节/帧），300 bytes
    │ SendMsgMainFun()
    ▼
CAN 硬件发送
```

---

## 6. UDS 诊断服务

### 6.1 服务列表

| SID | 服务名 | 支持会话 | 寻址方式 | 安全等级 | 实现文件 |
|-----|--------|----------|----------|----------|----------|
| `0x10` | DiagnosticSessionControl | 全部 | 物理+功能 | None | [`uds_app.c`](BootLoader_dualBank/AppSw/Tricore/App_UDS/uds_app.c:796) |
| `0x11` | ECUReset | 默认/扩展/编程 | 物理+功能 | None | [`uds_app.c`](BootLoader_dualBank/AppSw/Tricore/App_UDS/uds_app.c:942) |
| `0x27` | SecurityAccess | 扩展/编程 | 物理 | None | [`uds_app.c`](BootLoader_dualBank/AppSw/Tricore/App_UDS/uds_app.c:972) |
| `0x28` | CommunicationControl | 全部 | 物理+功能 | Level 1 | [`uds_app.c`](BootLoader_dualBank/AppSw/Tricore/App_UDS/uds_app.c:1085) |
| `0x22` | ReadDataByIdentifier | 全部 | 物理+功能 | None | [`uds_app.c`](BootLoader_dualBank/AppSw/Tricore/App_UDS/uds_app.c:1192) |
| `0x23` | ReadMemoryByAddress | 默认/扩展 | 物理+功能 | None | [`uds_app.c`](BootLoader_dualBank/AppSw/Tricore/App_UDS/uds_app.c:1131) |
| `0x2E` | WriteDataByIdentifier | 扩展/编程 | 物理 | Level 1 | [`uds_app.c`](BootLoader_dualBank/AppSw/Tricore/App_UDS/uds_app.c:1238) |
| `0x31` | RoutineControl | 全部 | 物理 | Level 1/2 | [`uds_app.c`](BootLoader_dualBank/AppSw/Tricore/App_UDS/uds_app.c:1640) |
| `0x34` | RequestDownload | 编程 | 物理+功能 | Level 2 | [`uds_app.c`](BootLoader_dualBank/AppSw/Tricore/App_UDS/uds_app.c:1386) |
| `0x36` | TransferData | 编程 | 物理+功能 | Level 2 | [`uds_app.c`](BootLoader_dualBank/AppSw/Tricore/App_UDS/uds_app.c:1501) |
| `0x37` | RequestTransferExit | 编程 | 物理+功能 | Level 2 | [`uds_app.c`](BootLoader_dualBank/AppSw/Tricore/App_UDS/uds_app.c:1590) |
| `0x3E` | TesterPresent | 全部 | 物理+功能 | None | [`uds_app.c`](BootLoader_dualBank/AppSw/Tricore/App_UDS/uds_app.c:477) |
| `0x85` | ControlDTCSetting | 全部 | 物理+功能 | Level 1 | [`uds_app.c`](BootLoader_dualBank/AppSw/Tricore/App_UDS/uds_app.c:1332) |

### 6.2 会话管理

| 会话类型 | 子功能 | 说明 |
|----------|--------|------|
| 默认会话 | `0x01` / `0x81` | 基础诊断，无需保持 |
| 编程会话 | `0x02` / `0x82` | 刷写操作，需 Level 2 解锁 |
| 扩展会话 | `0x03` / `0x83` | 高级诊断，需 Level 1 解锁 |

**S3 超时机制**：
- 配置: `S3_SERVER_TIMEOUT = 5000 ms`
- 在扩展/编程会话中，5 秒内未收到任何 UDS 请求 → 自动回到默认会话 + 安全等级归零
- 诊断仪应每 2 秒发送 `0x3E 80` 保持会话

### 6.3 安全访问 (0x27)

| 子功能 | 含义 | 安全等级 | 说明 |
|--------|------|----------|------|
| `0x01` | Request Seed Level 1 | — | 扩展会话解锁 |
| `0x02` | Send Key Level 1 | Level 1 | 失败后 3 次锁定 10 秒 |
| `0x03` | Request Seed Level 2 | — | 编程会话解锁 |
| `0x04` | Send Key Level 2 | Level 2 | 刷写操作必需 |

Seed 长度: 4 bytes，通过 [`UDS_ALG_HAL_GetRandom()`](BootLoader_dualBank/AppSw/Tricore/App_UDS/uds_alg.c) 生成。
Key 计算: 由外部 DLL（`ZcanProDll.dll`）或 [`UDS_ALG_HAL_ComputeKey_Level1/2()`](BootLoader_dualBank/AppSw/Tricore/App_UDS/uds_alg.c) 实现。

### 6.4 刷写相关服务详解

#### 6.4.1 RequestDownload (0x34)

```
请求格式: [SID=0x34] [dataFormatIdentifier=0x00] [addrAndLenFormat=0x44]
          [addr byte3] [addr byte2] [addr byte1] [addr byte0]
          [len byte3]  [len byte2]  [len byte1]  [len byte0]

正响应:   [0x74] [lengthFormatIdentifier=0x10] [maxNumberOfBlockLength=0x80]
          表示每帧 0x36 最大可传输 128 - 2 = 126 字节数据
```

**双分区关键逻辑**：
- 根据下载起始地址判断目标 Bank：
  - `0x80020000~0x800FFFFF` → Bank A
  - `0x80100000~0x801FFFFF` → Bank B
- **禁止刷写当前运行 Bank**：若目标地址落在 `activeBank` 范围内，返回 `NRC_CONDITIONS_NOT_CORRECT (0x22)`
- 设置 `g_udsTargetWriteBank`，后续 `0x31 01 FF 00` 擦除仅操作该 Bank 的 Sector

#### 6.4.2 TransferData (0x36)

```
请求格式: [SID=0x36] [blockSequenceCounter] [data...]

正响应:   [0x76] [blockSequenceCounter]
```

- 序列号 `blockSequenceCounter` 从 1 开始，每帧递增，溢出后回到 0
- 若序列号不匹配，返回 `NRC_REQUEST_SEQUENCE_ERROR (0x24)`，并复位下载状态机
- 数据通过 [`Flash_ProgramRegion()`](BootLoader_dualBank/AppSw/Tricore/App_UDS/Flah_app/fls_app.c:81) 写入 PFlash
- **Streaming CRC**：在接收数据的同时累加 CRC32（`gs_DownloadCRC`），避免刷写完成后重新读取 Flash

#### 6.4.3 RequestTransferExit (0x37)

- 仅在 `FL_EXIT_TRANSFER_STEP` 状态下允许执行
- 成功后将下载状态推进到 `FL_CHECKSUM_STEP`

#### 6.4.4 RoutineControl (0x31)

**RID = 0xFFFD — 检查编程条件**

```
请求:  31 01 FF FD
响应:  71 01 FF FD <canFlash> <targetBank>
       canFlash:    1=允许刷写, 0=禁止
       targetBank:  0x0A=Bank A, 0x0B=Bank B
```

- 返回推荐刷写的目标 Bank（非当前运行 Bank）
- 诊断仪根据响应动态调整 `TARGET_BANK`

**RID = 0xFF00 — 擦除 Flash Sector**

```
请求:  31 01 FF 00 <sectorHigh> <sectorLow>
响应:  71 01 FF 00 <sector> <result>
       result: 0x01=成功, 0x00=失败, 0xFE=越界, 0xFC=Bootloader保护区, 0xFD=非目标Bank
```

- 仅擦除属于 `targetWriteBank` 的 Sector
- Bootloader Sector (S0~S7) 受保护，禁止擦除

**RID = 0xDFFF — 验证并激活 Bank**

```
请求:  31 01 DF FF <CRC32_big_endian_4bytes>
响应:  71 01 DF FF <result>
       result: 0x01=验证通过并激活, 0x00=CRC不匹配
```

- 使用 Streaming CRC（`gs_DownloadCRC ^ 0xFFFFFFFF`）与诊断仪传入的 CRC 比对
- CRC 匹配后：
  1. 调用 [`Boot_DualBank_MarkBankValid()`](BootLoader_dualBank/AppSw/Tricore/App_bootloader/Boot_DualBank.c:657) 计算并记录 Bank CRC
  2. 调用 [`Boot_DualBank_SetActiveBank()`](BootLoader_dualBank/AppSw/Tricore/App_bootloader/Boot_DualBank.c:958) 设置激活标志
- 下次复位后将启动到新 Bank

**RID = jumpToApp (0x??) — 跳转到 APP**

```
请求:  31 02 <jumpToApp>
响应:  71 02 <jumpToApp>
```

- 正响应发送完成后，通过 Tx Callback [`DoJumpToActiveBank()`](BootLoader_dualBank/AppSw/Tricore/App_UDS/uds_app.c:882) 执行跳转

### 6.5 通信控制 (0x28)

| 子功能 | 通信类型 | 说明 |
|--------|----------|------|
| `0x00` | Enable RX and TX | 正常通信 |
| `0x01` | Enable RX, Disable TX | 静默接收（仅诊断） |
| `0x02` | Disable RX, Enable TX | 禁止接收 |
| `0x03` | Disable RX and TX | 全静默 |

通信类型固定为 `0x03`（应用报文），通过 [`DrvCanRxTxModeSet()`](BootLoader_dualBank/AppSw/Tricore/Communication/CANRxTxInterface.c) 控制 TLE9252 收发器模式。

---

## 7. Flash 驱动与刷写

### 7.1 Flash 操作约束

TC234 PFlash 擦写时必须从 **PSPR (Program Scratch-Pad RAM, `0x70100000`)** 执行，原因：
- PFlash 擦写期间不能从 PFlash 取指（RWW — Read-While-Write 限制）
- 解决方案：将擦写函数复制到 PSPR，跳转到 RAM 执行

### 7.2 PSPR 重定位映射

| 函数 | RAM 地址 | 大小 |
|------|----------|------|
| `eraseSectors` | `0x70100000` | 256 bytes |
| `waitUnbusy` | `0x70100100` | 256 bytes |
| `enterPageMode` | `0x70100200` | 256 bytes |
| `load2X32bits` | `0x70100300` | 256 bytes |
| `writePage` | `0x70100400` | 256 bytes |
| `eraseFlash` (PFlash wrapper) | `0x70100500` | 512 bytes |
| `writeFlash` (PFlash wrapper) | `0x70100700` | 512 bytes |

初始化调用 [`Flash_copyFunctionsToPSPR()`](BootLoader_dualBank/AppSw/Tricore/Tc234_Modules/Flash/Flash.c) 完成复制。

### 7.3 PFlash 写入流程

```
Flash_writePFlash_portex(addr, data, len)
    │
    ├── 地址对齐检查 (32-byte page aligned)
    ├── 数据按 32-byte page 拆分
    │
    ├── 对每个 page:
    │       ├── enterPageMode(pageAddr)      → 进入页编程模式
    │       ├── load2X32bits(pageAddr, low, high) → 加载 64-bit 数据
    │       ├── writePage(pageAddr)          → 触发写入
    │       └── waitUnbusy(PFLASH)           → 等待完成
    │
    └── 返回成功/失败
```

### 7.4 DFlash 写入流程

DFlash Page 大小为 **8 bytes**（vs PFlash 32 bytes），写入流程类似，但使用 DFlash 专用命令序列。

### 7.5 刷写状态机

```
FL_REQUEST_STEP      ← 初始状态，等待 0x34
    │ 0x34 成功
    ▼
FL_TRANSFER_STEP     ← 等待 0x36 数据帧
    │ 0x36 传输完成 (DataLen == 0)
    ▼
FL_EXIT_TRANSFER_STEP ← 等待 0x37
    │ 0x37 成功
    ▼
FL_CHECKSUM_STEP     ← 等待 0x31 01 DFFF CRC 验证
    │ CRC 匹配
    ▼
完成 → Bank 标记有效 + 设置激活 Bank
```

---

## 8. 启动流程

### 8.1 完整上电启动时序

```
上电复位
    │
    ▼
SSW 执行（硬件）
    │─ 读取 BMHD_0 @ 0x80000000
    │─ 校验 BMHD_0 CRC
    │─ 跳转到 _START @ 0x80000020
    ▼
_Core0_start() 执行
    │─ 初始化栈、PSW、Cache、CSA
    │─ 调用 IfxScuCcu_init() 初始化时钟
    ▼
core0_main()
    │
    ├─ 关闭看门狗 (CPU + Safety)
    ├─ IfxScuClock_init()     — 系统时钟
    ├─ delay_init()           — 延时模块
    ├─ BrdLed_init()          — LED 指示
    │
    ├─ Boot_DualBank_Init()   — 读取/初始化 DFlash 标志
    ├─ Boot_DualBank_SelectAndJump() — 选择并跳转 APP
    │   │
    │   ├── 成功跳转 → APP 运行（永不返回）
    │   └── 两 Bank 均无效 → 继续执行 Bootloader
    │
    ├─ AppBL_init()
    │   ├─ CAN 初始化 (MultiCAN + TLE9252)
    │   ├─ UDS 初始化 (TP + 会话/安全等级)
    │   ├─ Flash 初始化 (PSPR 复制)
    │   └─ Timer 初始化 (1ms 系统 Tick)
    │
    ▼
while(1) {
    AppBL_main()     — UDS 主循环 + CAN 收发处理
    BrdLed_main()    — LED 状态指示
}
```

### 8.2 复位类型

| 复位子功能 | 值 | 说明 | 实现 |
|------------|-----|------|------|
| Hard Reset | `0x01` | 硬件复位（Application Reset） | [`DoHardReset()`](BootLoader_dualBank/AppSw/Tricore/App_UDS/uds_app.c:908) |
| Soft Reset | `0x03` | 软件复位（CPU Reset） | [`DoSoftReset()`](BootLoader_dualBank/AppSw/Tricore/App_UDS/uds_app.c:934) |
| Key Off/On | `0x02` | 钥匙循环复位（预留） | — |

**Hard Reset 实现**：
1. 等待 PFlash/DFlash 操作完成
2. 清除 Safety Endinit
3. 写 `SCU_SWRSTCON.U = 0x01` 触发 Application Reset
4. 死循环等待复位生效

---

## 9. 刷写流程

### 9.1 首次刷写（空片/两 Bank 均无效）

```
ECU 上电 → Bootloader 模式（两 Bank 无效）
    │
    ▼
诊断仪连接
    │
    ├─ 10 01 → 默认会话
    ├─ 10 03 → 扩展会话
    ├─ 27 01/02 → Level 1 解锁
    ├─ 31 01 FF FD → 检查编程条件（返回 targetBank=A）
    ├─ 85 02 → 关闭 DTC
    ├─ 10 02 → 编程会话
    ├─ 27 03/04 → Level 2 解锁
    ├─ 2E F1 5A → 写指纹
    ├─ 31 01 FF 00 → 逐个擦除 Bank A Sector (S8~S22)
    ├─ 34 00 44 A0 02 00 00 <len> → 请求下载到 Bank A
    ├─ 36 <data>... → 传输数据
    ├─ 37 → 传输退出
    ├─ 31 01 DF FF <CRC32> → 验证 CRC 并激活 Bank A
    └─ 11 01 → ECU Hard Reset
    ▼
复位后 → Boot_DualBank_SelectAndJump() → Bank A 有效 → 跳转 APP
```

### 9.2 滚动升级（OTA 标准做法）

```
当前 Bank A 运行中
    │
    ▼
诊断仪请求刷写
    │
    ├─ ...（同上进入编程会话）
    ├─ 31 01 FF FD → 返回 targetBank=B（非运行 Bank）
    ├─ 擦除 Bank B Sector (S23~S26)
    ├─ 34 00 44 A0 10 00 00 <len> → 请求下载到 Bank B
    ├─ 36/37 → 数据传输完成
    ├─ 31 01 DF FF <CRC32> → 验证并激活 Bank B
    └─ 11 01 → ECU Hard Reset
    ▼
复位后 → 跳转到 Bank B（新版本）
         Bank A 保留旧版本作为备份
         若 Bank B 启动失败 3 次 → 自动回滚到 Bank A
```

> **车企 OTA 黄金法则**：永远写非运行 Bank → 标记有效 → 切换 Bank → 复位。旧版本作为热备份保留。

---

## 10. 安全机制

### 10.1 刷写安全

| 机制 | 实现 | 说明 |
|------|------|------|
| 运行区保护 | [`RequestDownload0x34()`](BootLoader_dualBank/AppSw/Tricore/App_UDS/uds_app.c:1438) | 禁止刷写 activeBank |
| Bootloader 区保护 | [`EraseFlashSector()`](BootLoader_dualBank/AppSw/Tricore/App_UDS/uds_app.c:1943) | S0~S7 禁止擦除 |
| Sector-Bank 绑定 | `IsSectorInTargetBank()` | 仅擦除 targetWriteBank 所属 Sector |
| CRC 验证 | [`RoutineControl0x31()`](BootLoader_dualBank/AppSw/Tricore/App_UDS/uds_app.c:1757) | 0xDFFF 验证完整镜像 CRC |
| 双备份标志区 | [`Boot_WriteFlagsToDFlash()`](BootLoader_dualBank/AppSw/Tricore/App_bootloader/Boot_DualBank.c:211) | 主区+影子区，sequence 裁决 |

### 10.2 启动安全

| 机制 | 实现 | 说明 |
|------|------|------|
| 入口点检查 | [`Boot_CheckEntryPoint()`](BootLoader_dualBank/AppSw/Tricore/App_bootloader/Boot_DualBank.c:275) | 检查 `_START` 非 0xFFFFFFFF/0x00000000 |
| 擦除检测 | [`Boot_IsBankErased()`](BootLoader_dualBank/AppSw/Tricore/App_bootloader/Boot_DualBank.c:257) | 采样检测 Bank 是否为空 |
| 启动失败计数 | `bootAttempts` + `MAX_BOOT_ATTEMPTS` | 3 次失败后自动回滚 |
| 上下文清除 | [`Boot_DualBank_JumpToBank()`](BootLoader_dualBank/AppSw/Tricore/App_bootloader/Boot_DualBank.c:714) | 清栈、关 Cache、切 PCXI |

### 10.3 通信安全

| 机制 | 实现 | 说明 |
|------|------|------|
| 安全访问锁定 | [`SecurityAccess0x27()`](BootLoader_dualBank/AppSw/Tricore/App_UDS/uds_app.c:972) | 3 次错误 Key 锁定 10 秒 |
| 会话超时 | [`UDS_SystemTickCtl()`](BootLoader_dualBank/AppSw/Tricore/App_UDS/uds_app.c:382) | S3=5s 超时回默认会话 |
| 物理/功能寻址过滤 | `SaveRequestIdType()` | 区分 0x74C/0x7DF |

---

## 11. 接口说明

### 11.1 Boot_DualBank 公共接口

| 函数 | 原型 | 说明 |
|------|------|------|
| Init | `void Boot_DualBank_Init(void)` | 初始化标志系统 |
| ReadFlags | `boolean Boot_DualBank_ReadFlags(DualBankFlags_t* flags)` | 读取双备份标志 |
| WriteFlags | `boolean Boot_DualBank_WriteFlags(const DualBankFlags_t* flags)` | 写入双备份标志 |
| SelectAndJump | `void Boot_DualBank_SelectAndJump(void)` | 选择 Bank 并跳转（不返回） |
| VerifyBank | `BankStatus_t Boot_DualBank_VerifyBank(uint32 bank)` | 验证 Bank 有效性 |
| MarkBankValid | `void Boot_DualBank_MarkBankValid(uint32 bank, uint32 version, uint32 codeSize)` | 标记 Bank 有效 |
| SwitchBank | `void Boot_DualBank_SwitchBank(uint32 targetBank)` | 切换 Bank 并复位 |
| JumpToBank | `void Boot_DualBank_JumpToBank(uint32 bank)` | 裸跳转到指定 Bank |
| GetActiveBank | `uint32 Boot_DualBank_GetActiveBank(void)` | 获取当前激活 Bank |
| SetActiveBank | `void Boot_DualBank_SetActiveBank(uint32 targetBank)` | 设置激活 Bank（不复位） |
| ClearBootAttempts | `void Boot_DualBank_ClearBootAttempts(void)` | 清零启动失败计数（APP 调用） |
| RecordRollbackReason | `void Boot_DualBank_RecordRollbackReason(uint8 reason)` | 记录回滚原因 |

### 11.2 Flash 公共接口

| 函数 | 原型 | 说明 |
|------|------|------|
| erasePFlash_port | `int Flash_erasePFlash_port(uint32 flashAddr)` | 擦除 PFlash Sector |
| writePFlash_port | `int Flash_writePFlash_port(uint32 flashAddr, uint32 *data, uint32 bytelength)` | 按 32-byte page 写入 |
| writePFlash_portex | `int Flash_writePFlash_portex(uint32 flashAddr, uint8 *data, uint32 byteLength)` | UDS 刷写专用接口 |
| eraseDFlash_port | `int Flash_eraseDFlash_port(uint32 flashAddr)` | 擦除 DFlash Sector |
| writeDFlash_port | `int Flash_writeDFlash_port(uint32 flashAddr, uint32 *data, uint32 bytelength)` | 写入 DFlash |

### 11.3 UDS 公共接口

| 函数 | 原型 | 说明 |
|------|------|------|
| UDS_MainFun | `void UDS_MainFun(void)` | UDS 服务主循环 |
| UDS_SystemTickCtl | `void UDS_SystemTickCtl(void)` | 1ms 定时递减（S3/安全锁定） |
| SendMsgMainFun | `void SendMsgMainFun(void)` | CAN 发送主循环 |
| UdsInit | `void UdsInit(tUdsId xRxFunId, tUdsId xRxPhyId, tUdsId xTxId)` | UDS 初始化 |

---

## 12. 附录

### 12.1 术语表

| 术语 | 英文 | 说明 |
|------|------|------|
| Bootloader | Bootloader | 启动加载程序，负责初始化硬件、选择 APP、支持诊断刷写 |
| UDS | Unified Diagnostic Services | ISO 14229 统一诊断服务 |
| CAN TP | CAN Transport Layer | ISO 15765-2 CAN 传输层 |
| PFlash | Program Flash | 程序闪存（TC234 共 2MB） |
| DFlash | Data Flash | 数据闪存（TC234 共 128KB） |
| PSPR | Program Scratch-Pad RAM | 程序暂存 RAM，用于 RWW 安全执行 |
| BMHD | Boot Mode Header | 启动模式头，SSW 解析 |
| SSW | Startup Software | 芯片硬件启动代码 |
| RWW | Read-While-Write | 边读边写，PFlash 不支持 |
| CSA | Context Save Area | TriCore 上下文保存区域 |

### 12.2 关键宏定义汇总

| 宏 | 值 | 说明 |
|----|----|----|
| `FLAG_MAGIC` | `0x5A5AA5A5` | 标志区有效性魔数 |
| `BANK_VALID_MAGIC` | `0x55AA55AA` | Bank 有效标志 |
| `BANK_A_START_ADDR` | `0x80020000` | Bank A 起始地址 |
| `BANK_B_START_ADDR` | `0x80100000` | Bank B 起始地址 |
| `DFLASH_FLAG_ADDR` | `0xAF000000` | DFlash 标志区起始 |
| `MAX_BOOT_ATTEMPTS` | `3` | 最大启动失败次数 |
| `S3_SERVER_TIMEOUT` | `5000` | UDS S3 超时时间 (ms) |

### 12.3 版本历史

| 版本 | 日期 | 修改内容 |
|------|------|----------|
| V1.0 | 2024-07 | 初始单分区 Bootloader |
| V2.0 | 2025-07 | 单分区 + 备份恢复机制 |
| V3.0 | 2026-05 | A/B 双分区启动，DFlash 双备份标志，Streaming CRC，裸跳转 |

---

> **文档结束**
