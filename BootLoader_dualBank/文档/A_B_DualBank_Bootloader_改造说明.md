# TC234 A/B Dual Bank Bootloader 改造说明

## 1. 项目概述

**目标**：将现有单分区备份恢复机制的 Bootloader 升级为**车企标准 A/B 双分区（Dual Bank）启动方案**。

**平台**：Infineon AURIX TC234 + Tasking 编译器  
**关键约束**：
- TC234 只有单 PFlash Bank（PF0），**无硬件 AB Swap**（不同于 TC3xx）
- PFlash 擦写时必须从 PSPR（RAM）执行，避免 RWW 冲突
- Bank 切换纯软件实现，通过 DFlash 标志区管理

---

## 2. 改造前后架构对比

### 改造前（单分区 + 备份恢复）
```
┌─────────────────────────────────────────────────────┐
│  Bootloader (S0-S5)          0x80000000 ~ 0x8001FFFF │
│  Reserved (S6-S7)            0x80020000 ~ 0x8003FFFF │
│  APP Bank A (S8-S22)         0x80040000 ~ 0x8013FFFF │  ← 唯一运行区
│  APP Backup Bank B (S23-S26) 0x80140000 ~ 0x801FFFFF │  ← 仅备份，不可启动
└─────────────────────────────────────────────────────┘
启动流程：上电 → Bootloader → 检查标志 → 恢复 Bank B 到 Bank A → 跳 Bank A
```

### 改造后（A/B 双分区独立启动）
```
┌─────────────────────────────────────────────────────┐
│  Bootloader (S0-S5)          0x80000000 ~ 0x8001FFFF │
│  Reserved (S6-S7)            0x80020000 ~ 0x8003FFFF │
│  APP Bank A (S8-S22)         0x80040000 ~ 0x8013FFFF │  ← 独立可启动
│  APP Bank B (S23-S26)        0x80140000 ~ 0x801FFFFF │  ← 独立可启动
│  DFlash 标志区                0xAF000000 ~ 0xAF001FFF │  ← 主/影子双备份
└─────────────────────────────────────────────────────┘
启动流程：上电 → Bootloader → 读 DFlash 标志 → 选择有效 Bank → 直接跳转
         Bank A 损坏 → 自动回滚到 Bank B（无需恢复操作）
```

> **核心差异**：旧方案是"备份+恢复"（Bank B 只是仓库），新方案是"双系统互为备份"（Bank A/B 均可独立启动，切换只需改标志）。

---

## 3. 新建/修改文件清单

| 文件 | 操作 | 说明 |
|------|------|------|
| `AppSw/Tricore/App_bootloader/Boot_DualBank.h` | 新建 | 双分区标志区数据结构、宏定义、函数声明 |
| `AppSw/Tricore/App_bootloader/Boot_DualBank.c` | 新建 | 标志区读写、CRC 校验、Bank 验证、跳转、回滚 |
| `AppSw/Tricore/App_bootloader/App_bootloader_cfg.h` | 修改 | 追加 Bank A/B 地址、DFlash 标志区地址定义 |
| `AppSw/Tricore/App_bootloader/App_bootloader.c` | 修改 | 集成双分区跳转，禁用旧单分区启动逻辑 |
| `AppSw/Tricore/Main/Cpu0_Main.c` | 修改 | 启动时初始化双分区标志并尝试跳转 APP |
| `BootLoader20250714_UDS_tasking622.lsl` | 修改 | 启用 `bmh_1` 在 Bank B 起始地址 |
| `IfxCpu_CStart0.c` | 修改 | 定义 `BootModeHeader_1[]` 指向 Bank B 入口 |

---

## 4. 各文件详细说明

### 4.1 `Boot_DualBank.h`（新建）

**用途**：定义 DFlash 标志区数据结构、Bank 地址常量、函数接口。

**核心数据结构**：

```c
/* 主标志区（40 bytes） */
typedef struct
{
    uint32 magic;           // 0x5A5AA5A5，标志区有效性魔数
    uint32 activeBank;      // 0=Bank A, 1=Bank B，当前指定启动的 Bank
    uint32 bankA_valid;     // Bank A 的 CRC32（0 表示未标记有效）
    uint32 bankB_valid;     // Bank B 的 CRC32
    uint32 bankA_version;   // Bank A 固件版本号
    uint32 bankB_version;   // Bank B 固件版本号
    uint16 bootAttempts;    // 当前 Bank 连续启动尝试计数
    uint16 flags;           // 保留标志位
    uint32 sequence;        // 单调递增序列号，用于主/影子区冲突裁决
    uint32 crc32;           // 本结构体前 36 bytes 的 CRC32
} BootFlagMain_t;

/* 影子标志区（40 bytes）：主区的冗余备份 */
typedef struct
{
    uint32 shadow_magic;
    uint32 shadow_activeBank;
    ... // 与主区一一对应
} BootFlagShadow_t;

/* 组合结构（80 bytes），占 10 个 DFlash Page（每页 8 bytes） */
typedef struct
{
    BootFlagMain_t   main;    // 0xAF000000
    BootFlagShadow_t shadow;  // 0xAF000100（偏移 256 bytes，不同物理页）
} DualBankFlags_t;
```

**关键常量**：
- `DFLASH_FLAG_ADDR = 0xAF000000` — 主标志区起始
- `DFLASH_FLAG_SHADOW_OFFSET = 0x100` — 影子区偏移（256 bytes，确保不在同一 DFlash 物理页）
- `MAX_BOOT_ATTEMPTS = 3` — 连续启动失败阈值，触发自动回滚
- `BANK_APP_SIZE = 384*1024` — 每个 Bank 的 APP 镜像大小

---

### 4.2 `Boot_DualBank.c`（新建）

**用途**：双分区启动管理的核心实现。

#### 4.2.1 CRC32 校验
```c
static uint32 Boot_CRC32(const uint8 *data, uint32 length)
```
- 采用 **IEEE 802.3 标准 CRC32** 查表法
- 用于：① 标志区自身完整性校验 ② Bank 镜像完整性校验
- 软件实现，不依赖 TC234 FCE 硬件（兼容性好，但后续可优化为硬件 CRC）

#### 4.2.2 标志区读写（双备份策略）
```c
boolean Boot_DualBank_ReadFlags(DualBankFlags_t* flags);
boolean Boot_DualBank_WriteFlags(const DualBankFlags_t* flags);
```

**读取策略**（防掉电/坏块）：
1. 分别校验主区和影子区的 `magic` + `crc32`
2. 若主区有效、影子区无效 → 用主区，并自动修复影子区
3. 若影子区有效、主区无效 → 用影子区，并自动修复主区
4. 若两者均有效 → 比较 `sequence` 号，取较大者（最新写入）
5. 若两者均无效 → 返回 FALSE，触发首次初始化

**写入策略**（防写入过程中掉电）：
1. 先擦除 DFlash Sector（8KB，主/影子在同一 Sector 内，只擦一次）
2. 写入主标志区（`0xAF000000`）
3. 写入影子标志区（`0xAF000100`）
4. 回读验证

> **为什么需要影子区？**  
> DFlash 写入是按 Page（8 bytes）进行的。如果在写入主区过程中掉电，主区可能不完整；此时影子区仍保存着上一份完整数据。通过 `sequence` 号可以判断哪个更新。

#### 4.2.3 Bank 验证
```c
BankStatus_t Boot_DualBank_VerifyBank(uint32 bank);
```
- 读取标志区中存储的该 Bank 的 CRC（`bankX_valid`）
- 若 `bankX_valid == 0` → 从未标记过有效，返回 `BANK_STATUS_INVALID`
- 计算该 Bank 实际 384KB 内容的 CRC32
- 与实际 CRC 比对：匹配 → `VALID`，不匹配 → `INVALID`

#### 4.2.4 Bank 跳转
```c
void Boot_DualBank_JumpToBank(uint32 bank);
```

**跳转步骤**：
1. 读取目标 Bank 起始地址的向量表
   - `offset 0`：MSP（Main Stack Pointer）
   - `offset 4`：Reset Handler 地址
2. 对 MSP 和 Reset Handler 做合法性检查（非 0、非 0xFFFFFFFF）
3. 关闭全局中断（`IfxCpu_disableInterrupts()`）
4. 设置 MSP（TriCore 的 A[10] 寄存器）
5. 设置 BIV/BTV（中断向量表/陷阱向量表基地址）
   - BIV = `BankBase + 0xC000`
   - BTV = `BankBase + 0xD000`
   - ⚠️ **此偏移需与 APP 工程的 LSL 中 INTTAB/TRAPTAB 位置匹配**
6. 通过函数指针跳转到 Reset Handler

> **注意**：该函数若跳转成功则**永不返回**。

#### 4.2.5 启动选择与自动回滚（核心）
```c
void Boot_DualBank_SelectAndJump(void);
```

**五级回滚触发逻辑**：

```
上电
  │
  ▼
读取 DFlash 标志
  │
  ├── 标志损坏 ──→ 尝试跳 Bank A（默认）
  │
  ▼
确定 targetBank = activeBank
  │
  ▼
验证 targetBank CRC
  │
  ├── Bank 无效 ──→ 验证 fallbackBank
  │                    ├── fallback 有效 → 切 activeBank=fallback，跳
  │                    └── fallback 无效 → 返回 Bootloader（双 Bank 均坏）
  │
  ▼
Bank 有效
  │
  ├── bootAttempts >= 3 ──→ 标记 target 无效，切到 fallback，软复位
  │                           （连续 3 次启动失败，判定为坏版本）
  │
  └── bootAttempts < 3 ──→ bootAttempts++，写标志，跳转
                              （APP 启动成功后应清 bootAttempts）
```

> **启动失败计数清零时机**：APP 正常启动后应在初始化阶段调用相关接口（或通过看门狗喂狗、心跳等机制）通知 Bootloader 本次启动成功。简化方案是：APP 在 `main()` 运行稳定后通过 DFlash 或 RAM 标志通知 Bootloader。若暂不实现，可用"启动后 30 秒无复位即认为成功"的简化策略。

#### 4.2.6 Bank 标记与切换
```c
void Boot_DualBank_MarkBankValid(uint32 bank, uint32 version);
void Boot_DualBank_SwitchBank(uint32 targetBank);
```

- **`MarkBankValid`**：在 UDS 刷写完成后调用。计算目标 Bank 的 CRC32，写入标志区。
- **`SwitchBank`**：切换 `activeBank`，清 `bootAttempts`，递增 `sequence`，然后触发 **软件复位**（`SW_Reset()`）。复位后 Bootloader 会读取新标志并跳转到新 Bank。

---

### 4.3 `App_bootloader_cfg.h`（修改）

**追加内容**：
```c
/* Bank cached (local) base addresses */
#define DUALBANK_APP_A_CACHED_ADDR      0x80020000u
#define DUALBANK_APP_B_CACHED_ADDR      0x80100000u
#define DUALBANK_APP_A_UNCACHED_ADDR    0xA0020000u
#define DUALBANK_APP_B_UNCACHED_ADDR    0xA0100000u
#define DUALBANK_APP_SIZE               (384u * 1024u)

/* DFlash flag area for dual-bank management */
#define DUALBANK_FLAG_ADDR              0xAF000000u
#define DUALBANK_FLAG_SHADOW_OFFSET     0x100u
```

**保留旧定义**：`FL_APP_FLAG_Addr`、`FL_APP_PRG_PFLASH_Addr` 等继续保留，确保与旧代码和 UDS 刷写模块的兼容性。

---

### 4.4 `App_bootloader.c`（修改）

#### 修改点 1：包含双分区头文件
```c
#include "Boot_DualBank.h"
```

#### 修改点 2：`AppBL_GotoAppSW()` 改为动态 Bank 跳转
```c
void AppBL_GotoAppSW(void)
{
    uint32 activeBank = Boot_DualBank_GetActiveBank();
    Boot_DualBank_JumpToBank(activeBank);
}
```
- 旧代码：固定跳转到 `0x80020000 + 0x20`
- 新代码：根据 `activeBank` 动态选择 Bank A 或 Bank B

#### 修改点 3：禁用旧单分区启动逻辑
```c
#if 0  /* Legacy single-bank backup/restore logic - disabled for Dual Bank */
    /* 原有 ramFlag/flashFlag 检查、Flash_RestoreDeletedBlocks() 等 */
#endif
```

**原因**：启动时的 Bank 选择已前移到 `Cpu0_Main.c` 的 `Boot_DualBank_SelectAndJump()`。若该函数返回，说明两 Bank 均无效，直接留在 Bootloader 等待 UDS 重刷即可，无需再执行旧的"备份恢复"流程。

> 保留 `#if 0` 而非删除，便于调试阶段对比和回退。

---

### 4.5 `Cpu0_Main.c`（修改）

**启动流程改造**：

```c
void core0_main(void)
{
    IfxScuWdt_disableCpuWatchdog(...);
    IfxScuWdt_disableSafetyWatchdog(...);
    IfxCpu_disableInterrupts();
    
    IfxScuClock_init();     // 时钟初始化
    delay_init();
    BrdLed_init();
    
    /* ===== 新增：A/B 双分区启动决策 ===== */
    Boot_DualBank_Init();
    Boot_DualBank_SelectAndJump();
    /* 若 SelectAndJump() 返回，说明两 Bank 均无效 */
    /* ==================================== */
    
    AppBL_init();           // 初始化 CAN/UDS/Flash/Timer
    while (TRUE)
    {
        AppBL_main();       // UDS 主循环
        BrdLed_main();
    }
}
```

**关键设计**：`Boot_DualBank_SelectAndJump()` 在**最早期**调用，此时 CAN、UDS、Flash 等均未初始化。若 Bank 有效则直接跳转，启动时间最短；若无效则继续进入 Bootloader 模式。

---

### 4.6 `BootLoader20250714_UDS_tasking622.lsl`（修改）

**修改**：启用 Bank B 的 Boot Mode Header 段

```lsl
group  bmh_0 (ordered, run_addr=0x80000000)
{
    select "*.bmhd_0";
}
group  bmh_1 (ordered, run_addr=0x80100000)    // ← 新增
{
    select "*.bmhd_1";
}
```

**用途**：
- `bmh_0` 在 `0x80000000`：Bootloader 的启动模式头，SSW 上电后首先解析
- `bmh_1` 在 `0x80100000`：Bank B 的启动模式头，提供硬件级冗余入口

> **注意**：TC234 的 SSW 通常只使用 `bmh_0`（由 BMI 配置决定），`bmh_1` 主要用于：① 某些安全启动场景 ② 软件解析备用 ③ 与 TC3xx 方案对齐的习惯性设计。

---

### 4.7 `IfxCpu_CStart0.c`（修改）

**修改**：定义 `BootModeHeader_1[]`

```c
const uint32 BootModeHeader_1[] = {
    0xA0100020,     /* STADBM: Bank B uncached start = 0xA0100020 (cached 0x80100020) */
    0xB3590170,     /* BMI=0170h, BMHDID=B359h */
    0xA0100020,     /* ChkStart */
    0xA0100020,     /* ChkEnd */
    0x9E8D0E7D,     /* CRCrange - PLACEHOLDER */
    0x6172F182,     /* !CRCrange */
    0x822DBDDD,     /* CRChead - PLACEHOLDER */
    0x7DD24222      /* !CRChead */
};
```

**说明**：
- `STADBM` 指向 Bank B 的 Reset 入口（`0xA0100020` = cached `0x80100020`）
- **⚠️ CRC 占位符警告**：当前 CRCrange 和 CRChead 是从 `bmh_0` 复制过来的，与 `bmh_1` 的实际内容不匹配。
- **必须操作**：使用 Infineon **MemTool** 或 **IfxBmhdGenerator** 工具，根据 `BootModeHeader_1` 的实际内容重新计算 CRC，然后替换占位符值。否则 SSW 在校验 BMHD 时可能报错。

---

## 5. DFlash 标志区布局

```
DFlash 地址空间（每 Sector 8KB）
├─ Sector 0 (0xAF000000 ~ 0xAF001FFF)
│   ├─ 0xAF000000 ~ 0xAF00004F  : 主标志区 Main (80 bytes)
│   │                              [magic][activeBank][bankA_valid][bankB_valid]
│   │                              [bankA_ver][bankB_ver][bootAtt][flags]
│   │                              [sequence][crc32]
│   │                              共 10 个 DFlash Page（每页 8 bytes）
│   │
│   ├─ 0xAF000100 ~ 0xAF00014F  : 影子标志区 Shadow (80 bytes)
│   │                              内容与主区相同，sequence 同步递增
│   │
│   └─ 0xAF000150 ~ 0xAF001FFF  : 保留（未使用）
│
├─ Sector 1 (0xAF002000 ~ 0xAF003FFF)
│   └─ 可用于 UDS 刷写临时缓冲、版本日志等
│
└─ ...
```

**主/影子区物理隔离**：虽然两者位于同一个 8KB DFlash Sector 内，但擦除时一次性擦除整个 Sector，写入时分别写入不同 Page。影子区的存在意义是：**写入主区过程中掉电 → 影子区仍为上一版有效数据**。

---

## 6. 完整启动流程时序图

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
    ├─ 关闭看门狗
    ├─ IfxScuClock_init()
    ├─ delay_init()
    ├─ BrdLed_init()
    │
    ▼
Boot_DualBank_Init()
    │─ 尝试读取 DFlash 主/影子标志区
    │─ 若有效：加载到内存全局变量 g_activeBank
    │─ 若无效：初始化默认标志（activeBank=A, bootAttempts=0, sequence=1）
    ▼
Boot_DualBank_SelectAndJump()
    │
    ├─ 读取标志区 ──→ 失败 ──→ 尝试跳 Bank A（默认）
    │
    ├─ 验证 targetBank CRC
    │       │
    │       ├─ 无效 ──→ 验证 fallbackBank
    │       │              ├─ 有效 → 写标志(activeBank=fallback, bootAttempts=1) → 跳转
    │       │              └─ 无效 → 返回 Bootloader（等重刷）
    │       │
    │       └─ 有效 ──→ 检查 bootAttempts
    │                      │
    │                      ├─ >=3 → 标记无效 → 切 fallback → SW_Reset()
    │                      │        （连续启动失败，自动回滚）
    │                      │
    │                      └─ <3  → bootAttempts++ → 写标志 → 跳转
    │                               （进入 APP）
    ▼
【若跳转成功，以下代码永不执行】
    │
    ▼
AppBL_init()
    │─ 初始化 CAN、UDS、Flash、Timer
    ▼
while(1) { AppBL_main(); }
    │─ UDS 诊断会话
    │─ 刷写服务（0x34/0x36/0x37）
    │─ 收到合法 APP 镜像后写入目标 Bank
    │─ 刷写完成 → Boot_DualBank_MarkBankValid(bank, version)
    │─ 若需激活 → Boot_DualBank_SwitchBank(bank) → SW_Reset()
```

---

## 7. UDS 刷写与 Bank 激活流程

### 7.1 首次刷写（空片）
1. ECU 上电，两 Bank 均无效 → 进入 Bootloader
2. 诊断仪通过 UDS 0x34/0x36/0x37 将 APP 写入 **Bank A**（`0x80020000`）
3. 刷写完成后，诊断仪发送 RoutineControl（0x31）或 Reset（0x11）
4. Bootloader 调用 `Boot_DualBank_MarkBankValid(BANK_A, version)`
5. 调用 `Boot_DualBank_SwitchBank(BANK_A)` → 软件复位
6. 复位后 `SelectAndJump()` 检测到 Bank A 有效 → 跳转 → APP 启动

### 7.2 升级刷写（A→B 滚动升级）
1. 当前 Bank A 运行中，诊断仪请求刷写
2. 诊断仪将新固件写入 **Bank B**（`0x80100000`）
3. 刷写完成后，Bootloader 调用 `Boot_DualBank_MarkBankValid(BANK_B, newVersion)`
4. 调用 `Boot_DualBank_SwitchBank(BANK_B)` → 软件复位
5. 复位后跳转到 Bank B 运行新固件
6. **旧版本仍保留在 Bank A**，若 Bank B 启动失败 3 次 → 自动回滚到 Bank A

> **车企 OTA 标准做法**：永远是"写非运行 Bank → 标记有效 → 切 Bank → 复位"。旧版本作为备份保留，升级失败可一键回滚。

---

## 8. 注意事项与后续必做工作

### 8.1 ⚠️ BMHD_1 CRC 必须重新计算
当前 `BootModeHeader_1` 中的 `CRCrange` 和 `CRChead` 是从 `bmh_0` 复制过来的占位符，与 `bmh_1` 的实际内容不匹配。

**解决方法**：
- 使用 Infineon **MemTool** → "BMHD Calculator" 功能
- 或使用 **IfxBmhdGenerator** 命令行工具
- 输入 STADBM、BMI、ChkStart、ChkEnd，工具自动生成 CRCrange、!CRCrange、CRChead、!CRChead
- 将生成的 4 个值填入 `IfxCpu_CStart0.c` 的 `BootModeHeader_1[]`

### 8.2 ⚠️ APP 工程必须提供两套 LSL
TC234 **没有 MMU**，无法做地址重映射。若 Bank B 的 APP 镜像使用 Bank A 的链接地址（`0x80020000` 开头），则跳转到 `0x80100000` 后所有函数调用、全局变量访问都会指向错误的物理地址。

**解决方案（二选一）**：

**方案 A：两套 LSL（推荐，最常用）**
- `App_BankA.lsl`：基地址 `0x80020000`
- `App_BankB.lsl`：基地址 `0x80100000`
- 编译脚本中根据目标 Bank 选择 LSL：
  ```
  ctc --lsl-file=App_BankA.lsl ... → 生成 App_BankA.hex
  ctc --lsl-file=App_BankB.lsl ... → 生成 App_BankB.hex
  ```
- 刷写 Bank B 时必须使用 `App_BankB.hex`

**方案 B：APP 自定位（高级）**
- APP 启动时读取自身所在 Bank 地址，动态计算 BIV/BTV/数据段偏移
- 需要 APP 的 startup 代码做重定位，复杂度高，一般不用

### 8.3 ⚠️ APP 需清零 bootAttempts
当前实现中，`bootAttempts` 在 Bootloader 跳转前递增，但 APP 成功启动后不会自动清零。若 APP 每次运行都正常，但反复复位（如电源抖动），`bootAttempts` 可能累积到 3 导致误回滚。

**建议方案**：
- **方案 1**：APP 在 `main()` 初始化稳定后，调用接口（或直接写 DFlash）清零 `bootAttempts`
- **方案 2**：在 APP 中设置一个 RAM 标志，Bootloader 检测到该标志说明上次启动成功，清零计数器
- **方案 3**：简化处理——`bootAttempts` 只在更新后第一次启动时有效，APP 运行 5 秒后清标志

### 8.4 ⚠️ Bank 跳转时 BIV/BTV 偏移需与 APP LSL 匹配
`Boot_DualBank_JumpToBank` 中 hardcode 了：
```c
const uint32 INTTAB_OFFSET  = 0x0000C000u;
const uint32 TRAPTAB_OFFSET = 0x0000D000u;
```
这些偏移必须与 APP 工程 LSL 中的 `INTTAB0` / `TRAPTAB0` 位置一致。若 APP 的向量表不在这些偏移处，需修改此处。

### 8.5 可选优化：硬件 CRC
当前使用软件 CRC32 查表法计算 384KB Bank 镜像，约需 **10~30ms**（取决于 CPU 频率）。若启动时间敏感，可改用 TC234 的 **FCE（Flexible CRC Engine）** 硬件模块，计算时间可降至 **<1ms**。

---

## 9. 调试建议

### 9.1 首次上电调试
1. 确保 DFlash 已擦除（标志区全 0xFF）
2. 上电后应在 `Boot_DualBank_Init()` 中检测到标志无效，初始化默认标志
3. 由于两 Bank 均无效，`SelectAndJump()` 返回，进入 Bootloader
4. 通过 UDS 刷写 APP 到 Bank A
5. 观察 `Boot_DualBank_MarkBankValid()` 是否成功写入 DFlash

### 9.2 强制触发回滚测试
1. 正常刷写 Bank A，确保可启动
2. 手动修改 DFlash 中的 `bankA_valid` 为一个错误 CRC（如 `0x12345678`）
3. 复位，Bootloader 应检测到 Bank A CRC 不匹配
4. 若 Bank B 也无效 → 留在 Bootloader
5. 若 Bank B 有效 → 自动跳转到 Bank B

### 9.3 查看标志区（内存调试）
在调试器中查看地址：
- `0xAF000000`：主标志区（`DualBankFlags_t` 结构）
- `0xAF000100`：影子标志区

关键字段：
- `magic` 应为 `0x5A5AA5A5`
- `activeBank`：`0`=A，`1`=B
- `bankA_valid` / `bankB_valid`：存储的 CRC32（0 表示未标记）
- `bootAttempts`：连续启动尝试次数
- `sequence`：写入次数（单调递增）

---

## 10. 总结

本次改造实现了 TC234 平台**软件级 A/B 双分区启动**的核心能力：

| 能力 | 状态 |
|------|------|
| DFlash 双备份标志区 | ✅ 完成 |
| CRC32 校验（标志区 + Bank 镜像） | ✅ 完成 |
| 启动失败计数 + 自动回滚 | ✅ 完成 |
| Bank 动态跳转（MSP + BIV/BTV） | ✅ 完成 |
| BMHD_1（Bank B 硬件入口） | ✅ 完成（需重算 CRC） |
| UDS 刷写后标记 Bank 有效 | ✅ 接口就绪 |
| UDS 刷写后切换 Bank | ✅ 接口就绪 |
| APP 工程两套 LSL | ⚠️ 需配套完成 |
| APP 启动成功后清 bootAttempts | ⚠️ 需配套完成 |

**下一步建议**：
1. 用 MemTool 生成 `bmh_1` 的正确 CRC
2. 改造 APP 工程，提供 Bank A/B 两套 LSL
3. 在 UDS 刷写完成流程中集成 `Boot_DualBank_MarkBankValid` + `Boot_DualBank_SwitchBank`
4. 在 APP 中添加启动成功标志清零机制
