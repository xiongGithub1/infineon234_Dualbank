# App_dualBank 车企标准优化文档

> **文档目的**：将 App_dualBank（APP 工程）按照车企标准进行优化，确保与 Bootloader 的 Dual-Bank 机制完全兼容、可追溯、可诊断。

---

## 一、关键发现：BIV/BTV 偏移严重错误！

### 问题描述
Bootloader 中原本硬编码的向量表偏移与 APP 实际 LSL 链接器配置**完全不匹配**：

| 项目 | 原硬编码值 | LSL 实际值（Bank A） | LSL 实际值（Bank B） |
|------|-----------|---------------------|---------------------|
| INTTAB 偏移 | `0xC000` | `0x80090000 - 0x80020000 = 0x70000` | `0x80170000 - 0x80100000 = 0x70000` |
| TRAPTAB 偏移 | `0xD000` | `0x80098000 - 0x80020000 = 0x78000` | `0x80178000 - 0x80100000 = 0x78000` |

**风险**：Bootloader 跳转前设置的 BIV/BTV 指向了错误的地址（`0x8002DFE0` 附近没有有效的向量表）。虽然 APP 的 `_Core0_start()` 会重新设置正确的 BIV/BTV，但在 Bootloader 跳转完成到 APP CStart 执行前的极短窗口内，如果发生 NMI 或不可屏蔽 Trap，CPU 将跳转到无效向量地址，导致致命错误。

### 修正方案
将 `Boot_DualBank.h` 中的偏移修正为实际值：
```c
#define APP_INTTAB_OFFSET               0x00070000u
#define APP_TRAPTAB_OFFSET              0x00078000u
```

> **验证来源**：`App_dualBank_a.lsl` 第 49~50 行、`App_dualbank_b.lsl` 第 50~51 行。

---

## 二、修改文件清单

| 序号 | 文件路径 | 修改类型 | 核心内容 |
|------|---------|---------|---------|
| 1 | `BootLoader_dualBank/AppSw/Tricore/App_bootloader/Boot_DualBank.h` | 修正 | BIV/BTV 偏移从 `0xC000/0xD000` 修正为 `0x70000/0x78000` |
| 2 | `App_dualBank/AppSw/Tricore/App_bootloader/Boot_DualBank.h` | 完全替换 | 与 Bootloader 版本同步（含阶段标识、偏移修正、新函数声明） |
| 3 | `App_dualBank/AppSw/Tricore/App_bootloader/Boot_DualBank.c` | 完全替换 | 与 Bootloader 版本同步（含 `g_bootPhase`、`Boot_DualBank_SetAppVectors`、统一 CRC 实现） |
| 4 | `App_dualBank/Cpu0_Main.c` | 重写 | 提前 `ClearBootAttempts`、增加 APP 阶段标识、复位原因记录 |

---

## 三、详细修改说明

### 3.1 Bootloader 侧修正：`APP_INTTAB_OFFSET` / `APP_TRAPTAB_OFFSET`

**文件**：`BootLoader_dualBank/AppSw/Tricore/App_bootloader/Boot_DualBank.h`

```c
// 修改前（错误）
#define APP_INTTAB_OFFSET               0x0000C000u
#define APP_TRAPTAB_OFFSET              0x0000D000u

// 修改后（正确，与 LSL 一致）
#define APP_INTTAB_OFFSET               0x00070000u
#define APP_TRAPTAB_OFFSET              0x00078000u
```

**影响函数**：`Boot_DualBank_SetAppVectors()`（行 532~548）

```c
void Boot_DualBank_SetAppVectors(uint32 bankStartAddr)
{
    uint32 appBiv = (bankStartAddr + APP_INTTAB_OFFSET) | 0x1FE0u;  // 现在正确指向 APP INTTAB
    uint32 appBtv = (bankStartAddr + APP_TRAPTAB_OFFSET);           // 现在正确指向 APP TRAPTAB
    __mtcr(CPU_BIV, appBiv);
    __isync();
    __mtcr(CPU_BTV, appBtv);
    __isync();
}
```

---

### 3.2 APP 侧同步：`Boot_DualBank.h` / `Boot_DualBank.c`

**背景**：APP 和 Bootloader 原本是两套独立的 `Boot_DualBank` 实现，存在以下隐患：

#### 隐患 1：CRC 计算方式不一致（严重）

| 工程 | `Boot_CalcFlagCRC` 实现 | 计算范围 |
|------|------------------------|---------|
| Bootloader（新） | `memcpy` struct → zero crc32 field → CRC32(40 bytes) | 全 40 字节（含 4 字节零填充） |
| APP（旧） | `Boot_CRC32(p, 36)` | 前 36 字节（排除 crc32 字段） |

**后果**：APP 调用 `Boot_DualBank_ClearBootAttempts()` 写入 DFlash 后，Bootloader 读取时 CRC 校验失败，导致标志被误判为损坏，触发重新初始化（丢失 activeBank、bank_valid 等关键信息）。

**解决方案**：将 APP 的 `Boot_DualBank.c` 完全替换为 Bootloader 版本，统一 CRC 计算逻辑。

#### 隐患 2：APP 版本缺少关键函数

APP 旧版本缺少：
- `Boot_CRC32_Update()` — 流式 CRC 更新（Bootloader UDS 刷写使用）
- `Boot_DualBank_VerifyBankWithCrc()` — 外部 CRC 比对
- `Boot_DualBank_SetAppVectors()` — BIV/BTV 设置
- `BootPhase_t` / `g_bootPhase` — 阶段标识

**解决方案**：完全同步 Bootloader 版本，确保两套代码逻辑一致。

---

### 3.3 APP 启动流程优化：`Cpu0_Main.c`

#### 优化 1：`ClearBootAttempts()` 提前调用

**修改前**：
```c
void core0_main(void)
{
    IfxScuClock_init();
    delay_init();
    IfxStm_init();
    Multican_init();       // CAN 初始化（可能失败/耗时）
    BrdLed_init();
    UdsInit(...);          // UDS 初始化（可能失败/耗时）
    Boot_DualBank_ClearBootAttempts();  // <-- 太晚了！
    ...
}
```

**问题**：如果 CAN/UDS 初始化过程中 APP 崩溃（如总线错误、内存错误），`bootAttempts` 不会被清除。Bootloader 下次启动时会认为 APP 启动失败，计数器 +1，连续 3 次后自动回滚到旧版本。

**修改后**：
```c
void core0_main(void)
{
    IfxScuClock_init();
    Boot_DualBank_ClearBootAttempts();  // <-- 时钟稳定后立即清除
    delay_init();
    IfxStm_init();
    Multican_init();
    ...
}
```

**OEM 依据**：`ClearBootAttempts` 必须在 APP 初始化最早期、最稳定的时刻调用，确保只要 CPU 能执行到此处，就认定本次启动成功。

#### 优化 2：增加 APP 阶段标识

**新增代码**：
```c
typedef enum
{
    APP_PHASE_INIT    = 0xA0u,  /* APP early initialization */
    APP_PHASE_PERIPH  = 0xA1u,  /* APP peripheral initialization */
    APP_PHASE_RUN     = 0xA2u,  /* APP main loop running */
    APP_PHASE_ERROR   = 0xAFu   /* APP unrecoverable error */
} AppPhase_t;

volatile AppPhase_t g_appPhase = APP_PHASE_INIT;
```

**使用位置**：
| 阶段 | 代码位置 | 说明 |
|------|---------|------|
| `APP_PHASE_INIT` | `core0_main()` 入口 | 看门狗关闭、复位原因记录、时钟初始化 |
| `APP_PHASE_PERIPH` | `Multican_init()` 前 | CAN、LED、UDS 等外设初始化 |
| `APP_PHASE_RUN` | `while(TRUE)` 前 | 主循环运行中 |

**调试价值**：通过调试器查看 `g_appPhase` 和 `g_bootPhase`（后者在 Bootloader 的 DFlash 管理代码中），可以完整追溯从复位 → Bootloader → APP 的全流程：
- `g_bootPhase = 0x04` (JUMP_EXEC) → `g_appPhase = 0xA0` (INIT) → `g_appPhase = 0xA2` (RUN)
- 如果 `g_appPhase` 卡在 `0xA1`，说明外设初始化阶段出错。

#### 优化 3：复位原因记录

```c
resetReason = ResetStatus_Previous();
isPowerOnReset = (resetReason & 0x01) != 0;
```

与 Bootloader 保持一致，便于故障分析时判断是上电复位、软件复位还是看门狗复位。

---

## 四、OEM 合规检查清单（APP 侧）

| 检查项 | 状态 | 说明 |
|--------|------|------|
| APP 启动后清除 `bootAttempts` | ✅ 优化 | 从 UDS 初始化后提前到时钟初始化后 |
| `ClearBootAttempts` 在风险操作前调用 | ✅ 优化 | 在 CAN/UDS/Flash 初始化之前 |
| APP 与 Bootloader DFlash 标志 CRC 兼容 | ✅ 修复 | 统一 `Boot_CalcFlagCRC` 实现 |
| BIV/BTV 偏移与 APP LSL 一致 | ✅ 修正 | `0x70000` / `0x78000`（原 `0xC000`/`0xD000` 错误） |
| APP 阶段标识（可追溯） | ✅ 新增 | `AppPhase_t` + `g_appPhase` |
| 复位原因记录 | ✅ 启用 | `ResetStatus_Previous()` |
| RAM 标志支持进入 Bootloader | ✅ 已有 | UDS 0x10 02 / 0x31 jumpToBL |
| APP 向量表由 CStart 正确设置 | ✅ 已有 | `IfxCpu_CStart0.c:124~127` |

---

## 五、风险与注意事项

### 5.1 DFlash 标志兼容性

由于统一了 `Boot_CalcFlagCRC` 实现（从 APP 的 36 字节 CRC 改为 Bootloader 的 40 字节 CRC），**已写入 DFlash 的旧标志在新代码下会被判定为 CRC 错误**。

**应对措施**：
- `Boot_DualBank_Init()` 会在 CRC 失败时自动重新初始化标志（默认值：activeBank=A, 两 Bank 均 INVALID）。
- 这意味着升级后首次启动会进入 Bootloader，需要重新刷写至少一个 Bank。
- **建议**：在产线或售后升级时，确保升级包包含完整的 Bootloader + APP，升级后立即刷写新 APP。

### 5.2 编译验证

`Boot_DualBank.c` 从 Bootloader 复制到 APP 后，包含以下头文件：
```c
#include "Can.h"      /* APP 工程中路径正确 */
#include "Tmr.h"      /* APP 工程中路径正确 */
#include "Bsp.h"      /* APP 工程中路径正确 */
```

这些头文件在 APP 工程中均存在，但需在编译时验证 include path 是否覆盖。如果编译报错，检查项目配置中的 Include Paths 是否包含 `AppSw/Tricore` 或相应子目录。

### 5.3 LSL 向量表位置变更

如果未来 APP 的 LSL 文件修改了 `LCF_INTVEC0_START` 或 `LCF_TRAPVEC0_START`，必须同步更新：
- `Boot_DualBank.h` 中的 `APP_INTTAB_OFFSET` / `APP_TRAPTAB_OFFSET`
- 两个工程（Bootloader + APP）必须同时更新，保持一致

---

## 六、调试指南

### 6.1 典型故障场景追溯

| 现象 | 查看变量 | 分析 |
|------|---------|------|
| 上电后反复复位，最终进 Bootloader | `g_bootPhase = 0x05` (ROLLBACK) | APP 连续 3 次启动失败，触发回滚 |
| APP 启动后瞬间崩溃 | `g_appPhase = 0xA0` (INIT) | APP 在 `ClearBootAttempts` 之后、外设初始化之前崩溃 |
| APP 外设初始化阶段崩溃 | `g_appPhase = 0xA1` (PERIPH) | CAN/UDS/LED 初始化出错 |
| 刷写后 APP 不启动 | `g_bootPhase = 0x21` (PROG_VERIFY) | CRC 校验通过但跳转未执行，检查 UDS 0x31 02 |
| Bootloader 跳转后黑屏 | `g_bootPhase = 0x04` (JUMP_EXEC) | 裸跳转执行了但 APP CStart 未正确运行，检查 BIV/BTV/入口地址 |

### 6.2 验证 BIV/BTV 一致性

在 APP 的 `core0_main()` 断点处，检查：
```c
(uint32)__mfcr(CPU_BIV)  == ((BANK_A_START_ADDR + APP_INTTAB_OFFSET) | 0x1FE0)
(uint32)__mfcr(CPU_BTV)  == (BANK_A_START_ADDR + APP_TRAPTAB_OFFSET)
```

如果 Bootloader 跳转前设置的 BIV 与 APP CStart 重新设置的 BIV 不一致，说明偏移值仍有错误。
