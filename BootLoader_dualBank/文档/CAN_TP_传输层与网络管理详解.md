# CAN TP 传输层原理与网络管理详解

> 基于 BootLoader20250714_UDS_tasking622 项目代码，结合 ISO 15765-2、ISO 14229 标准讲解

---

## 1. 为什么需要 CAN TP 传输层？

### 1.1 CAN 帧的先天限制

| 特性 | CAN 2.0 | CAN FD |
|------|---------|--------|
| 数据段最大长度 | **8 字节** | **64 字节** |
| 帧 ID 长度 | 11 bit (标准帧) / 29 bit (扩展帧) | 同左 |

UDS 诊断报文长度范围：
- 单服务请求：3~10 字节（如 `10 02`）
- 长数据传输：`0x34/0x36/0x37` 刷写时，一次传输可能达 **几十到几百字节**
- 读取 DID：`0x22` 响应可能包含 VIN（17 字节）、软件版本等

**结论**：CAN 硬件一次最多发 8 字节，但 UDS 需要传输更长的数据。**CAN TP（ISO 15765-2）** 就是在 CAN 数据链路层之上实现**分段传输（Segmentation）**的协议。

### 1.2 分层架构

```
┌─────────────────────────────────────────┐
│  UDS Application Layer (ISO 14229)      │  ← 0x10/0x27/0x34/0x36...
│  诊断服务实现（uds_app.c）               │
├─────────────────────────────────────────┤
│  CAN Transport Layer (ISO 15765-2)      │  ← 分段/重组/流控
│  TP 状态机（uds_tp.c）                   │
├─────────────────────────────────────────┤
│  CAN Data Link Layer (ISO 11898)        │  ← 硬件发送/接收
│  MultiCAN 驱动 + TLE9252 收发器         │
├─────────────────────────────────────────┤
│  Physical Layer (双绞线)                 │  ← 差分信号
└─────────────────────────────────────────┘
```

> **关键概念**：TP 层对 UDS 层是透明的。UDS 层只管"发一条 100 字节的数据"，TP 层自动把它切成十几个 CAN 帧发出去，对方 TP 层再重组回 100 字节交给对端 UDS。

---

## 2. CAN TP 的四种帧类型

CAN TP 定义了 **4 种 N_PDU（Network Protocol Data Unit）**，通过 CAN 帧的第一个字节（PCI，Protocol Control Information）的高 4 位区分：

| 类型 | 名称 | PCI 高 4 位 | 用途 |
|------|------|-------------|------|
| **SF** | Single Frame（单帧） | `0x0` | 数据 ≤ 7 字节，一帧发完 |
| **FF** | First Frame（首帧） | `0x1` | 数据 > 7 字节，发送第一帧 |
| **CF** | Consecutive Frame（连续帧） | `0x2` | 首帧之后，逐帧发送剩余数据 |
| **FC** | Flow Control（流控帧） | `0x3` | 接收方控制发送方的发送节奏 |

### 2.1 PCI 格式详解

#### SF — 单帧（数据长度 ≤ 7）

```
CAN 帧数据场（8 字节）：
├─ Byte0: 0x0L  ── 高 4 位 = 0（SF），低 4 位 = 数据长度 L（1~7）
├─ Byte1~ByteL: 实际数据
└─ Byte(L+1)~Byte7: 填充（通常填 0xAA 或 0x55）

示例：UDS 请求 10 02（进入编程会话）
┌──┬──┬──┬──┬──┬──┬──┬──┐
│02│10│02│AA│AA│AA│AA│AA│  ← PCI=0x02（SF，长度=2），数据=10 02
└──┴──┴──┴──┴──┴──┴──┴──┘
```

#### FF — 首帧（数据长度 > 7）

```
CAN 帧数据场（8 字节）：
├─ Byte0: 0x1X  ── 高 4 位 = 1（FF），低 4 位 = 总长的高 4 位
├─ Byte1: 0xXX  ── 总长的低 8 位（与 Byte0 组成 12 位长度，最大 4095）
├─ Byte2~Byte7: 前 6 字节数据

示例：发送 32 字节数据
总长度 = 32 = 0x020
┌──┬──┬──┬──┬──┬──┬──┬──┐
│10│20│D1│D2│D3│D4│D5│D6│  ← PCI=0x10 0x20（FF，长度=0x020=32）
└──┴──┴──┴──┴──┴──┴──┴──┘
      ↑ 低 8 位长度
   ↑ 高 4 位长度
```

#### CF — 连续帧

```
CAN 帧数据场（8 字节）：
├─ Byte0: 0x2N  ── 高 4 位 = 2（CF），低 4 位 = 序列号 SN（0~15，循环）
├─ Byte1~Byte7: 7 字节数据

序列号 SN 规则：
- 首帧之后的第一个 CF，SN = 1
- 之后每发一帧 CF，SN 递增（2, 3, ..., 15, 0, 1, ...）
- 接收方如果检测到 SN 不连续，报 N_WRONG_SN 错误

示例：接上面的 32 字节传输
FF 已发 6 字节，剩余 26 字节，需要 ⌈26/7⌉ = 4 个 CF

CF #1: ┌──┬──┬──┬──┬──┬──┬──┬──┐   SN=1
        │21│D7│D8│D9│D10│D11│D12│D13│
        └──┴──┴──┴──┴──┴──┴──┴──┘

CF #2: ┌──┬──┬──┬──┬──┬──┬──┬──┐   SN=2
        │22│D14│D15│D16│D17│D18│D19│D20│
        └──┴──┴──┴──┴──┴──┴──┴──┘

CF #3: ┌──┬──┬──┬──┬──┬──┬──┬──┐   SN=3
        │23│D21│D22│D23│D24│D25│D26│D27│
        └──┴──┴──┴──┴──┴──┴──┴──┘

CF #4: ┌──┬──┬──┬──┬──┬──┬──┬──┐   SN=4（只剩 5 字节有效）
        │24│D28│D29│D30│D31│D32│XX│XX│
        └──┴──┴──┴──┴──┴──┴──┴──┘
```

#### FC — 流控帧

```
CAN 帧数据场（8 字节）：
├─ Byte0: 0x3X  ── 高 4 位 = 3（FC），低 4 位 = FS（Flow Status）
├─ Byte1: BS     ── Block Size，允许连续发送的 CF 数量（0 = 无限）
├─ Byte2: STmin  ── 发送方两帧 CF 之间的最小间隔时间
└─ Byte3~Byte7: 填充

FS（Flow Status）值：
- 0x0 = CTS（Continue To Send）→ 继续发送
- 0x1 = WT（Wait）→ 暂停发送，发送方等待下一个 FC
- 0x2 = OVFLW（Overflow）→ 接收方缓存溢出，终止传输

BS（Block Size）值：
- 0x00 → 无限大，不需要中间插入 FC，发完为止
- 0x01~0xFF → 发送方连续发 BS 个 CF 后，必须等待下一个 FC

STmin（Separation Time minimum）值：
- 0x00~0x7F → 单位毫秒（ms），如 0x01 = 1ms
- 0x80~0xF0 → 保留
- 0xF1~0xF9 → 单位 100 微秒（μs），如 0xF1 = 100μs = 0.1ms

示例：接收方要求发送方每发 8 个 CF 停一次，间隔 5ms
┌──┬──┬──┬──┬──┬──┬──┬──┐
│30│08│05│AA│AA│AA│AA│AA│  ← FS=CTS, BS=8, STmin=5ms
└──┴──┴──┴──┴──┴──┴──┴──┘
```

---

## 3. 分段传输完整流程（图解）

### 3.1 场景：诊断仪向 ECU 发送 30 字节刷写数据

```
诊断仪（Sender）                              ECU（Receiver）
    │                                              │
    │  ┌──────────────────────────────────────┐   │
    │  │ FF: PCI=0x10 0x1E, Data[0:5]        │   │
    │  │ 总长度=30，首帧携带 6 字节            │   │
    │  └──────────────────────────────────────┘   │
    │ ──────────────────────────────────────────> │
    │                                              │ 收到 FF，知道总长 30
    │                                              │ 已收 6，还差 24
    │                                              │
    │  ┌──────────────────────────────────────┐   │
    │  │ FC: PCI=0x30, BS=0, STmin=1ms        │   │
    │  │ 无限接收，间隔至少 1ms                │   │
    │  └──────────────────────────────────────┘   │
    │ <────────────────────────────────────────── │
    │                                              │
    │ 等待 STmin=1ms...                            │
    │                                              │
    │  ┌──────────────────────────────────────┐   │
    │  │ CF#1: PCI=0x21, Data[6:12]           │   │
    │  └──────────────────────────────────────┘   │
    │ ──────────────────────────────────────────> │ 收 7 字节，累计 13
    │ 等待 STmin=1ms...                            │
    │                                              │
    │  ┌──────────────────────────────────────┐   │
    │  │ CF#2: PCI=0x22, Data[13:19]          │   │
    │  └──────────────────────────────────────┘   │
    │ ──────────────────────────────────────────> │ 收 7 字节，累计 20
    │ 等待 STmin=1ms...                            │
    │                                              │
    │  ┌──────────────────────────────────────┐   │
    │  │ CF#3: PCI=0x23, Data[20:26]          │   │
    │  └──────────────────────────────────────┘   │
    │ ──────────────────────────────────────────> │ 收 7 字节，累计 27
    │ 等待 STmin=1ms...                            │
    │                                              │
    │  ┌──────────────────────────────────────┐   │
    │  │ CF#4: PCI=0x24, Data[27:29] (3字节)   │   │
    │  └──────────────────────────────────────┘   │
    │ ──────────────────────────────────────────> │ 收 3 字节，累计 30 ✓
    │                                              │ 重组完成，交给 UDS 层
    │                                              │
```

### 3.2 场景：ECU 向诊断仪发送 50 字节正响应（如 `0x22` 读 DID）

```
ECU（Sender）                                 诊断仪（Receiver）
    │                                              │
    │  UDS 层构造 50 字节响应数据                   │
    │  TP 层判断 > 7 字节 → 需要分段                │
    │                                              │
    │  FF ──────────────────────────────────────> │
    │ <─────────────────────────────────────── FC │ BS=8, STmin=5ms
    │                                              │
    │  CF#1~CF#8 ─────────────────────────────> │ 连续发 8 帧
    │ <─────────────────────────────────────── FC │ BS=8, STmin=5ms
    │                                              │
    │  CF#9~CF#15 ────────────────────────────> │ 再发 7 帧（共 50 字节）
    │                                              │ 接收完成
```

---

## 4. 超时机制（ISO 15765-2 Timing）

CAN TP 定义了 6 个定时器，任何一方超时都会导致传输失败。

### 4.1 超时参数定义

| 定时器 | 方向 | 含义 | 本项目配置值 | 触发条件 |
|--------|------|------|-------------|----------|
| **N_As** | TX | 发送方发送**任意一帧**的最大时间 | 25 ms | 发完 SF/FF/CF/FC 后等待硬件确认超时 |
| **N_Ar** | RX | 接收方发送**FC 帧**的最大时间 | 25 ms | 收到 FF/CF 块后，发 FC 前等待超时 |
| **N_Bs** | TX | 发送方等待**FC 帧**的最大时间 | 75 ms | 发完 FF 或一批 CF 后，等待 FC 超时 |
| **N_Br** | RX | 接收方发送**FC 帧**前的最大等待时间 | 0 ms | 本项目设为 0，即收到 FF 立即发 FC |
| **N_Cs** | TX | 发送方发送**下一帧 CF** 前的最大时间 | 25 ms | 收到 FC 后，发第一个 CF 前等待超时 |
| **N_Cr** | RX | 接收方等待**下一帧 CF** 的最大时间 | 150 ms | 收到一帧 CF 后，等待下一帧 CF 超时 |

> **记忆口诀**：
> - N**A**s / N**A**r → **A**ny frame（任意帧的发送/等待）
> - N**B**s / N**B**r → **B**lock（块传输中的 FC 相关）
> - N**C**s / N**C**r → **C**onsecutive（连续帧的发送/等待）

### 4.2 超时错误码

```c
typedef enum
{
    N_OK = 0,           /* 成功 */
    N_TIMEOUT_A,        /* N_As / N_Ar 超时 */
    N_TIMEOUT_Bs,       /* N_Bs 超时（等不到 FC） */
    N_TIMEOUT_Cr,       /* N_Cr 超时（等不到下一 CF） */
    N_WRONG_SN,         /* 序列号不连续 */
    N_INVALID_FS,       /* FS 值非法 */
    N_UNEXP_PDU,        /* 收到意外的 PDU */
    N_WTF_OVRN,         /* Wait 帧次数超限 */
    N_BUFFER_OVFLW,     /* 接收缓存溢出 */
    N_ERROR             /* 通用错误 */
} tN_Result;
```

### 4.3 本项目超时实现

```c
/* uds_tp.c */
static tUdsCANNetLayerCfg g_stCANUdsNetLayerCfgInfo =
{
    1u,     /* TP 主函数调用周期 = 1ms */
    0u,     /* RX 功能寻址 ID（运行时由 UdsInit 设置） */
    0u,     /* RX 物理寻址 ID */
    0u,     /* TX 响应 ID */
    0u,     /* BS = 0，不限制连续帧数量 */
    1u,     /* STmin = 1ms */
    25u,    /* N_As */
    25u,    /* N_Ar */
    75u,    /* N_Bs */
    0u,     /* N_Br = 0，立即回复 FC */
    25u,    /* N_Cs */
    150u,   /* N_Cr */
    50u,    /* TX 最大阻塞时间 */
    ...
};
```

**Tick 递减**（在 1ms 定时器中断中执行）：
```c
void TP_SystemTickCtl(void)
{
    if (gs_stCanTPRxDataInfo.xSTmin)        { gs_stCanTPRxDataInfo.xSTmin--; }
    if (gs_stCanTPRxDataInfo.xMaxWatiTimeout) { gs_stCanTPRxDataInfo.xMaxWatiTimeout--; }
    if (gs_stCanTPTxDataInfo.xSTmin)        { gs_stCanTPTxDataInfo.xSTmin--; }
    if (gs_stCanTPTxDataInfo.xMaxWatiTimeout) { gs_stCanTPTxDataInfo.xMaxWatiTimeout--; }
    if (gs_CANTPTxMsgMaxWaitTime)           { gs_CANTPTxMsgMaxWaitTime--; }
}
```

---

## 5. 本项目 TP 状态机详解

### 5.1 状态定义

```c
typedef enum
{
    IDLE,           /* 空闲 */
    RX_SF,          /* 等待接收单帧 */
    RX_FF,          /* 等待接收首帧 */
    RX_FC,          /* 等待接收流控帧 */
    RX_CF,          /* 等待接收连续帧 */
    TX_SF,          /* 发送单帧 */
    TX_FF,          /* 发送首帧 */
    TX_FC,          /* 发送流控帧 */
    TX_CF,          /* 发送连续帧 */
    WAITING_TX,     /* 等待发送 */
    WAIT_CONFIRM    /* 等待发送确认 */
} tCanTpWorkStatus;
```

### 5.2 RX 接收状态机（ECU 接收诊断仪数据）

```
                        ┌─────────────┐
      CAN 帧到来 ──────>│    IDLE     │
                        └──────┬──────┘
                               │
              ┌────────────────┼────────────────┐
              │                │                │
         PCI=SF(0x0)     PCI=FF(0x1)      PCI=FC(0x3)
              │                │                │
              ▼                ▼                ▼
        ┌─────────┐     ┌─────────┐      ┌─────────┐
        │  RX_SF  │     │  RX_FF  │      │  RX_FC  │
        │处理单帧  │     │收首帧    │      │收流控   │
        └────┬────┘     └────┬────┘      └────┬────┘
             │               │                 │
             │               │ 发 FC 帧        │ 解析 BS/STmin
             │               ▼                 ▼
             │          ┌─────────┐       ┌─────────┐
             │          │  TX_FC  │       │  TX_CF  │
             │          │(作为接收方)│      │(作为发送方)│
             │          └────┬────┘       └────┬────┘
             │               │                 │
             └───────────────┴─────────────────┘
                             │
                             ▼
                    UDS 层读取完整数据
```

### 5.3 TX 发送状态机（ECU 发送响应数据）

```
UDS 层有数据要发 ──> 判断长度
    │
    ├── ≤ 7 字节 ──> TX_SF（发单帧）
    │
    └── > 7 字节 ──> TX_FF（发首帧）──> 等 FC ──> TX_CF（发连续帧）
                                              │
                                              ├── 若 BS ≠ 0，发 BS 个后等下一个 FC
                                              │
                                              └── 若 BS = 0，一直发到完
```

### 5.4 关键代码：接收首帧 + 发送流控

```c
/* uds_tp.c: CANTP_DoReceiveFF() */
static void CANTP_DoReceiveFF(tUdsCANMsgInfo *msg)
{
    uint16 totalLen;
    
    /* 从 PCI 提取 12 位总长度 */
    totalLen = ((msg->data[0] & 0x0F) << 8) | msg->data[1];
    
    /* 保存总长度，拷贝首帧中的 6 字节数据 */
    gs_stCanTPRxDataInfo.usDataLen = totalLen;
    gs_stCanTPRxDataInfo.usAlreadyReceivedDataLen = 6;
    memcpy(gs_stCanTPRxDataInfo.dataBuf, &msg->data[2], 6);
    
    /* 下一期望的序列号 = 1 */
    gs_stCanTPRxDataInfo.ucExpectedSN = 1;
    
    /* 状态切换：准备发送 FC */
    SetCurCANTPSatus(TX_FC);
}

/* uds_tp.c: CANTP_DoTransmitFC() */
static void CANTP_DoTransmitFC(void)
{
    uint8 fcFrame[8];
    
    fcFrame[0] = 0x30;                              /* FS = CTS (0x0), PCI = 0x3 */
    fcFrame[1] = g_stCANUdsNetLayerCfgInfo.xBlockSize;  /* BS */
    fcFrame[2] = g_stCANUdsNetLayerCfgInfo.xSTmin;      /* STmin */
    memset(&fcFrame[3], 0xAA, 5);                   /* 填充 */
    
    /* 写入 TX BUS FIFO，由 CAN 驱动发送 */
    CANTP_TxMsg(fcFrame, 8);
    
    /* 启动 N_Cr 超时定时器（等待第一个 CF） */
    RXFrame_SetRxMsgWaitTime(g_stCANUdsNetLayerCfgInfo.xNCr);
    
    /* 状态切换：等待连续帧 */
    SetCurCANTPSatus(RX_CF);
}
```

---

## 6. FIFO / 队列管理

本项目 TP 层使用 **4 个 FIFO** 实现数据缓冲：

```
┌─────────────────────────────────────────────────────────────┐
│                    CAN 硬件接收中断                          │
│         收到 0x7DF 或 0x74C 的 CAN 帧                        │
└──────────────────────┬──────────────────────────────────────┘
                       │ 写入
                       ▼
              ┌─────────────────┐
              │  RX_BUS_FIFO    │  ← 原始 CAN 帧（8 字节/帧）
              │   ('r' queue)   │    长度：300 字节
              └────────┬────────┘
                       │ CANTP_MainFun() 读取
                       ▼
              ┌─────────────────┐
              │   TP 状态机      │  ← 分段重组
              │  (SF/FF/CF/FC)  │
              └────────┬────────┘
                       │ 重组完成
                       ▼
              ┌─────────────────┐
              │  RX_TP_QUEUE    │  ← 完整 UDS 请求报文
              │   ('R' queue)   │    长度：150 字节
              └────────┬────────┘
                       │ UDS_MainFun() 读取
                       ▼
              ┌─────────────────┐
              │   UDS 服务处理   │  ← 执行 0x10/0x27/0x34...
              │   (uds_app.c)   │
              └────────┬────────┘
                       │ 构造响应
                       ▼
              ┌─────────────────┐
              │  TX_TP_QUEUE    │  ← 完整 UDS 响应报文
              │   ('T' queue)   │    长度：150 字节
              └────────┬────────┘
                       │ CANTP_MainFun() 读取
                       ▼
              ┌─────────────────┐
              │   TP 状态机      │  ← 分段发送
              │  (SF/FF/CF/FC)  │
              └────────┬────────┘
                       │
                       ▼
              ┌─────────────────┐
              │  TX_BUS_FIFO    │  ← 原始 CAN 帧（8 字节/帧）
              │   ('t' queue)   │    长度：300 字节
              └────────┬────────┘
                       │ SendMsgMainFun() 读取
                       ▼
              ┌─────────────────┐
              │   CAN 硬件发送   │
              └─────────────────┘
```

**FIFO 实现**（`uds_fifo.c`）是通用环形队列：
```c
typedef struct
{
    uint8  id;          /* FIFO ID: 'R'/'T'/'r'/'t' */
    uint16 head;        /* 写指针 */
    uint16 tail;        /* 读指针 */
    uint16 size;        /* 队列总容量 */
    uint8  data[];      /* 数据区 */
} tFifoInfo;
```

---

## 7. 诊断 CAN ID 与寻址方式

### 7.1 诊断 CAN ID 配置

本项目使用 **标准 CAN 帧（11-bit ID）**，定义在 `CANRxTxInterface.h`：

```c
typedef enum
{
    /* 接收 */
    UDS_FUN_ADDR_ID = 0x7DFu,   /* 功能寻址（广播）：所有 ECU 都接收 */
    UDS_PHY_ADDR_ID = 0x74Cu,   /* 物理寻址（点对点）：仅目标 ECU 接收 */
    /* 发送 */
    UDS_RESP_ADDR_ID = 0x75Cu,  /* ECU 响应 ID */
} UdsCanMsgId;
```

**MultiCAN 消息对象配置**：

| MsgObj | CAN ID | 方向 | 用途 |
|--------|--------|------|------|
| 20 | `0x7DF` | RX | 功能寻址接收（广播） |
| 21 | `0x74C` | RX | 物理寻址接收（点对点） |
| 10 | `0x75C` | TX | ECU 响应发送 |

### 7.2 物理寻址 vs 功能寻址

| 特性 | 物理寻址（Physical） | 功能寻址（Functional） |
|------|----------------------|------------------------|
| CAN ID | `0x74C` | `0x7DF` |
| 目标 | 单一 ECU | 总线上所有 ECU |
| 用途 | 正常诊断通信（刷写、读故障码） | 唤醒/广播查询（如 `10 03` 唤醒整车网络） |
| 响应要求 | 必须回复 | 可不回复 |
| UDS 服务限制 | 所有服务 | 通常仅支持 `0x10`、`0x3E`、`0x28` 等 |

**代码中的 ID 校验**：
```c
uint8 CANTP_IsReceivedMsgIDValid(const uint16 i_receiveMsgID)
{
    if ((i_receiveMsgID == CANTP_GetConfigRxMsgFUNID()) || 
        (i_receiveMsgID == CANTP_GetConfigRxMsgPHYID()))
    {
        return TRUE;  /* 是有效的诊断帧 */
    }
    return FALSE;
}
```

---

## 8. 网络管理（Network Management）

### 8.1 车载网络拓扑

```
                    ┌─────────────┐
                    │   诊断仪     │
                    │  (CANoe/    │
                    │   ZCANPRO)  │
                    └──────┬──────┘
                           │ CAN 总线
        ┌──────────────────┼──────────────────┐
        │                  │                  │
   ┌────┴────┐      ┌─────┴─────┐      ┌────┴────┐
   │  ECM    │      │   TCU     │      │  ABS    │
   │ (发动机) │      │ (变速箱)   │      │ (制动)  │
   │0x74C/0x75C│    │0x760/0x770│      │0x780/0x790│
   └─────────┘      └───────────┘      └─────────┘
        │                  │                  │
   ┌────┴────┐      ┌─────┴─────┐      ┌────┴────┐
   │  IEBS   │      │   BCM     │      │  EPS    │
   │(电控刹车)│      │ (车身控制) │      │ (转向)  │
   └─────────┘      └───────────┘      └─────────┘
```

每个 ECU 有**一对诊断 ID**（RX 物理寻址 + TX 响应）。功能寻址 `0x7DF` 是公共广播 ID。

### 8.2 UDS 会话超时（S3 Server）

虽然不是严格意义上的"网络管理"，但 S3 超时是保持诊断会话的核心机制：

```c
/* uds_app.c */
#define S3_SERVER_TIMEOUT 5000u  /* S3 超时 = 5000 ms = 5 秒 */

/* 每次收到任何 UDS 请求，重置 S3 计时器 */
void RestartS3Server(void)
{
    gs_stUdsInfo.xUdsS3ServerTime = S3_SERVER_TIMEOUT;
}

/* 1ms 递减 */
void UDS_SystemTickCtl(void)
{
    if (gs_stUdsInfo.xUdsS3ServerTime > 0)
    {
        gs_stUdsInfo.xUdsS3ServerTime--;
    }
}

/* S3 超时 → 强制回到 Default Session */
if (0u == gs_stUdsInfo.xUdsS3ServerTime)
{
    SetCurrentSession(DEFALUT_SESSION);
    SetSecurityLevel(NONE_SECURITY);
}
```

**规则**：
- 在 **Default Session** 中：不需要 `0x3E TesterPresent`，不会超时
- 在 **Extended/Programming Session** 中：若 5 秒内未收到任何 UDS 请求，自动回 Default Session + 掉安全等级
- 诊断仪应每 **2 秒**发送一次 `0x3E 80`（或 `0x3E 00`）保持会话

### 8.3 通信控制（0x28）

UDS `0x28 CommunicationControl` 用于控制 ECU 的 CAN 通信：

```c
/* Can_session.c */
void CAN_SSN_ServiceID28(diag_can_stt *rxdata, diag_can_stt *txdata)
{
    uint8 subFunc = rxdata->data[1];  /* 0x03 = 控制特定通信类型 */
    uint8 comType = rxdata->data[2];  /* 通信类型 */
    
    switch(comType)
    {
        case 0x01:  /* Enable RX and TX */
            CAN2_SendAllow = 1;
            break;
        case 0x02:  /* Enable RX, Disable TX */
            CAN2_SendAllow = 0;  /* 静默模式：只收不发 */
            break;
        case 0x03:  /* Disable RX, Enable TX */
            break;
    }
}
```

**应用场景**：
- 刷写前：`28 03 03` 关闭所有应用报文发送，只保留诊断通信
- 刷写后：`28 00 00` 恢复所有通信

### 8.4 TLE9252 CAN 收发器模式

本项目使用 Infineon **TLE9252** CAN 收发器，支持多种工作模式：

```c
typedef enum
{
    Normal,         /* 正常收发 */
    ReceiveOnly,    /* 只收不发 */
    GoToSleep,      /* 进入休眠过渡 */
    Sleep,          /* 休眠（低功耗） */
    StandBy,        /* 待机 */
    PowerReset      /* 复位 */
} TLE925x_CAN_MODE;
```

**唤醒机制**（网络管理相关）：
- **本地唤醒**：ECU 上电/复位后自动进入 Normal 模式
- **远程唤醒**：Sleep 模式下，CAN 总线出现显性电平（如功能寻址 `0x7DF` 的帧）→ 唤醒 ECU
- **休眠条件**：所有应用任务空闲 + S3 超时 + 无总线活动 → 可进入 Sleep 降功耗

> **注意**：本项目的 Bootloader 通常不做复杂的 OSEK NM / AUTOSAR NM，只在 APP 中实现完整的网络管理。Bootloader 仅处理诊断通信的使能/禁用。

---

## 9. 完整数据流（从 CAN 到 UDS 再回 CAN）

### 9.1 接收链路（诊断仪 → ECU）

```
步骤 1：CAN 硬件中断
├─ TLE9252 收到 CAN 帧 → MultiCAN 模块存入 Message RAM
├─ CAN 中断触发 → 驱动层读取帧
│
步骤 2：CAN 驱动层
├─ CANRxTxInterface.c: Can9252RxLookup()
├─ 将原始 CAN 帧放入 gl_rxDataQueue
│
步骤 3：主循环处理
├─ AppBL_main() → CanMainProcess()
├─ 从 gl_rxDataQueue 取出帧
├─ 判断是 UDS 帧（0x7DF / 0x74C）→ 调用 UdsRxCANMsgMainFun()
│
步骤 4：CANTP 写入
├─ uds_tp.c: CANTP_DriverWriteDataInCANTP()
├─ 将 8 字节原始帧写入 RX_BUS_FIFO ('r')
│
步骤 5：TP 主循环
├─ UdsMainProcess() → CANTP_MainFun()
├─ 从 RX_BUS_FIFO 读取帧
├─ 判断 PCI 类型（SF/FF/CF/FC）→ 运行状态机
├─ 重组完成后 → 写入 RX_TP_QUEUE ('R')
│
步骤 6：UDS 层处理
├─ UDS_MainFun() → TP_ReadAFrameDataFromTP()
├─ 从 RX_TP_QUEUE 读取完整 UDS 请求
├─ 根据 SID 分发到对应服务函数（如 RequestDownload0x34）
└─ 构造响应数据 → 写入 TX_TP_QUEUE ('T')
```

### 9.2 发送链路（ECU → 诊断仪）

```
步骤 1：UDS 层构造响应
├─ uds_app.c: RequestDownload0x34() 正响应
├─ m_pstPDUMsg->aDataBuf[0] = 0x74  /* SID + 0x40 */
├─ 调用 TP_WriteAFrameDataInTP() 写入 TX_TP_QUEUE ('T')
│
步骤 2：TP 主循环分段
├─ CANTP_MainFun() 从 TX_TP_QUEUE 读取完整响应
├─ 若长度 ≤ 7 → 直接发 SF
├─ 若长度 > 7 → 发 FF → 等 FC → 发 CF（按 BS/STmin 节奏）
├─ 每帧 8 字节写入 TX_BUS_FIFO ('t')
│
步骤 3：CAN 驱动发送
├─ SendMsgMainFun() 从 TX_BUS_FIFO 读取帧
├─ DrvCanSendMessage() → MultiCAN 发送
├─ TLE9252 将帧驱动到 CAN 总线
└─ 诊断仪收到完整响应
```

---

## 10. 调试技巧与常见问题

### 10.1 用 CANoe/ZCANPRO 抓包分析 TP 层

在 CANoe 中加载诊断描述文件（CDD/ODX）后，Trace 窗口会**自动解析 TP 层**：

```
Raw CAN Frame:        7DF  [8]  02 10 03 AA AA AA AA AA
CANoe 解析后显示:      SF  PCI=0x02  Len=2  Data=10 03
                      └── 单帧，携带 2 字节数据（进入扩展会话）

Raw CAN Frame:        74C  [8]  10 20 34 00 44 A0 02 00
CANoe 解析后显示:      FF  PCI=0x10 0x20  Len=32  Data=34 00 44 A0 02 00...
                      └── 首帧，总长度 32 字节

Raw CAN Frame:        75C  [8]  30 00 05 AA AA AA AA AA
CANoe 解析后显示:      FC  FS=CTS  BS=0  STmin=5ms
                      └── 流控帧，允许无限发送，间隔 5ms

Raw CAN Frame:        74C  [8]  21 00 00 06 00 00 36 01
CANoe 解析后显示:      CF  SN=1  Data=00 00 06 00 00 36 01
                      └── 连续帧 #1
```

### 10.2 常见问题排查

| 现象 | 可能原因 | 排查方法 |
|------|----------|----------|
| 诊断仪报 "等待 FC 超时" | ECU 的 N_Br 太长或没发 FC | 抓包看 ECU 是否在收到 FF 后回复了 `0x30` |
| 诊断仪报 "CF 序列号错误" | ECU 收到的 SN 不连续 | 检查 CAN 总线是否丢帧（Bus Load 过高） |
| 诊断仪报 "N_Cr 超时" | ECU 发送 CF 太慢或中断 | 检查 STmin 设置，确认 TP 主循环 1ms 是否被阻塞 |
| UDS 响应发不出去 | TX BUS FIFO 满 | 检查 SendMsgMainFun() 是否正常执行 |
| 刷写时 0x36 总是失败 | 上一帧没发完或序列号错 | 用逻辑分析仪抓 CAN 波形，检查 `0x36` 的 SN 是否连续 |
| S3 超时掉会话 | 诊断仪没发 0x3E 或间隔太长 | 确认诊断仪是否配置了 TesterPresent 周期 < 5s |

### 10.3 关键调试断点

```c
/* uds_tp.c */
CANTP_DoReceiveSF()     /* 收到单帧时断这里 */
CANTP_DoReceiveFF()     /* 收到首帧时断这里 */
CANTP_DoReceiveCF()     /* 收到连续帧时断这里 */
CANTP_DoTransmitFC()    /* 发送流控帧时断这里 */
CANTP_DoTransmitSF()    /* 发送单帧时断这里 */
CANTP_DoTransmitCF()    /* 发送连续帧时断这里 */

/* uds_app.c */
RequestDownload0x34()   /* 收到 0x34 时断这里 */
TransferData0x36()      /* 收到 0x36 时断这里 */
RoutineControl0x31()    /* 收到 0x31 时断这里 */
```

---

## 11. 总结

| 模块 | 核心职责 | 本项目文件 |
|------|----------|-----------|
| **CAN 硬件** | 收发 CAN 帧（8 字节） | MultiCAN 驱动 + TLE9252 |
| **CAN TP** | 分段/重组长数据（ISO 15765-2） | `uds_tp.c` / `uds_tp.h` |
| **FIFO** | 缓冲收发数据 | `uds_fifo.c` |
| **CAN 接口** | 连接硬件与 TP | `CANRxTxInterface.c` |
| **UDS 应用** | 诊断服务实现（ISO 14229） | `uds_app.c` |
| **网络管理** | 会话超时、通信控制 | `uds_timer.c` / `Can_session.c` |

**记忆口诀**：
- **SF** ≤ 7，一帧搞定；**FF** 开头，**CF** 接力，**FC** 控速
- **BS** = 0 不限速，**STmin** 防总线爆满
- **N_Cr** 等 CF，**N_Bs** 等 FC，谁超时谁报错
- **0x7DF** 广播，**0x74C/0x75C** 点对点，**S3** = 5 秒保活
