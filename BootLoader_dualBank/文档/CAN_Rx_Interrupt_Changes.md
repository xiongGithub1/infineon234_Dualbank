# BootLoader CAN 接收方式修改说明

> 修改日期: 2026-05-08  
> 修改内容: 将 CAN 接收从**轮询(Polling)**方式改为**中断(Interrupt)**方式  
> 涉及节点: CAN0 (TLE9252) 和 CAN1 (TLE9251)

---

## 修改文件汇总

| 序号 | 文件路径 | 修改内容 |
|:---:|---|---|
| 1 | `AppSw/Tricore/Tc234_Modules/CAN/MultiCAN.c` | 启用 RX 中断优先级配置，配置消息对象接收中断 |
| 2 | `AppSw/Tricore/Tc234_Modules/CAN/Can.c` | CAN1 轮询接收函数改为中断服务函数 `isrCAN1_RX` |
| 3 | `AppSw/Tricore/Communication/CANRxTxInterface.c` | CAN0 轮询接收函数改为中断服务函数 `isrCAN0_RX` |
| 4 | `AppSw/Tricore/App_bootloader/App_bootloader.c` | 主循环中移除轮询函数调用 |

---

## 1. MultiCAN.c 修改

### 1.1 启用 CAN0 / CAN1 接收中断优先级

**修改前:**
```c
//  canConfig.nodePointer[IfxMultican_SrcId_1].priority = ISR_PRIORITY_CAN0_RX;
//  canConfig.nodePointer[IfxMultican_SrcId_1].typeOfService = IfxSrc_Tos_cpu0;
    canConfig.nodePointer[IfxMultican_SrcId_2].priority = ISR_PRIORITY_CAN0_ER;
    canConfig.nodePointer[IfxMultican_SrcId_2].typeOfService = IfxSrc_Tos_cpu0;
    // CAN1 uses polling, no RX interrupt priority configured
//  canConfig.nodePointer[IfxMultican_SrcId_3].priority = ISR_PRIORITY_CAN1_RX;
//  canConfig.nodePointer[IfxMultican_SrcId_3].typeOfService = IfxSrc_Tos_cpu0;
```

**修改后:**
```c
    canConfig.nodePointer[IfxMultican_SrcId_1].priority = ISR_PRIORITY_CAN0_RX;
    canConfig.nodePointer[IfxMultican_SrcId_1].typeOfService = IfxSrc_Tos_cpu0;
    canConfig.nodePointer[IfxMultican_SrcId_2].priority = ISR_PRIORITY_CAN0_ER;
    canConfig.nodePointer[IfxMultican_SrcId_2].typeOfService = IfxSrc_Tos_cpu0;
    // CAN1 RX interrupt enabled
    canConfig.nodePointer[IfxMultican_SrcId_3].priority = ISR_PRIORITY_CAN1_RX;
    canConfig.nodePointer[IfxMultican_SrcId_3].typeOfService = IfxSrc_Tos_cpu0;
```

### 1.2 CAN1 (Node1/9251) 接收消息对象配置中断

在 `CAN_TLE9251_config_node1()` 中，接收消息对象初始化部分添加：

**修改前:**
```c
        canMsgObjConfig.control.fastBitRate   = FALSE;
        /* initialize message object */
        IfxMultican_Can_MsgObj_init(&g_MulticanBasic.drivers.canNode1MsgObjRx, &canMsgObjConfig);
```

**修改后:**
```c
        canMsgObjConfig.control.fastBitRate   = FALSE;
        canMsgObjConfig.rxInterrupt.enabled   = TRUE;
        canMsgObjConfig.rxInterrupt.srcId     = IfxMultican_SrcId_3;
        /* initialize message object */
        IfxMultican_Can_MsgObj_init(&g_MulticanBasic.drivers.canNode1MsgObjRx, &canMsgObjConfig);
```

### 1.3 CAN0 (Node0/9252) 接收消息对象配置中断

在 `CAN_TLE9252_config_node0()` 中，接收消息对象初始化循环内添加：

**修改前:**
```c
			canMsgObjConfig.control.fastBitRate   = FALSE;

			/* initialize message object */
			IfxMultican_Can_MsgObj_init(&g_MulticanBasic.drivers.canNode0MsgRx2[j], &canMsgObjConfig);
```

**修改后:**
```c
			canMsgObjConfig.control.fastBitRate   = FALSE;
			canMsgObjConfig.rxInterrupt.enabled     = TRUE;
			canMsgObjConfig.rxInterrupt.srcId       = IfxMultican_SrcId_1;

			/* initialize message object */
			IfxMultican_Can_MsgObj_init(&g_MulticanBasic.drivers.canNode0MsgRx2[j], &canMsgObjConfig);
```

---

## 2. Can.c 修改

### 2.1 添加头文件

**修改前:**
```c
#include    "Can.h"
#include    "App_bootloader_cfg.h"
#include    "Can_session.h"
#include    "CanIf.h"
#include 	"MultiCAN.h"
#include    "IfxMultican.h"
```

**修改后:**
```c
#include    "Can.h"
#include    "App_bootloader_cfg.h"
#include    "Can_session.h"
#include    "CanIf.h"
#include 	"MultiCAN.h"
#include    "IfxMultican.h"
#include    "ConfigurationIsr.h"
```

### 2.2 CAN1 接收改为中断服务函数

**修改前:**
```c
void Can_RxIndicationMainFunc(void)	//CAN2 9251 polling receive
{
	uint8 receiveFrameState;

    if(TRUE == IfxMultican_Can_MsgObj_isRxPending(&g_MulticanBasic.drivers.canNode1MsgObjRx) )
    {
        IfxMultican_Message_init(&rxmsg, 0x0, 0x0, 0x0, IfxMultican_DataLengthCode_8);
        readStatus = IfxMultican_Can_MsgObj_readMessage(&g_MulticanBasic.drivers.canNode1MsgObjRx, &rxmsg);

        if(CAN1_MO_RXPDU_ID == rxmsg.id)
        {
        	receiveFrameState = CanIf_fillBuffer(rxmsg,  &gAppData.rx );
            if(receiveFrameState == 0)
            {
                CAN_SSN_ServiceWithId();
            }
        }
    }

    return;
}
```

**修改后:**
```c
// CAN1(9251) RX interrupt service routine
IFX_INTERRUPT(isrCAN1_RX, 0, ISR_PRIORITY_CAN1_RX);

void isrCAN1_RX(void)
{
	uint8 receiveFrameState;

    if(TRUE == IfxMultican_Can_MsgObj_isRxPending(&g_MulticanBasic.drivers.canNode1MsgObjRx) )
    {
        IfxMultican_Message_init(&rxmsg, 0x0, 0x0, 0x0, IfxMultican_DataLengthCode_8);
        readStatus = IfxMultican_Can_MsgObj_readMessage(&g_MulticanBasic.drivers.canNode1MsgObjRx, &rxmsg);

        if(CAN1_MO_RXPDU_ID == rxmsg.id)
        {
        	receiveFrameState = CanIf_fillBuffer(rxmsg,  &gAppData.rx );
            if(receiveFrameState == 0)
            {
                CAN_SSN_ServiceWithId();
            }
        }
    }
}

// Original polling function, kept for compatibility. Reception is now handled by isrCAN1_RX interrupt.
void Can_RxIndicationMainFunc(void)
{
    // Interrupt mode: isrCAN1_RX handles CAN1 reception
}
```

---

## 3. CANRxTxInterface.c 修改

### 3.1 添加头文件

**修改前:**
```c
#include "MultiCAN.h"
#include "CANRxTxInterface.h"
#include "tool_class.h"
#include "uds_app.h"
```

**修改后:**
```c
#include "MultiCAN.h"
#include "CANRxTxInterface.h"
#include "tool_class.h"
#include "uds_app.h"
#include "ConfigurationIsr.h"
```

### 3.2 CAN0 接收改为中断服务函数

**修改前:**
```c
void Can9252RxLookup(void)
{
    uint32 status = 0;
	QueueMsgObject qmsg;

	for(uint8 j=0; j<CanRxFrm9252Num;j++)
	{
		if(TRUE == IfxMultican_Can_MsgObj_isRxPending(&g_MulticanBasic.drivers.canNode0MsgRx2[j]))
		{
			status = IfxMultican_Status_ok;
			status = IfxMultican_Can_MsgObj_readMessage(&g_MulticanBasic.drivers.canNode0MsgRx2[j],&Node0Readmsg);
			if(status != IfxMultican_Status_receiveEmpty)
			{
				if((Node0Readmsg.id==UDS_FUN_ADDR_ID)||(Node0Readmsg.id==UDS_PHY_ADDR_ID))
				{
					qmsg.id = (uint16)Node0Readmsg.id;
					tl_memcpy(qmsg.data,Node0Readmsg.data,8);
					tl_queue_add_item(&gl_rxDataQueue,&qmsg);
				}
			}
		}
	}
}
```

**修改后:**
```c
IFX_INTERRUPT(isrCAN0_RX, 0, ISR_PRIORITY_CAN0_RX);

void isrCAN0_RX(void)
{
    uint32 status = 0;
	QueueMsgObject qmsg;

	for(uint8 j=0; j<CanRxFrm9252Num;j++)
	{
		if(TRUE == IfxMultican_Can_MsgObj_isRxPending(&g_MulticanBasic.drivers.canNode0MsgRx2[j]))
		{
			status = IfxMultican_Status_ok;
			status = IfxMultican_Can_MsgObj_readMessage(&g_MulticanBasic.drivers.canNode0MsgRx2[j],&Node0Readmsg);
			if(status != IfxMultican_Status_receiveEmpty)
			{
				if((Node0Readmsg.id==UDS_FUN_ADDR_ID)||(Node0Readmsg.id==UDS_PHY_ADDR_ID))
				{
					qmsg.id = (uint16)Node0Readmsg.id;
					tl_memcpy(qmsg.data,Node0Readmsg.data,8);
					tl_queue_add_item(&gl_rxDataQueue,&qmsg);
				}
			}
		}
	}
}

// Original polling function replaced by interrupt isrCAN0_RX. No need to call in main loop.
void Can9252RxLookup(void)
{
    // Interrupt mode: isrCAN0_RX handles CAN0 reception
}
```

---

## 4. App_bootloader.c 修改

### 4.1 主循环移除轮询调用

**修改前:**
```c
	/* Bootloader main loop */

    Can_RxIndicationMainFunc();

  	Can9252RxLookup();
	UdsMainProcess();
	CanMainProcess();
```

**修改后:**
```c
	/* Bootloader main loop */

    // Can_RxIndicationMainFunc(); // Replaced by interrupt isrCAN1_RX

  	// Can9252RxLookup(); // Replaced by interrupt isrCAN0_RX
	UdsMainProcess();
	CanMainProcess();
```

---

## 中断优先级配置 (ConfigurationIsr.h)

本次修改使用了以下已定义的中断优先级宏，无需修改此文件：

```c
#define ISR_PRIORITY_CAN0_RX					40
#define ISR_PRIORITY_CAN0_ER					41
#define ISR_PRIORITY_CAN1_RX					42
#define ISR_PRIORITY_CAN1_ER					43
```

---

## 注意事项

1. **中断向量表自动注册**: `IFX_INTERRUPT` 宏会自动将中断服务函数注册到 TC234 的中断向量表，无需手动修改启动文件。

2. **空壳函数保留**: `Can_RxIndicationMainFunc()` 和 `Can9252RxLookup()` 被保留为空函数，防止其他模块的外部引用导致链接错误。如需彻底删除，需同步删除头文件中的 `extern` 声明。

3. **RX Pending 自动清除**: `IfxMultican_Can_MsgObj_readMessage()` 内部会自动清除消息对象的 `RXpnd` 标志，中断服务函数中无需手动清除。

4. **编译验证**: 修改后请在 Tasking 编译器中重新编译整个 Bootloader 工程，确认中断服务函数正确链接。
