# TC234 A/B Dual Bank Bootloader 完整开发文档

> **版本**: V1.0  
> **平台**: Infineon AURIX TC234 + TASKING TriCore v6.2r2  
> **规范**: ISO 14229-1 (UDS) / ISO 15765-2 (CAN-TP)  
> **适用**: BootLoader_dualBank + App_dualBank + ZcanProDll + shuaxie.py

---

## 目录

1. [项目概述](#1-项目概述)
2. [系统架构](#2-系统架构)
3. [BootLoader_dualBank 详解](#3-bootloader_dualbank-详解)
4. [App_dualBank 详解](#4-app_dualbank-详解)
5. [ZcanProDll 安全访问库](#5-zcanprodll-安全访问库)
6. [shuaxie.py 刷写脚本](#6-shuaxiepy-刷写脚本)
7. [刷写流程实操](#7-刷写流程实操)
8. [常见问题与故障排查](#8-常见问题与故障排查)
9. [附录](#9-附录)

---

## 1. 项目概述

### 1.1 项目组成

本项目是一个完整的 **TC234 双 Bank Bootloader 解决方案**，包含四个子项目：

| 子项目 | 路径 | 说明 |
|--------|------|------|
| **BootLoader_dualBank** | `BootLoader_dualBank/` | Bootloader 主工程，负责启动仲裁、UDS 诊断、Flash 擦写、Bank 切换 |
| **App_dualBank** | `App_dualBank/` | APP 应用工程，编译出 Bank A/B 两个版本的 hex |
| **ZcanProDll** | `ZcanProDll/` | Seed/Key 安全访问算法 DLL，供 ZXDoc 刷写工具调用 |
| **shuaxie.py** | `BootLoader_dualBank/shuaxie.py` | ZXDoc 刷写脚本，实现完整的 UDS 刷写时序 |

### 1.2 核心能力

- **A/B 双分区独立启动**: Bank A (`0x80020000`) 和 Bank B (`0x80100000`) 均可独立运行 APP
- **无缝滚动升级**: 刷写非运行 Bank → 标记有效 → 切换激活 → 复位启动
- **自动回滚**: 连续 3 次启动失败自动回退到旧版本
- **UDS 标准刷写**: 完整的 0x10/0x27/0x31/0x34/0x36/0x37/0x11 服务链
- **多层校验**: CAN CRC → FSR 硬件错误 → 逐页回读验证 → 全 Bank CRC32
- **车企安全规范**: S3 超时回退、Security Access 尝试锁定、运行 Bank 保护

### 1.3 关键技术约束

| 约束 | 说明 |
|------|------|
| TC234 单 PFlash Bank | 无硬件 A/B Swap，Bank 切换纯软件实现 |
| PFlash 擦除态 = `0x00` | AURIX 特有，与传统 Flash 的 `0xFF` 不同 |
| RWW 约束 | 擦写 PFlash 时必须从 PSPR (RAM) 执行代码 |
| PFlash Page = 32B | 最小写入单元，地址和长度必须 32 字节对齐 |
| PFlash Sector = 16KB/32KB | 最小擦除单元 |
| DFlash Page = 8B | 标志区写入单元 |
| ECC Trap | 擦除态 PFlash 读取会触发 ECC 错误，跳转前需禁用 |

---

## 2. 系统架构

### 2.1 内存布局

```
+-------------------------------------------------------------+
|  PFlash (Program Flash)                                      |
+-------------------------------------------------------------+
|  0x80000000 ~ 0x8001FFFF  | Bootloader (S0~S5, 128KB)       |
|  0x80020000 ~ 0x8003FFFF  | Reserved (S6~S7, 128KB)         |
|  0x80040000 ~ 0x8013FFFF  | APP Bank A (S8~S22, 896KB)      | <- 可启动
|  0x80140000 ~ 0x801FFFFF  | APP Bank B (S23~S26, 1MB)       | <- 可启动
+-------------------------------------------------------------+
|  DFlash (Data Flash)                                         |
+-------------------------------------------------------------+
|  0xAF000000 ~ 0xAF001FFF  | Sector 0: 标志区 (8KB)          |
|      0xAF000000 ~ 0xAF00004F  | Main Flag (80 bytes)          |
|      0xAF000100 ~ 0xAF00014F  | Shadow Flag (80 bytes)        |
|  0xAF002000 ~ ...         | Sector 1+: 预留                 |
+-------------------------------------------------------------+
|  PSPR (Program Scratch-Pad RAM)                              |
+-------------------------------------------------------------+
|  0x70100000 ~ 0x701017FF  | .text.psram_cpu0 (链接器分配)    |
|  0x70101800 ~ 0x701019FF  | iLLD 运行时拷贝区                |
+-------------------------------------------------------------+
|  RAM                                                         |
+-------------------------------------------------------------+
|  0x7002DFFC               | RAM 启动标志 (APP 请求进 BL)     |
+-------------------------------------------------------------+
```

### 2.2 启动流程

```
上电复位
    |
    v
SSW 执行 -> 读取 BMHD_0 @ 0x80000000 -> 跳转到 _START @ 0x80000020
    |
    v
core0_main()
    |
    +- 关闭看门狗 + 时钟初始化
    |
    v
Boot_DualBank_Init()          <- 读 DFlash 主/影子标志
    |                              Magic + CRC + Sequence 仲裁
    |
    v
Boot_DualBank_SelectAndJump()
    |
    +- 验证 activeBank CRC --> 无效 --> 验证 fallbackBank
    |                              |           +- 有效 -> 切换 -> SW_Reset
    |                              |           +- 无效 -> 留在 Bootloader
    |                              |
    |                              v
    |                           有效
    |                              |
    |                              +- bootAttempts >= 3
    |                              |      -> 标记无效 -> 切 fallback -> SW_Reset
    |                              |
    |                              +- bootAttempts < 3
    |                                     -> bootAttempts++ -> 跳转 APP
    |
    v
[若跳转失败或两 Bank 均无效]
    |
    v
AppBL_init()                  <- 初始化 CAN / UDS / Flash / Timer
    |
    v
while(1) { AppBL_main(); }    <- UDS 诊断主循环，等待刷写
```

### 2.3 UDS 刷写时序（车企标准三阶段）

```
+----------------------------------------------------------------+
|  阶段 1: 预编程 (Extended Session)                              |
+----------------------------------------------------------------+
|  10 03                    -> 进入 Extended Session               |
|  85 02                    -> 关闭 DTC 记录                       |
|  27 01 / 27 02 Key        -> 解锁 Security Level 1               |
|  2E F1 5A ...             -> 写入刷写设备指纹                    |
+----------------------------------------------------------------+
|  阶段 2: 编程 (Programming Session)                             |
+----------------------------------------------------------------+
|  10 02                    -> 进入 Programming Session            |
|  27 03 / 27 04 Key        -> 解锁 Security Level 2               |
|  31 01 FF 00 BlockNum     -> 逐个擦除目标 Bank Sector            |
|  34 00 44 addr len        -> RequestDownload (地址强制转 uncached)|
|  36 01 data...            -> TransferData (SN 严格递增)          |
|  36 02 data...            -> ...                                 |
|  37                       -> RequestTransferExit                 |
+----------------------------------------------------------------+
|  阶段 3: 后编程 (校验 + 激活 + 复位)                             |
+----------------------------------------------------------------+
|  31 01 DFFF               -> VerifyBank CRC + MarkValid          |
|  31 01 0203               -> CheckProgrammingDependency          |
|                              (Verify -> MarkValid -> SetActive)   |
|  11 03                    -> SoftReset -> 启动新 Bank             |
+----------------------------------------------------------------+
```

---

## 3. BootLoader_dualBank 详解

### 3.1 项目结构

```
BootLoader_dualBank/
+-- AppSw/Tricore/
|   +-- App_bootloader/
|   |   +-- App_bootloader.c/h          # Bootloader 主循环、初始化
|   |   +-- App_bootloader_cfg.h        # 配置宏 (Bank 地址、标志地址)
|   |   +-- Boot_DualBank.c/h           # 双 Bank 核心 (标志/CRC/跳转/回滚)
|   |   +-- Cpu0_Main.c                 # 启动入口
|   +-- App_UDS/
|   |   +-- uds_app.c/h                 # UDS 服务实现
|   |   +-- uds_main.c                  # UDS 栈主循环
|   |   +-- uds_alg.c                   # Seed/Key 算法
|   |   +-- uds_cfg.h                   # 编译模式配置 (BL / APP)
|   |   +-- Flah_app/
|   |       +-- fls_app.c/h             # Flash 下载状态机
|   +-- Tc234_Modules/
|   |   +-- Flash/
|   |   |   +-- Flash.c/h               # 底层 Flash 擦写 + PSPR 拷贝
|   |   |   +-- Flash_Driver.h          # 新驱动头文件 (FSR 宏/内联函数)
|   |   +-- Can/
|   |       +-- CANRxTxInterface.c      # CAN 队列 + BusOff 恢复
|   +-- Driver/                           # iLLD 底层驱动
+-- iLLD_1_0_1_11_0/                      # Infineon iLLD 库
+-- Debug/                                # 编译输出
+-- shuaxie.py                            # ZXDoc 刷写脚本
+-- align_hex.py                          # HEX 对齐预处理工具
+-- 文档/                                 # 设计/修复/测试文档
```

### 3.2 UDS 服务实现 (uds_app.c)

#### 3.2.1 支持的服务列表

| SID | 服务 | 说明 | 会话要求 | 安全级别 |
|:---:|:-----|:-----|:---------|:---------|
| 0x10 | DiagnosticSessionControl | 会话控制 (Default/Extended/Programming) | Any | None |
| 0x11 | ECUReset | ECU 复位 (Hard/Soft/Quick) | Extended+ | None |
| 0x22 | ReadDataByIdentifier | 读取 DID | Any | None |
| 0x23 | ReadMemoryByAddress | 按地址读取内存 | Extended+ | L1+ |
| 0x27 | SecurityAccess | 安全访问 (Seed/Key) | Extended+ | None |
| 0x28 | CommunicationControl | 通信控制 | Extended+ | L1+ |
| 0x2E | WriteDataByIdentifier | 写入 DID (指纹) | Extended+ | L1+ |
| 0x31 | RoutineControl | 例程控制 (擦除/校验/跳转) | Programming | L2 |
| 0x34 | RequestDownload | 请求下载 | Programming | L2 |
| 0x36 | TransferData | 传输数据 | Programming | L2 |
| 0x37 | RequestTransferExit | 请求传输退出 | Programming | L2 |
| 0x3E | TesterPresent | 测试仪在线 | Any | None |
| 0x85 | ControlDTCSetting | DTC 控制 | Extended+ | L1+ |

#### 3.2.2 NRC 负响应码 (ISO 14229-1:2020 扩展)

```c
NRC_GENERAL_REJECT                    = 0x10,
NRC_SERVICE_NOT_SUPPORTED             = 0x11,
NRC_SUBFUNCTION_NOT_SUPPORTED         = 0x12,
NRC_INCORRECT_MESSAGE_LENGTH          = 0x13,
NRC_CONDITIONS_NOT_CORRECT            = 0x22,
NRC_REQUEST_SEQUENCE_ERROR            = 0x24,
NRC_REQUEST_OUT_OF_RANGE              = 0x31,
NRC_SECURITY_ACCESS_DENIED            = 0x33,
NRC_INVALID_KEY                       = 0x35,
NRC_EXCEEDED_NUMBER_OF_ATTEMPTS       = 0x36,
NRC_REQUIRED_TIME_DELAY_NOT_EXPIRED   = 0x37,
NRC_TRANSFER_DATA_SUSPENDED           = 0x71,
NRC_GENERAL_PROGRAMMING_FAILURE       = 0x72,
NRC_WRONG_BLOCK_SEQUENCE_COUNTER      = 0x73,
NRC_RESPONSE_TOO_LONG                 = 0x78,
NRC_SUBFUNCTION_NOT_SUPPORTED_IN_ACTIVE_SESSION = 0x7E,
NRC_SERVICE_NOT_SUPPORTED_IN_ACTIVE_SESSION     = 0x7F,
```

#### 3.2.3 关键服务实现要点

**RequestDownload (0x34)**:
- 地址强制转 uncached: `(addr & 0x00FFFFFF) | 0xA0000000`
- 自动判断目标 Bank (A/B)
- **运行 Bank 保护**: 若目标 Bank == activeBank -> NRC 0x22
- 初始化流式 CRC (`gs_DownloadCRC = 0xFFFFFFFF`)
- 重置下载状态机 (`Flash_InitDowloadInfo()`)

**TransferData (0x36)**:
- **SN 严格校验**: 接收到的 BlockSN 必须与 `gs_RxBlockNum` 一致，否则 NRC 0x24
- 每帧数据实时更新流式 CRC
- 调用 `Flash_ProgramRegion()` -> `Flash_writePFlash_portex()` 写入 Flash
- **逐页回读验证**: `Flash_writeAndVerifyPage()` 写入后立即从 Flash 回读比对

**RequestTransferExit (0x37)**:
- 刷新 `s_remainBuffer` 中残留的不足 32 字节数据
- 验证失败 -> NRC 0x72 + 重置下载状态机

**RoutineControl (0x31)**:
- `0xFF 00`: 擦除 Sector，保护 Bootloader 区 (S0~S7) -> NRC 0xFC
- `0xDFFF`: 全 Bank CRC32 校验 + 标记有效 + 切换激活
- `0x0203`: CheckProgrammingDependency (Verify -> MarkValid -> SetActive)
- `0x02 JumpToApp`: 正响应发送完成后回调跳转

**SecurityAccess (0x27)**:
- 连续失败 3 次后锁定 10 秒 (NRC 0x36)
- 锁定期间再次请求 -> NRC 0x37
- Seed 引入 CPU 频率、复位状态、LCG 迭代等熵源

### 3.3 Flash 驱动 (Flash.c / Flash_Driver.h)

#### 3.3.1 PSPR 执行机制

TC234 是单 Bank PFlash，擦写时不能从 PFlash 取指。解决方案：

```
PSPR 地址布局 (8KB @ 0x70100000)
+- 0x70100000 ~ 0x701017FF   .text.psram_cpu0 (链接器自动分配用户 Flash 函数)
+- 0x70101800 ~ 0x701018FF   eraseSectors   (iLLD 拷贝)
+- 0x70101900 ~ 0x701019FF   waitUnbusy     (iLLD 拷贝)
+- 0x70101A00 ~ 0x70101AFF   enterPageMode  (iLLD 拷贝)
+- 0x70101B00 ~ 0x70101BFF   load2X32bits   (iLLD 拷贝)
+- 0x70101C00 ~ 0x70101CFF   writePage      (iLLD 拷贝)
+- 0x70101D00 ~ 0x70101FFF   预留
```

**编译要求**: 必须使用 `-O2` 优化，确保 `IfxScuWdt_*Inline` 函数真正内联。

#### 3.3.2 Flash 写入流程

```
Flash_writePFlash_portex(flashAddr, data, byteLength)
    |
    +- Step 0: 未对齐起始处理 (若 s_remainSize==0 且 flashAddr 未对齐)
    |           s_remainAddr = flashAddr & ~0x1F
    |           skip = flashAddr - s_remainAddr
    |           memset(s_remainBuffer, 0x00, 32)  // AURIX 擦除态
    |           memcpy(s_remainBuffer + skip, data, copySize)
    |           s_remainSize = skip + copySize
    |           if (s_remainSize >= 32) Flash_writeAndVerifyPage(s_remainAddr, s_remainBuffer)
    |
    +- Step 1: 拼接旧缓存 (若 s_remainSize > 0 且地址连续)
    |           memcpy(s_remainBuffer + s_remainSize, data, copySize)
    |           Flash_writeAndVerifyPage(s_remainAddr, s_remainBuffer)
    |
    +- Step 2: 批量写入完整 32B 页
    |           while (offset + 32 <= byteLength)
    |               Flash_writeAndVerifyPage(currentAddr, data + offset)
    |
    +- Step 3: 缓存剩余不足 32B 数据
                memset(s_remainBuffer, 0x00, 32)
                memcpy(s_remainBuffer, data + offset, remainSize)
                s_remainSize = remainSize
```

> **注意**: AURIX PFlash 擦除态为 `0x00`，故 pad 字节填 `0x00`。
> 若上位机 hex 文件地址未 32 字节对齐，建议使用 `align_hex.py` 预处理。

#### 3.3.3 FSR 错误检查

```c
/* Flash_Driver.h 定义的 FSR 错误掩码 */
#define FLASH_DRV_FSR_ERROR_MASK  (0x02003800U)
/* bit 11: OPER  (Operation Error)    */
/* bit 12: SQER  (Sequence Error)     */
/* bit 13: PROER (Protection Error)   */
/* bit 25: PVER  (Program Verify Error) */
/* bit 26: EVER  (Erase Verify Error)   */
```

每次擦写操作前后：
1. **前置清除**: `FLASH0_FSR.U = FLASH_DRV_FSR_ERROR_MASK;` (W1C)
2. **执行操作**: `enterPageMode -> load -> writePage -> waitUnbusy`
3. **后置检查**: `if (Flash_Drv_GetFSRError() != 0) -> 错误处理`

### 3.4 双 Bank 管理 (Boot_DualBank.c)

#### 3.4.1 DFlash 标志区结构

```c
typedef struct
{
    uint32 magic;           /* 0x5A5AA5A5 */
    uint32 activeBank;      /* 0=Bank A, 1=Bank B */
    uint32 bankA_valid;     /* Bank A CRC32 (0=未标记) */
    uint32 bankB_valid;     /* Bank B CRC32 */
    uint32 bankA_version;   /* Bank A 版本号 */
    uint32 bankB_version;   /* Bank B 版本号 */
    uint16 bootAttempts;    /* 连续启动尝试计数 */
    uint16 flags;           /* 保留 */
    uint32 sequence;        /* 单调递增序列号 */
    uint32 crc32;           /* 本结构前 36 bytes 的 CRC32 */
} BootFlagMain_t;
```

#### 3.4.2 双备份策略

| 场景 | 主区 | 影子区 | 行为 |
|------|------|--------|------|
| 主有效、影无效 | 有效 | 无效 | 用主区，自动修复影子 |
| 主无效、影有效 | 无效 | 有效 | 用影子，自动修复主区 |
| 两者均有效 | 有效 | 有效 | 比较 sequence，取较大者 |
| 两者均无效 | 无效 | 无效 | 初始化默认值 |

#### 3.4.3 启动失败回滚

```
bootAttempts 计数逻辑:

上电 -> SelectAndJump() -> bootAttempts++ -> 写入 DFlash -> 跳转 APP
    |
    +- APP 正常启动 -> APP 应调用 Boot_DualBank_ClearBootAttempts() 清零
    |
    +- APP 崩溃/Trap/WDT -> 下次上电 bootAttempts 继续累积
        |
        +- bootAttempts >= 3 -> 标记当前 Bank INVALID
                              -> 验证 fallback Bank
                              -> fallback 有效 -> 切换 activeBank -> SW_Reset()
                              -> fallback 无效 -> 留在 Bootloader
```

#### 3.4.4 裸跳转实现

```c
void Boot_DualBank_JumpToBank(uint32 bank)
{
    /* 1. 禁用中断 */
    IfxCpu_disableInterrupts();
    __dsync();
    
    /* 2. 禁用 ECC Trap (防止 APP 擦除态区域触发 Trap) */
    uint16 pwd = IfxScuWdt_getCpuWatchdogPassword();
    IfxScuWdt_clearCpuEndinit(pwd);
    FLASH0_MARP.B.TRAPDIS = 1;
    FLASH0_MARD.B.TRAPDIS = 1;
    IfxScuWdt_setCpuEndinit(pwd);
    
    /* 3. 禁用 Cache */
    IfxCpu_setDataCache(0);
    IfxCpu_setProgramCache(0);
    __dsync(); __isync();
    
    /* 4. 设置 APP 向量表 */
    Boot_DualBank_SetAppVectors(startAddr);
    
    /* 5. 切断 CSA 上下文 */
    __mtcr(CPU_PCXI, 0); __isync();
    
    /* 6. 裸跳转 (ji a15) */
    __asm("mov d15, %0"::"d"(entryAddr):"d15");
    __asm("mov.a a15, d15");
    __asm("mov.a a11, #0");
    __asm("ji a15");
    while(1);
}
```

**关键要求**:
- 必须用 `ji` (Jump Indirect)，不能用 `call`
- `PCXI = 0` 切断 CSA 链表，防止 APP 返回时 Bus Error
- `entryAddr = uncachedStart + 0x20` (跳过 BMHD)

---

## 4. App_dualBank 详解

### 4.1 项目结构

```
App_dualBank/
+-- AppSw/
|   +-- Tricore/
|   |   +-- App_UDS/           # 与 BootLoader 同步的 UDS 代码
|   |   +-- Main/Cpu0_Main.c   # APP 启动入口
|   |   +-- ...
|   +-- ...
+-- Debug/
|   +-- App_dualBank.hex       # Bank A 版本 (地址 0x8002xxxx)
+-- App_dualBank.hex           # Bank B 版本 (地址 0x8010xxxx)
+-- App_dualBank_a.lsl         # Bank A Linker Script
+-- App_dualbank_b.lsl         # Bank B Linker Script
+-- ...
```

### 4.2 Linker Script 配置

**Bank A** (`App_dualBank_a.lsl`):
```
LCF_INTVEC0_START  = 0x80090000   /* 中断向量表 */
LCF_TRAPVEC0_START = 0x80098000   /* Trap 向量表 */
代码/数据基址       = 0x80020000   /* Bank A 起始 */
```

**Bank B** (`App_dualbank_b.lsl`):
```
LCF_INTVEC0_START  = 0x80170000   /* 中断向量表 */
LCF_TRAPVEC0_START = 0x80178000   /* Trap 向量表 */
代码/数据基址       = 0x80100000   /* Bank B 起始 */
```

> **关键**: Bootloader 中的 `APP_INTTAB_OFFSET` / `APP_TRAPTAB_OFFSET` 必须与上述 LSL 配置一致。

### 4.3 APP -> Bootloader 跳转

APP 通过 UDS `0x10 02` (Programming Session) 请求进入 Bootloader:

```c
/* App_dualBank 中 uds_cfg.h 必须启用 DIAGNOSTIC_MODE_FOR_APP */
#define DIAGNOSTIC_MODE_FOR_APP
/* #define DIAGNOSTIC_MODE_FOR_BOOTLOADER */
```

收到 `0x10 02` 后:
1. 设置 RAM 标志: `*(uint16 *)0x7002DFFC = RAM_BOOT_MODE_APP`
2. 注册 Tx 回调: 正响应发送完成后执行 `DoResetToBootloader`
3. 回调中调用 `SW_Reset()` 软复位
4. 复位后 Bootloader 检测到 RAM 标志 -> 留在 Bootloader

---

## 5. ZcanProDll 安全访问库

### 5.1 功能概述

`ZcanProDll.dll` 是一个标准的 **ZCAN Pro 安全访问 DLL**，实现了 UDS `0x27` 服务的 Seed/Key 计算算法。

### 5.2 接口说明

DLL 导出标准接口，供 ZXDoc `ZSecurityAccessReq` 调用：

```c
/* DLL 入口 */
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved);

/* Seed/Key 算法函数 (由 ZCANPRO 框架约定) */
/* 具体函数签名取决于 ZCANPRO SDK 的 ZSecurityAccessReq 接口 */
```

### 5.3 在 shuaxie.py 中的使用

```python
KEY_DLL = r"E:\visualStudioCode\ZcanProDll\Debug\ZcanProDll.dll"

def security_access(uds, level, desc):
    sa_req = ZSecurityAccessReq(
        keyDllPath=KEY_DLL,        # 指定 DLL 路径
        srcAddr=PHY_ADDR,           # ECU 物理请求地址
        dstAddr=TESTER_ADDR,        # Tester 响应地址
        securityLevel=level,        # 1 或 3
        isExtend=False,
    )
    if uds.security_access(sa_req):
        app.log_i(f"[{desc}] Security Level {level} passed")
        return True
    else:
        app.log_e(f"[{desc}] Security Level {level} failed")
        return False
```

### 5.4 编译与部署

```
ZcanProDll/
+-- ZcanProDll.sln              # Visual Studio 解决方案
+-- ZcanProDll/
|   +-- ZcanProDll.vcxproj      # 项目文件
|   +-- dllmain.cpp             # DLL 入口
|   +-- framework.h             # 标准头文件
|   +-- Debug/
|       +-- ZcanProDll.dll      # 编译输出 (被 shuaxie.py 引用)
```

**编译步骤**:
1. 用 Visual Studio 2019/2022 打开 `ZcanProDll.sln`
2. 选择 Debug / Release 配置
3. 编译生成 `ZcanProDll.dll`
4. 确保 `shuaxie.py` 中的 `KEY_DLL` 路径指向正确的 DLL 文件

> **安全注意**: Seed/Key 算法是安全敏感代码，应保护好源码和 DLL，防止泄露。

---

## 6. shuaxie.py 刷写脚本

### 6.1 功能概述

`shuaxie.py` 是 **ZXDoc 刷写脚本**，实现了完整的 UDS 刷写时序。通过 ZXDoc 的 Python API 与 ECU 通信。

### 6.2 配置参数

```python
# ========== 用户配置区 ==========

# CAN 诊断 ID
PHY_ADDR      = 0x74C       # ECU 物理请求地址 (RX)
TESTER_ADDR   = 0x75C       # Tester 响应地址 (TX)
FUNC_ADDR     = 0x7DF       # 功能地址（会话保持）
CHANNEL       = 1           # CAN 通道号

# 刷写文件路径
HEX_FILE      = r"E:\workFiles\IEBS\tc234bootloader\App_dualBank\Debug\App_dualBank.hex"
HEX_FILE_B    = r"E:\workFiles\IEBS\tc234bootloader\App_dualBank\App_dualBank.hex"

# 安全访问 DLL
KEY_DLL       = r"E:\visualStudioCode\ZcanProDll\Debug\ZcanProDll.dll"

# 刷写目标 Bank
TARGET_BANK   = "B"         # "A" 或 "B"

# Bank 地址与大小
BANK_A_START_ADDR = 0x80020000
BANK_B_START_ADDR = 0x80100000
BANK_APP_A_SIZE   = 896 * 1024
BANK_APP_B_SIZE   = 1024 * 1024
```

### 6.3 刷写流程函数

| 函数 | 说明 |
|:-----|:-----|
| `session_control(uds, session_type)` | 10 服务：诊断会话控制 |
| `security_access(uds, level)` | 27 服务：安全访问 |
| `erase_target_bank(uds)` | 31 01 FF 00：逐个擦除目标 Bank Sector |
| `file_download(uds)` | 34/36/37：文件下载 |
| `uds_request(uds, sid, data)` | 通用 UDS 请求发送与响应检查 |

### 6.4 HEX 对齐预处理

由于 TC234 PFlash 要求 32 字节对齐写入，若 hex 文件地址未对齐，需先用 `align_hex.py` 预处理：

```bash
python align_hex.py input.hex output_aligned.hex 32
```

或在 `shuaxie.py` 中集成调用（见 `align_hex.py` 代码）。

---

## 7. 刷写流程实操

### 7.1 首次刷写（空片 -> Bank A）

```
1. ECU 上电，两 Bank 均无效 -> 进入 Bootloader
2. 运行 shuaxie.py（或手动发送 UDS）:
   10 03 -> 85 02 -> 27 01/02 -> 2E F15A
   10 02 -> 27 03/04
   31 01 FF 00 (擦除 S8~S22)
   34 00 44 80 02 00 00 00 00 0E 00 00  (Bank A, 896KB)
   36 01 ... -> 36 02 ... -> ... -> 37
   31 01 DFFF  (CRC 校验 + 标记有效)
   11 03  (SoftReset)
3. 复位后 Bootloader 检测到 Bank A 有效 -> 跳转 -> APP 启动
```

### 7.2 升级刷写（Bank A 运行中 -> 刷 Bank B）

```
1. 当前 Bank A 运行中，诊断仪请求刷写
2. 31 01 FF FD -> 返回目标 Bank = B
3. 擦除 S23~S26 (Bank B)
4. 34 地址 = 0x80100000 (Bank B)
5. 36/37 传输数据
6. 31 01 DFFF -> 校验 Bank B -> 标记有效 -> 切 activeBank = B
7. 11 03 -> 复位后启动 Bank B
8. Bank A 旧版本保留，若 Bank B 启动失败 3 次 -> 自动回滚到 Bank A
```

### 7.3 APP 回滚到 Bootloader（OTA 入口）

```
1. APP 运行中收到 10 02 (Programming Session)
2. APP 设置 RAM 标志 RAM_BOOT_MODE_APP @ 0x7002DFFC
3. APP 发送正响应 50 02，然后 SW_Reset
4. 复位后 Bootloader 检测到 RAM 标志 -> 留在 Bootloader
5. 诊断仪开始刷写流程
```

---

## 8. 常见问题与故障排查

### 8.1 跳转 APP 后停在 DEBUG 指令 (00 A0)

| 排查项 | 方法 |
|--------|------|
| HEX 文件版本 | 确认刷写的是目标 Bank 对应的版本 (A/B) |
| 地址对齐 | 使用 `align_hex.py` 预处理 hex 文件 |
| Flash 写入验证 | 检查 `g_flashLastErrorFSR` 是否非零 |
| BIV/BTV 偏移 | 确认 Bootloader 和 APP LSL 中的向量表偏移一致 |
| ECC Trap | 跳转前已禁用 `FLASH0_MARP/MARD.TRAPDIS` |

### 8.2 刷写时 0x36 返回 NRC 0x72

| 原因 | 排查 |
|------|------|
| Flash 写入失败 | 检查 `g_flashLastErrorFSR` (SQER/PROER/OPER/PVER) |
| 回读验证失败 | 地址未对齐导致数据错位 |
| 长度不对齐 | `Flash_writePFlashPage` 拒绝非 32 整数倍长度 |

### 8.3 Context Management Error Trap

| 原因 | 解决 |
|------|------|
| Flash busy 时从 PFlash 取指 | 确保编译优化为 `-O2`，函数内联到 PSPR |
| PSPR 地址冲突 | 确认 `RELOCATION_START_ADDR = 0x70101800` |
| 中断未关闭 | `Flash_writePFlash_portex` 已添加中断保护 |

### 8.4 Bank 切换后启动旧版本

| 原因 | 排查 |
|------|------|
| 0x34 地址错误 | 确认刷写地址是目标 Bank 的地址 |
| Active Bank 保护 | 确认没有覆盖当前运行 Bank |
| DFlash 标志未更新 | 检查 `31 01 DFFF` 是否成功执行 |

### 8.5 Security Access 失败

| 原因 | 解决 |
|------|------|
| DLL 路径错误 | 检查 `KEY_DLL` 路径是否存在 |
| 连续失败 3 次锁定 | 等待 10 秒后再试 |
| Seed/Key 算法不匹配 | 确认 Bootloader 和 DLL 使用相同算法 |

---

## 9. 附录

### 9.1 关键地址速查表

| 符号 | 地址 | 说明 |
|:-----|:-----|:-----|
| `BANK_A_START_ADDR` | `0x80020000` | Bank A cached 起始 |
| `BANK_B_START_ADDR` | `0x80100000` | Bank B cached 起始 |
| `BANK_A_UNCACHED` | `0xA0020000` | Bank A uncached 起始 |
| `BANK_B_UNCACHED` | `0xA0100000` | Bank B uncached 起始 |
| `DFLASH_FLAG_ADDR` | `0xAF000000` | DFlash 标志区 (uncached) |
| `RAM_BOOT_MODE_Addr` | `0x7002DFFC` | RAM 启动标志 |
| `PSPR_START` | `0x70100000` | PSPR 起始 |
| `RELOCATION_START_ADDR` | `0x70101800` | iLLD 拷贝区 |

### 9.2 关键常量速查表

| 常量 | 值 | 说明 |
|:-----|:---|:-----|
| `FLAG_MAGIC` | `0x5A5AA5A5` | DFlash 标志魔数 |
| `MAX_BOOT_ATTEMPTS` | `3` | 启动失败回滚阈值 |
| `PFLASH_PAGE_LENGTH` | `32` | PFlash 页大小 |
| `DFLASH_PAGE_LENGTH` | `8` | DFlash 页大小 |
| `BMHD_SIZE` | `0x20` | BMHD 偏移 |

### 9.3 编译与调试建议

1. **编译优化**: 必须使用 `-O2` (不能 `-O0`)，确保 iLLD inline 函数真正内联
2. **PSPR 检查**: 编译后查看 `.map` 文件，确认 Flash 操作函数地址在 `0x7010xxxx`
3. **向量表对齐**: Bootloader 和 APP 的 BIV/BTV 偏移必须一致
4. **HEX 对齐**: 刷写前用 `align_hex.py` 确保地址 32 字节对齐
5. **DFlash 初始化**: 首次上电或擦除后，DFlash 标志区需重新初始化

### 9.4 参考文档

- `Bootloader_OEM_Standard_Flow.md` -- 车企标准流程
- `Bootloader_OEM_Code_Implementation.md` -- 代码修改对照
- `A_B_DualBank_Bootloader_改造说明.md` -- 双 Bank 改造说明
- `UDS_A_B_DualBank_改造说明.md` -- UDS 刷写改造说明
- `修复记录_2026-05-11.md` -- 详细修复记录
- `修改记录_2026-05-12.md` -- Flash 驱动优化记录

---

> **文档维护**: 本文件由 Kimi Code CLI 根据项目实际代码生成。  
> **更新日期**: 2026-05-14  
> **下次更新**: 当代码结构或刷写流程发生重大变更时
