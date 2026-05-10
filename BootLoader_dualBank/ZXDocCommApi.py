# -*- coding: utf-8 -*-

import ctypes
from ctypes import Structure, Union
from enum import IntEnum, unique
from dataclasses import dataclass, field
import platform
import os
import time

ZXDoc_Char = ctypes.c_char
ZXDoc_UByte = ctypes.c_ubyte
ZXDoc_I8 = ctypes.c_int8
ZXDoc_I16 = ctypes.c_int16
ZXDoc_I32 = ctypes.c_int32
ZXDoc_I64 = ctypes.c_int64
ZXDoc_U8 = ctypes.c_uint8
ZXDoc_U16 = ctypes.c_uint16
ZXDoc_U32 = ctypes.c_uint32
ZXDoc_U64 = ctypes.c_uint64
ZXDoc_Float = ctypes.c_float
ZXDoc_Double = ctypes.c_double

ZXDoc_I8P = ctypes.POINTER(ZXDoc_I8)
ZXDoc_I16P = ctypes.POINTER(ZXDoc_I16)
ZXDoc_I32P = ctypes.POINTER(ZXDoc_I32)
ZXDoc_I64P = ctypes.POINTER(ZXDoc_I64)
ZXDoc_U8P = ctypes.POINTER(ZXDoc_U8)
ZXDoc_U16P = ctypes.POINTER(ZXDoc_U16)
ZXDoc_U32P = ctypes.POINTER(ZXDoc_U32)
ZXDoc_U64P = ctypes.POINTER(ZXDoc_U64)
ZXDoc_FloatP = ctypes.POINTER(ZXDoc_Float)
ZXDoc_DoubleP = ctypes.POINTER(ZXDoc_Double)
ZXDoc_UByteP = ctypes.POINTER(ZXDoc_UByte)
ZXDoc_VoidP = ctypes.c_void_p
ZXDoc_CharP = ctypes.c_char_p
ZXDoc_CharPP = ctypes.POINTER(ZXDoc_CharP)

ZXDocHandle = ZXDoc_U64
ZXDOC_INVALID_HANDLE = ZXDoc_U64(-1)

ZXDoc_Bool = ZXDoc_U8
ZXDoc_BoolP = ctypes.POINTER(ZXDoc_Bool)
ZXDoc_True = 1
ZXDoc_False = 0


ZXDocErrorCode = ZXDoc_U32
ZXDOC_E_OK = 0
ZXDOC_E_FAILED = 1
ZXDOC_E_INVALID_HANDLE = 2
ZXDOC_E_NULL_POINTER = 3
ZXDOC_E_INVALID_DATA_TYPE = 4
ZXDOC_E_INVALID_PARAM = 5
ZXDOC_E_BUFFER_IS_TO_SMALL = 6

ZXDoc_LogLevel = ZXDoc_U32

ZXDOC_LOG_LVL_DBG = 0
ZXDOC_LOG_LVL_INFO = 1
ZXDOC_LOG_LVL_WARNING = 2
ZXDOC_LOG_LVL_ERR = 3
ZXDOC_LOG_LVL_CRITICAL = 4

# 数据传输方向
ZXDoc_TransmitDirection = ZXDoc_U8
ZXDOC_RX = 0
ZXDOC_TX = 1

# 信号值类型
ZXDOC_SGL_VALUE_TYPE_INVALID = 0
ZXDOC_SGL_VALUE_TYPE_U8 = 1
ZXDOC_SGL_VALUE_TYPE_U16 = 2
ZXDOC_SGL_VALUE_TYPE_U32 = 3
ZXDOC_SGL_VALUE_TYPE_U64 = 4
ZXDOC_SGL_VALUE_TYPE_I8 = 5
ZXDOC_SGL_VALUE_TYPE_I16 = 6
ZXDOC_SGL_VALUE_TYPE_I32 = 7
ZXDOC_SGL_VALUE_TYPE_I64 = 8
ZXDOC_SGL_VALUE_TYPE_FLOAT = 9
ZXDOC_SGL_VALUE_TYPE_DOUBLE = 10
ZXDOC_SGL_VALUE_TYPE_STR = 11
ZXDOC_SGL_VALUE_TYPE_ARRAY = 12

ZXDoc_BusType = ZXDoc_I32

ZXDOC_BUSTYPE_UNKNOWN = 0
ZXDOC_BUSTYPE_CAN = 1
ZXDOC_BUSTYPE_LIN = 2
ZXDOC_BUSTYPE_ETHERNET = 3

ZXDoc_RawDataType = ZXDoc_I32
ZXDoc_RawDataType_Unknown = 0
ZXDoc_RawDataType_CANFD = 1  # ZXDoc_CANFDData
ZXDoc_RawDataType_CANFDError = 2  # ZXDoc_CANFDErrorData
ZXDoc_RawDataType_Event = 3  # Not implemented
ZXDoc_RawDataType_GPS = 4  # ZXDoc_GPSData
ZXDoc_RawDataType_LIN = 5  # ZXDoc_LINData
ZXDoc_RawDataType_LINError = 6  # ZXDoc_LINErrorData
ZXDoc_RawDataType_BusUsage = 7  # ZXDoc_BusUsageData
ZXDoc_RawDataType_LINEvent = 8  # Not implemented
ZXDoc_RawDataType_Ethernet = 9  # ZXDoc_EthernetData

ZXDoc_MeasEventType = ZXDoc_I32

ZXDOC_MEAS_EVENT_POLLING = 1  # 轮询获取
ZXDOC_MEAS_EVENT_DAQ_EVENT = 2  # ECU按指定DAQ事件上送
ZXDOC_MEAS_EVENT_DAQ_CYCLIC = (
    3  # ECU按DAQ事件的整数倍周期上送, 周期小于1ms的事件不支持Cyclic模式
)
ZXDOC_MEAS_EVENT_ON_INPUT = 4  # 依赖输入信号，自己不轮询

ZXDoc_E2ECrcType = ZXDoc_I32

ZXDOC_E2E_CRC8 = 0
ZXDOC_E2E_CRC8_SAE = 1
ZXDOC_E2E_CRC8_H2F = 2
ZXDOC_E2E_CRC16 = 3
ZXDOC_E2E_CRC16_CCITT = 4
ZXDOC_E2E_CRC16_CCITT_FALSE = 5
ZXDOC_E2E_CRC32 = 6
ZXDOC_E2E_CRC32_P4 = 7
ZXDOC_E2E_CRC64_ECMA = 8
ZXDOC_E2E_CRC_CUSTOM = 9

ZXDoc_SimuSendType = ZXDoc_I32
ZXDOC_CYCLE = 0
ZXDOC_ON_CHANGED = 1
ZXDOC_ON_WRITTEN = 2

ZXDoc_DeviceType = ZXDoc_I32
ZXDoc_CcpDevice = 0  # CCP协议的ECU
ZXDoc_XcpDevice = 1  # XCP协议的ECU
ZXDoc_DiagnosticDevice = 2  # 诊断
ZXDoc_CanfdDevice = 3  # CAN或CANFD总线监测
ZXDoc_LinDevice = 4  # LIN总线监测
ZXDoc_OBDDiagnosticDevice = 5  # OBD诊断
ZXDoc_AcquistionDevice = 6  # 传感器设备采集
ZXDoc_UnknownDevice = 0xFF

ZXDoc_FilterMode = ZXDoc_I32
ZXDoc_FilterMode_Accept = 0
ZXDoc_FilterMode_Reject = 1
ZXDoc_FilterMode_NoFilter = 3

ZXDoc_FilterDirection = ZXDoc_I32
ZXDoc_FilterDirection_Rx = 0x01
ZXDoc_FilterDirection_Tx = 0x02
ZXDoc_FilterDirection_All = 0xFF

ZXDoc_CANFilterFrameType = ZXDoc_I32
ZXDoc_CANFilterFrameType_Std = 0x01
ZXDoc_CANFilterFrameType_Ext = 0x02
ZXDoc_CANFilterFrameType_All = 0xFF

ZXDoc_DataSinkFilterType = ZXDoc_I32
ZXDoc_CANErrorFilterType = 0
ZXDoc_CANIDRangeFilterType = 1
ZXDoc_LINErrorFilterType = 2
ZXDoc_LINWakeUpEventFilterType = 3
ZXDoc_LINIDRangeFilterType = 4

current_file_path = os.path.abspath(__file__)
current_dir = os.path.dirname(current_file_path)
arch = platform.architecture()[0]
if "64" in arch:
    _dll = ctypes.WinDLL(current_dir + "/../lib/x64/ZXDocComm.dll")
else:
    _dll = ctypes.WinDLL(current_dir + "/../lib/x86/ZXDocComm.dll")


class ZXDoc_CANErrorFilter(Structure):
    _pack_ = 1
    _fields_ = [("channelIndex", ZXDoc_I32)]


PZXDoc_CANErrorFilter = ctypes.POINTER(ZXDoc_CANErrorFilter)


class ZXDoc_CANIDRangeFilter(Structure):
    _pack_ = 1
    _fields_ = [
        ("channelIndex", ZXDoc_I32),
        ("direction", ZXDoc_FilterDirection),
        ("frameType", ZXDoc_CANFilterFrameType),
        ("idMin", ZXDoc_U32),
        ("idMax", ZXDoc_U32),
    ]


PZXDoc_CANIDRangeFilter = ctypes.POINTER(ZXDoc_CANIDRangeFilter)


class ZXDoc_LINErrorFilter(Structure):
    _pack_ = 1
    _fields_ = [("channelIndex", ZXDoc_I32)]


PZXDoc_LINErrorFilter = ctypes.POINTER(ZXDoc_LINErrorFilter)


class ZXDoc_LINWakeUpEventFilter(Structure):
    _pack_ = 1
    _fields_ = [("channelIndex", ZXDoc_I32)]


PZXDoc_LINWakeUpEventFilter = ctypes.POINTER(ZXDoc_LINWakeUpEventFilter)


class ZXDoc_LINIDRangeFilter(Structure):
    _pack_ = 1
    _fields_ = [
        ("channelIndex", ZXDoc_I32),
        ("direction", ZXDoc_FilterDirection),
        ("idMin", ZXDoc_U32),
        ("idMax", ZXDoc_U32),
    ]


PZXDoc_LINIDRangeFilter = ctypes.POINTER(ZXDoc_LINIDRangeFilter)


class ZXDoc_DataSinkFilterRule(Union):
    _pack_ = 1
    _fields_ = [
        ("canErrorFilter", ZXDoc_CANErrorFilter),
        ("canIDRangeFilter", ZXDoc_CANIDRangeFilter),
        ("linErrorFilter", ZXDoc_LINErrorFilter),
        ("linWakeUpEventFilter", ZXDoc_LINWakeUpEventFilter),
        ("linIDRangeFilter", ZXDoc_LINIDRangeFilter),
    ]


class ZXDoc_DataSinkFilter(Structure):
    _pack_ = 1
    _fields_ = [("type", ZXDoc_DataSinkFilterType), ("rule", ZXDoc_DataSinkFilterRule)]


PZXDoc_DataSinkFilter = ctypes.POINTER(ZXDoc_DataSinkFilter)


class ZXDoc_CANFDData(Structure):
    """
    CAN FD 数据帧
    """

    _pack_ = 1
    _fields_ = [
        ("can_id", ZXDoc_U32),
        ("flags", ZXDoc_U32),
        ("data", ZXDoc_UByte * 64),
        ("len", ZXDoc_U8),
        ("DLC", ZXDoc_U8),
        ("res0", ZXDoc_U8),
        ("res1", ZXDoc_U8),
    ]

    def EFF(self, value: bool = None) -> bool:
        if value is not None:
            if value:
                self.flags = self.flags | 0x01
            else:
                self.flags = self.flags & ~0x01
        return bool(self.flags & 0x01)

    def FDF(self, value: bool = None) -> bool:
        if value is not None:
            if value:
                self.flags = self.flags | 0x02
            else:
                self.flags = self.flags & ~0x02
        return bool((self.flags & 0x02) >> 1)

    def RTR(self, value: bool = None) -> bool:
        if value is not None:
            if value:
                self.flags = self.flags | 0x04
            else:
                self.flags = self.flags & ~0x04
        return bool((self.flags & 0x04) >> 2)

    def BRS(self, value: bool = None) -> bool:
        if value is not None:
            if value:
                self.flags = self.flags | 0x08
            else:
                self.flags = self.flags & ~0x08
        return bool((self.flags & 0x08) >> 3)

    def ESI(self, value: bool = None) -> bool:
        if value is not None:
            if value:
                self.flags = self.flags | 0x10
            else:
                self.flags = self.flags & ~0x10
        return bool((self.flags & 0x10) >> 4)

    def direction(self, value: int = None) -> ZXDoc_TransmitDirection:
        if value is not None:
            if 0 == value:
                self.flags = self.flags & ~0x20
            else:
                self.flags = self.flags | 0x20

        return (self.flags & 0x20) >> 5

    def txDelayUnitType(self, value: int = None) -> int:
        if value is not None:
            self.flags = (self.flags & ~0xC0) | ((value & 0x03) << 6)

        return (self.flags & 0xC0) >> 6

    def transmitType(self, value: int = None) -> int:
        if value is not None:
            self.flags = (self.flags & ~0x0F00) | ((value & 0x0F) << 8)

        return (self.flags & 0x0F00) >> 8

    def txDelayTime(self, value: int = None) -> int:
        if value is not None:
            if value >= 0:
                tx_delay = value if value < 65535 else 65535
                self.res0 = tx_delay & 0x000000FF
                self.res1 = (tx_delay & 0x0000FF00) >> 8

        return self.res0 | (self.res1 << 8)


PZXDoc_CANFDData = ctypes.POINTER(ZXDoc_CANFDData)


class ZXDoc_CANFDErrorData(Structure):
    _pack_ = 1
    _fields_ = [
        ("errType", ZXDoc_U8),
        ("errSubType", ZXDoc_U8),
        ("nodeState", ZXDoc_U8),
        ("rxErrCount", ZXDoc_U8),
        ("txErrCount", ZXDoc_U8),
        ("errData", ZXDoc_U8),
        ("reserved", ZXDoc_U8 * 2),
    ]


PZXDoc_CANFDErrorData = ctypes.POINTER(ZXDoc_CANFDErrorData)


class ZXDoc_GPSTime(Structure):
    _pack_ = 1
    _fields_ = [
        ("year", ZXDoc_U16),
        ("mon", ZXDoc_U16),
        ("day", ZXDoc_U16),
        ("hour", ZXDoc_U16),
        ("min", ZXDoc_U16),
        ("sec", ZXDoc_U16),
        ("milsec", ZXDoc_U16),
    ]


class ZXDoc_GPSData(Structure):
    _pack_ = 1
    _fields_ = [
        ("time", ZXDoc_GPSTime),
        ("flags", ZXDoc_U16),
        ("latitude", ZXDoc_Double),
        ("longitude", ZXDoc_Double),
        ("altitude", ZXDoc_Double),
        ("speed", ZXDoc_Double),
        ("courseAngle", ZXDoc_Double),
    ]

    def timeValid(self) -> bool:
        return (self.flags & 0x01) > 0

    def latlongValid(self) -> bool:
        return (self.flags & 0x02) > 0

    def altitudeValid(self) -> bool:
        return (self.flags & 0x04) > 0

    def speedValid(self) -> bool:
        return (self.flags & 0x08) > 0

    def courseAngleValid(self) -> bool:
        return (self.flags & 0x10) > 0


PZXDoc_GPSData = ctypes.POINTER(ZXDoc_GPSData)

ZXDoc_LINFrameType = ZXDoc_I32
ZXDoc_LINFrameType_HeaderAndResponse = 0
ZXDoc_LINFrameType_HeaderOnly = 1
ZXDoc_LINFrameType_ResponseOnly = 2


class ZXDoc_LINData(Structure):
    _pack_ = 1
    _fields_ = [
        ("ID", ZXDoc_U8),
        ("timestamp", ZXDoc_U64),
        ("dataLen", ZXDoc_U8),
        ("direction", ZXDoc_U8),
        ("chksum", ZXDoc_U8),
        ("reserved", ZXDoc_UByte * 4),
        ("data", ZXDoc_UByte * 64),
        ("frameType", ZXDoc_LINFrameType),
    ]


PZXDoc_LINData = ctypes.POINTER(ZXDoc_LINData)


class ZXDoc_LINErrorData(Structure):
    _pack_ = 1
    _fields_ = [
        ("ID", ZXDoc_U8),
        ("dataLen", ZXDoc_U8),
        ("direction", ZXDoc_U8),
        ("errorState", ZXDoc_U8),
        ("errorReason", ZXDoc_U8),
        ("chksum", ZXDoc_U8),
        ("reserved", ZXDoc_UByte * 2),
        ("data", ZXDoc_UByte * 8),
    ]


PZXDoc_LINErrorData = ctypes.POINTER(ZXDoc_LINErrorData)


ZXDoc_LINEventType = ZXDoc_I32
ZXDoc_LINEventType_Unknown = 0
ZXDoc_LINEventType_WakeUp = 1


class ZXDoc_LINEventData(Structure):
    _pack_ = 1
    _fields_ = [("type", ZXDoc_LINEventType), ("reserved", ZXDoc_UByte * 7)]


PZXDoc_LINEventData = ctypes.POINTER(ZXDoc_LINEventData)


class ZXDoc_BusUsageData(Structure):
    _pack_ = 1
    _fields_ = [
        ("duration", ZXDoc_U64),
        ("busUsage", ZXDoc_U16),
        ("frameCount", ZXDoc_U32),
    ]


PZXDoc_BusUsageData = ctypes.POINTER(ZXDoc_BusUsageData)

ZXDoc_EthernetType = ZXDoc_U16
ZXDoc_EthernetType_IP = 0x0800
ZXDoc_EthernetType_ARP = 0x0806
ZXDoc_EthernetType_ETHBRIDGE = 0x6558
ZXDoc_EthernetType_REVARP = 0x8035
ZXDoc_EthernetType_AT = 0x809B
ZXDoc_EthernetType_AARP = 0x80F3
ZXDoc_EthernetType_VLAN = 0x8100
ZXDoc_EthernetType_IPX = 0x8137
ZXDoc_EthernetType_IPV6 = 0x86DD
ZXDoc_EthernetType_LOOPBACK = 0x9000
ZXDoc_EthernetType_PPPOED = 0x8863
ZXDoc_EthernetType_PPPOES = 0x8864
ZXDoc_EthernetType_MPLS = 0x8847
ZXDoc_EthernetType_PPP = 0x880B
ZXDoc_EthernetType_ROCEV1 = 0x8915
ZXDoc_EthernetType_IEEE_802_1AD = 0x88A8
ZXDoc_EthernetType_WAKE_ON_LAN = 0x0842

ZXDoc_EthernetFamily = ZXDoc_I32
ZXDoc_EthernetFamily_INET = 2
ZXDoc_EthernetFamily_NS = 6
ZXDoc_EthernetFamily_ISO = 7
ZXDoc_EthernetFamily_APPLETALK = 16
ZXDoc_EthernetFamily_IPX = 23
ZXDoc_EthernetFamily_INET6_BSD = 24
ZXDoc_EthernetFamily_INET6_FREEBSD = 28
ZXDoc_EthernetFamily_INET6_DARWIN = 30

ZXDoc_EthernetDataType = ZXDoc_I32
ZXDoc_EthernetDataType_ETHERNET = 1
ZXDoc_EthernetDataType_ETHERNET_DOT_3 = 2
ZXDoc_EthernetDataType_LOOPBACK = 3


class ZXDoc_EthernetDataHeader(Union):
    class Ethernet(Structure):
        _pack_ = 1
        _fields_ = [
            ("dstMac", ZXDoc_U8 * 6),
            ("srcMac", ZXDoc_U8 * 6),
            ("etherType", ZXDoc_EthernetType),
        ]

    class EthernetDot3(Structure):
        _pack_ = 1
        _fields_ = [
            ("dstMac", ZXDoc_U8 * 6),
            ("srcMac", ZXDoc_U8 * 6),
            ("length", ZXDoc_U16),
        ]

    class Loopback(Structure):
        _pack_ = 1
        _fields_ = [
            ("family", ZXDoc_U32),
        ]

    _pack_ = 1
    _fields_ = [
        ("ethernet", Ethernet),
        ("ethernetDot3", EthernetDot3),
        ("loopback", Loopback),
    ]


class ZXDoc_EthernetData(Structure):
    _pack_ = 1
    _fields_ = [
        ("header", ZXDoc_EthernetDataHeader),
        ("isLocal", ZXDoc_U8, 1),
        ("type", ZXDoc_U8, 2),
        ("reserved", ZXDoc_U8, 5),
        ("length", ZXDoc_U16),
        ("data", ZXDoc_U8 * 1),
    ]


PZXDoc_EthernetData = ctypes.POINTER(ZXDoc_EthernetData)


class ZXDoc_RawData(Structure):
    _pack_ = 1
    _fields_ = [
        ("number", ZXDoc_U64),
        ("absoluteTimestamp", ZXDoc_U64),
        ("relativeTimestamp", ZXDoc_I64),
        ("channel", ZXDoc_U8),
        ("type", ZXDoc_RawDataType),
        ("data", ZXDoc_U8 * 1),
    ]


PZXDoc_RawData = ctypes.POINTER(ZXDoc_RawData)
PPZXDoc_RawData = ctypes.POINTER(PZXDoc_RawData)


class ZXDoc_UserVariable(Structure):
    _pack_ = 1
    _fields_ = [
        ("name", ZXDoc_CharP),
        ("group", ZXDoc_CharP),
        ("valueType", ZXDoc_I32),
        ("unit", ZXDoc_CharP),
        ("initValue", ZXDoc_CharP),
        ("minValue", ZXDoc_CharP),
        ("maxValue", ZXDoc_CharP),
        ("comment", ZXDoc_CharP),
    ]


PZXDoc_UserVariable = ctypes.POINTER(ZXDoc_UserVariable)


ZXDoc_SignalValueType = ZXDoc_I32


class ZXDoc_SignalVariant(Structure):
    _pack_ = 1


PZXDoc_SignalVariant = ctypes.POINTER(ZXDoc_SignalVariant)
PPZXDoc_SignalVariant = ctypes.POINTER(PZXDoc_SignalVariant)
PPPZXDoc_SignalVariant = ctypes.POINTER(PPZXDoc_SignalVariant)


class ZXDoc_SignalVariantValue(Union):
    _pack_ = 1
    _fields_ = [
        ("u8", ZXDoc_U8),
        ("u16", ZXDoc_U16),
        ("u32", ZXDoc_U32),
        ("u64", ZXDoc_U64),
        ("i8", ZXDoc_I8),
        ("i16", ZXDoc_I16),
        ("i32", ZXDoc_I32),
        ("i64", ZXDoc_I64),
        ("f", ZXDoc_Float),
        ("db", ZXDoc_Double),
        ("str", ZXDoc_CharP),
        ("arr", PPPZXDoc_SignalVariant),
    ]


ZXDoc_SignalVariant._fields_ = [
    ("type", ZXDoc_SignalValueType),
    ("data", ZXDoc_SignalVariantValue),
    ("length", ZXDoc_I32),
]


class ZXDoc_SignalValue(Structure):
    _pack_ = 1
    _fields_ = [
        ("sourceId", ZXDoc_CharP),
        ("signalId", ZXDoc_CharP),
        ("timestamp", ZXDoc_U64),
        ("frameNumber", ZXDoc_U64),
        ("rawValue", PZXDoc_SignalVariant),
        ("phyValue", PZXDoc_SignalVariant),
        ("rowIndex", ZXDoc_I32),
        ("colIndex", ZXDoc_I32),
    ]


PZXDoc_SignalValue = ctypes.POINTER(ZXDoc_SignalValue)
PPZXDoc_SignalValue = ctypes.POINTER(PZXDoc_SignalValue)


class ZXDoc_SignalIdentifier(Structure):
    _pack_ = 1
    _fields_ = [("sourceId", ZXDoc_CharP), ("signalId", ZXDoc_CharP)]


PZXDoc_SignalIdentifier = ctypes.POINTER(ZXDoc_SignalIdentifier)


class ZXDoc_E2ECRCCalculatorParameters(Structure):
    _pack_ = 1
    _fields_ = [
        ("width", ZXDoc_U8),  # 8, 16, 32, 64
        ("polynomial", ZXDoc_U64),
        ("initialValue", ZXDoc_U64),
        ("xorValue", ZXDoc_U64),
        ("reflectInput", ZXDoc_Bool),
        ("reflectOutput", ZXDoc_Bool),
    ]


PZXDoc_E2ECRCCalculatorParameters = ctypes.POINTER(ZXDoc_E2ECRCCalculatorParameters)


class ZXDoc_DeviceInfo(Structure):
    _pack_ = 1
    _fields_ = [
        ("id", ZXDoc_Char * 64),
        ("type", ZXDoc_DeviceType),
        ("name", ZXDoc_Char * 128),
        ("busType", ZXDoc_BusType),
        ("logicalChannel", ZXDoc_I32),
        ("enabled", ZXDoc_Bool),
        ("databaseId", ZXDoc_Char * 64),
        ("databaseName", ZXDoc_Char * 128),
    ]


PZXDoc_DeviceInfo = ctypes.POINTER(ZXDoc_DeviceInfo)


ZXDoc_UdsInterfaceHandle = ZXDoc_I32
PZXDoc_UdsInterfaceHandle = ctypes.POINTER(ZXDoc_UdsInterfaceHandle)

ZXDoc_CANFrameType = ZXDoc_U32
ZXDOC_CAN_FRAME_TYPE_CAN = 0  # CAN 帧
ZXDOC_CAN_FRAME_TYPE_CANFD = 1  # CAN FD 帧
ZXDOC_CAN_FRAME_TYPE_CANFD_BRS = 2  # CAN FD 加速帧

ZXDoc_CANTpVersion = ZXDoc_U32
ZXDOC_CAN_TP_ISO15765_2_2004 = 0
ZXDOC_CAN_TP_ISO15765_2_2016 = 1

ZXDoc_UdsPort = ZXDoc_U32


class ZXDoc_DoCANCfg(Structure):
    _pack_ = 1
    _fields_ = [
        ("udsPort", ZXDoc_UdsPort),  # UDS接口类型
        ("channelIndex", ZXDoc_U32),  # 通道索引
        ("frameType", ZXDoc_CANFrameType),  # CAN帧类型
        ("protocolVersion", ZXDoc_CANTpVersion),  # 协议版本
        ("fillByte", ZXDoc_U8),  # 填充字节
        ("isfillByte", ZXDoc_Bool),  # 是否填充字节
        ("p2Timeout", ZXDoc_U16),  # P2超时
        ("p2xTimeout", ZXDoc_U16),  # P2*超时
        ("isModifyEcuSTmin", ZXDoc_Bool),  # 是否替换远程STmin
        ("remoteSTmin", ZXDoc_U8),  # 远程STmin
        ("localSTmin", ZXDoc_U8),  # 本地STmin
        ("blockSize", ZXDoc_U8),  # 本地BS
        ("fcTimeout", ZXDoc_U16),  # 流控响应超时(ms)
    ]


PZXDoc_DoCANCfg = ctypes.POINTER(ZXDoc_DoCANCfg)


class ZXDoc_DoLINCfg(Structure):
    _pack_ = 1
    _fields_ = [
        ("udsPort", ZXDoc_UdsPort),  # UDS接口类型
        ("channelIndex", ZXDoc_U32),  # 通道索引
        ("fillByte", ZXDoc_U8),  # 填充字节
        ("isfillByte", ZXDoc_Bool),  # 是否填充字节
        ("p2Timeout", ZXDoc_U16),  # P2超时
        ("p2xTimeout", ZXDoc_U16),  # P2*超时
        ("isModifyEcuSTmin", ZXDoc_Bool),  # 是否替换远程STmin
        ("remoteSTmin", ZXDoc_U8),  # 远程STmin
    ]


PZXDoc_DoLINCfg = ctypes.POINTER(ZXDoc_DoLINCfg)

ZXDoc_DoipProtocolVersion = ZXDoc_U32
DOIP_TP_ISO_13400_2_2010 = 1
ZXDOC_DOIP_TP_ISO_13400_2_2012 = 2
ZXDOC_DOIP_TP_ISO_13400_2_2019 = 3
ZXDOC_DOIP_TP_ISO_13400_2_2019_AMD_1 = 4

ZXDoc_DoipRoutingActivationType = ZXDoc_U32
ZXDOC_DOIP_ACTIVATION_DEFAULT = 0x00  # 默认激活类型
ZXDOC_DOIP_ACTIVATION_WWH_OBD = 0x01  # WWH-OBD
ZXDOC_DOIP_ACTIVATION_CENTRAL_SECURITY = 0xE0  # 安全模式


class ZXDoc_DoIPCfg(Structure):
    _pack_ = 1
    _fields_ = [
        ("udsPort", ZXDoc_UdsPort),  # UDS接口类型
        ("vehicleIp", ZXDoc_CharP),  # DoIP实体的IP地址
        ("localIp", ZXDoc_CharP),  # 本地IP地址，0.0.0.0表示使用任意地址。
        ("localPort", ZXDoc_U16),  # 本地端口，0表示任意端口。
        ("protocolVersion", ZXDoc_DoipProtocolVersion),  # 协议版本
        ("testerAddress", ZXDoc_U16),  # 测试仪地址（源逻辑地址）
        ("routingActivationType", ZXDoc_DoipRoutingActivationType),  # 路由激活类型
        ("withOEMSpecificData", ZXDoc_Bool),  # 是否有OEM特定的数据
        ("oemSpecificData", ZXDoc_UByte * 4),  # OEM特定的数据
        ("aliveCheckCycle", ZXDoc_U32),  # 主动保活的周期，单位为毫秒，0表示不主动保活
        ("isResponseAliveCheck", ZXDoc_Bool),  # 是否响应存活检查
        ("p2Timeout", ZXDoc_U16),  # P2超时
        ("p2xTimeout", ZXDoc_U16),  # P2*超时
        ("waitForACK", ZXDoc_Bool),  # 每次请求是否等待ACK
        ("ackTimeoutMs", ZXDoc_U32),  # 等待ACK的超时时间
        ("connectTimeout", ZXDoc_U16),  # 等待ACK的超时时间
    ]


PZXDoc_DoIPCfg = ctypes.POINTER(ZXDoc_DoIPCfg)


class ZXDoc_UdsRequest(Structure):
    _pack_ = 1
    _fields_ = [
        ("handle", ZXDoc_UdsInterfaceHandle),  # UDS接口句柄
        ("reqAddr", ZXDoc_U32),  # 请求地址
        ("rspAddr", ZXDoc_U32),  # 响应地址
        ("extend", ZXDoc_Bool),  # 扩展帧
        ("suppressResponse", ZXDoc_Bool),  # True:抑制响应
        ("sid", ZXDoc_U8),  # 请求服务id
        ("dataLen", ZXDoc_U32),  # 参数数组的长度
    ]


PZXDoc_UdsRequest = ctypes.POINTER(ZXDoc_UdsRequest)

ZXDoc_UdsResponseStatus = ZXDoc_U32
ZXDoc_UdsResp_Ok = 0  # 成功
ZXDoc_UdsResp_Canceled = 1  # 取消操作
ZXDoc_UdsResp_SuppressResponse = 2  # 抑制响应
ZXDoc_UdsResp_Failed = 3  # 失败
ZXDoc_UdsResp_Unknown = 0xFF

ZXDoc_UdsResponseType = ZXDoc_U32
ZXDoc_UdsRespType_Negative = 0  # 消极响应
ZXDoc_UdsRespType_Positive = 1  # 积极响应
ZXDoc_UdsRespType_Unknown = 0xFF


class ZXDoc_UdsResponse(Structure):
    _pack_ = 1
    _fields_ = [
        ("status", ZXDoc_UdsResponseStatus),  # 响应状态
        ("responseType", ZXDoc_UdsResponseType),  # 响应类型
        ("errorCode", ZXDoc_U32),  # 错误码
        ("sid", ZXDoc_U8),  # 响应服务id
        ("data", ctypes.POINTER(ZXDoc_UByte)),  # 响应数据缓冲区
        ("dataLen", ZXDoc_U32),  # 参数数组的长度
        ("NRC", ZXDoc_U8),  # 消极响应错误码
    ]


PZXDoc_UdsResponse = ctypes.POINTER(ZXDoc_UdsResponse)

ZXDoc_DBType_UNKNOWN = 0x00
ZXDoc_DBType_DBC = 0x01
ZXDoc_DBType_ODX = 0x02
ZXDoc_DBType_A2L = 0x04
ZXDoc_DBType_LDF = 0x08
ZXDoc_DBType_SysVar = 0x10
ZXDoc_DBType_ExprVar = 0x20
ZXDoc_DBType_UserVar = 0x40
ZXDoc_DBType_Plugin = 0x80
ZXDoc_DBType_BasicDiag = 0x100
ZXDoc_DBType_FuncVar = 0x200
ZXDoc_DBType_ARXML = 0x400  # ARXML数据库
ZXDoc_DBType_ArxmlCAN = 0x800  # ARXML子数据库CAN网络
ZXDoc_DBType_ArxmlETH = 0x1000  # ARXML子数据库ETH网络
ZXDoc_DBType_ALL = 0xFFFF

ZXDoc_DBType = ZXDoc_U32


class ZXDoc_Database(Structure):
    _pack_ = 1
    _fields_ = [
        ("type", ZXDoc_DBType),  #
        ("id", ZXDoc_Char * 64),  #
        ("name", ZXDoc_Char * 128),  #
        ("filePath", ZXDoc_Char * 256),  #
    ]


PZXDoc_Database = ctypes.POINTER(ZXDoc_Database)


class ZXDoc_DBCData(Structure):
    _pack_ = 1
    _fields_ = [
        ("id", ZXDoc_U32),  #
        ("length", ZXDoc_U32),  #
        ("data", ZXDoc_U8 * 64),  #
    ]


PZXDoc_DBCData = ctypes.POINTER(ZXDoc_DBCData)

ZXDoc_DBCSignalValue_unknown = 0
ZXDoc_DBCSignalValue_int = 1
ZXDoc_DBCSignalValue_float = 2
ZXDoc_DBCSignalValue_double = 3

ZXDoc_DBCSignalValueType = ZXDoc_U32

ZXDoc_DBCEncodeObject_raw = 1
ZXDoc_DBCEncodeObject_phy = 2

ZXDoc_DBCEncodeObject = ZXDoc_U32


class ZXDoc_DBCSignalRawValue(Union):
    _pack_ = 1
    _fields_ = [("n", ZXDoc_I64), ("f", ZXDoc_Float), ("d", ZXDoc_Double)]


class ZXDoc_DBCSignalValue(Structure):
    _pack_ = 1
    _fields_ = [
        ("name", ZXDoc_Char * 128),  #
        ("rawValue", ZXDoc_DBCSignalRawValue),  #
        ("rawValueType", ZXDoc_DBCSignalValueType),  #
        ("phyValue", ZXDoc_Double),  #
    ]


PZXDoc_DBCSignalValue = ctypes.POINTER(ZXDoc_DBCSignalValue)


class ZXDoc_DBCMessageValue(Structure):
    _pack_ = 1
    _fields_ = [
        ("id", ZXDoc_U32),  #
        ("name", ZXDoc_Char * 128),  #
        ("isFd", ZXDoc_Bool),  #
        ("isJ1939Frame", ZXDoc_Bool),  #
        ("signalValues", ctypes.POINTER(PZXDoc_DBCSignalValue)),  #
    ]


PZXDoc_DBCMessageValue = ctypes.POINTER(ZXDoc_DBCMessageValue)

_dll.ZXDoc_BuildTime.restype = ZXDoc_CharP
_dll.ZXDoc_Version.restype = ZXDoc_CharP

_dll.ZXDoc_MemoryAlloc.argtypes = [ZXDoc_U64]
_dll.ZXDoc_MemoryAlloc.restype = ZXDoc_VoidP

_dll.ZXDoc_MemoryFree.argtypes = [ZXDoc_VoidP]
_dll.ZXDoc_MemoryFree.restype = None

_dll.ZXDoc_StrClone.argtypes = [ZXDoc_CharP]
_dll.ZXDoc_StrClone.restype = ZXDoc_CharP

_dll.ZXDoc_StrFree.argtypes = [ZXDoc_CharP]
_dll.ZXDoc_StrFree.restype = None

_dll.ZXDoc_SignalVariant_New.argtypes = None
_dll.ZXDoc_SignalVariant_New.restype = PZXDoc_SignalVariant

_dll.ZXDoc_SignalVariant_Init.argtypes = [PZXDoc_SignalVariant]
_dll.ZXDoc_SignalVariant_Init.restype = None

_dll.ZXDoc_SignalVariant_InitArr.argtypes = [PZXDoc_SignalVariant, ZXDoc_I32, ZXDoc_I32]
_dll.ZXDoc_SignalVariant_InitArr.restype = None

_dll.ZXDoc_SignalVariant_ArrGetRows.argtypes = [PZXDoc_SignalVariant]
_dll.ZXDoc_SignalVariant_ArrGetRows.restype = ZXDoc_I32

_dll.ZXDoc_SignalVariant_ArrGetCols.argtypes = [PZXDoc_SignalVariant]
_dll.ZXDoc_SignalVariant_ArrGetCols.restype = ZXDoc_I32

_dll.ZXDoc_SignalVariant_ArrAt.argtypes = [PZXDoc_SignalVariant, ZXDoc_I32, ZXDoc_I32]
_dll.ZXDoc_SignalVariant_ArrAt.restype = PZXDoc_SignalVariant

_dll.ZXDoc_SignalVariant_ArrSet.argtypes = [
    PZXDoc_SignalVariant,
    ZXDoc_I32,
    ZXDoc_I32,
    PZXDoc_SignalVariant,
]
_dll.ZXDoc_SignalVariant_ArrSet.restype = None

_dll.ZXDoc_SignalVariant_SetInt64.argtypes = [PZXDoc_SignalVariant, ZXDoc_I64]
_dll.ZXDoc_SignalVariant_SetInt64.restype = None

_dll.ZXDoc_SignalVariant_SetUint64.argtypes = [PZXDoc_SignalVariant, ZXDoc_U64]
_dll.ZXDoc_SignalVariant_SetUint64.restype = None

_dll.ZXDoc_SignalVariant_SetDouble.argtypes = [PZXDoc_SignalVariant, ZXDoc_Double]
_dll.ZXDoc_SignalVariant_SetDouble.restype = None

_dll.ZXDoc_SignalVariant_SetStr.argtypes = [PZXDoc_SignalVariant, ZXDoc_CharP]
_dll.ZXDoc_SignalVariant_SetStr.restype = None

_dll.ZXDoc_SignalVariant_Clear.argtypes = [PZXDoc_SignalVariant]
_dll.ZXDoc_SignalVariant_Clear.restype = None

_dll.ZXDoc_SignalVariant_Free.argtypes = [PZXDoc_SignalVariant]
_dll.ZXDoc_SignalVariant_Free.restype = None

_dll.ZXDoc_Create.restype = ZXDocHandle

_dll.ZXDoc_Free.argtypes = [ZXDocHandle]
_dll.ZXDoc_Free.restype = ZXDocErrorCode

_dll.ZXDoc_SetServerName.argtypes = [ZXDocHandle, ZXDoc_CharP]
_dll.ZXDoc_SetServerName.restype = ZXDocErrorCode

_dll.ZXDoc_Connect.argtypes = [ZXDocHandle, ZXDoc_CharP, ZXDoc_Bool]
_dll.ZXDoc_Connect.restype = ZXDocErrorCode

_dll.ZXDoc_Disonnect.argtypes = [ZXDocHandle]
_dll.ZXDoc_Disonnect.restype = ZXDocErrorCode

_dll.ZXDoc_App_Log.argtypes = [ZXDocHandle, ZXDoc_LogLevel, ZXDoc_CharP]
_dll.ZXDoc_App_Log.restype = ZXDocErrorCode

_dll.ZXDoc_App_ClearLog.argtypes = [ZXDocHandle]
_dll.ZXDoc_App_ClearLog.restype = ZXDocErrorCode

_dll.ZXDoc_Panel_Log.argtypes = [ZXDocHandle, ZXDoc_CharP, ZXDoc_LogLevel, ZXDoc_CharP]
_dll.ZXDoc_Panel_Log.restype = ZXDocErrorCode

_dll.ZXDoc_App_ExportLog.argtypes = [ZXDocHandle, ZXDoc_CharP]
_dll.ZXDoc_App_ExportLog.restype = ZXDocErrorCode

_dll.ZXDoc_App_GetCurrentProjectPath.argtypes = [ZXDocHandle, ZXDoc_CharP, ZXDoc_U32]
_dll.ZXDoc_App_GetCurrentProjectPath.restype = ZXDocErrorCode

_dll.ZXDoc_App_LoadProject.argtypes = [ZXDocHandle, ZXDoc_CharP]
_dll.ZXDoc_App_LoadProject.restype = ZXDocErrorCode

_dll.ZXDoc_App_GetVersion.argtypes = [ZXDocHandle, ZXDoc_CharP, ZXDoc_U32]
_dll.ZXDoc_App_GetVersion.restype = ZXDocErrorCode


ZCliResponseHandle = ZXDoc_VoidP
_dll.ZXDoc_App_SubwndCmdExec.argtypes = [
    ZXDocHandle,
    ZXDoc_U32,
    ZXDoc_CharP,
    ZXDoc_Bool,
    ctypes.POINTER(ZCliResponseHandle),
]
_dll.ZXDoc_App_SubwndCmdExec.restype = ZXDocErrorCode

_dll.ZXDoc_SetOnConnectedCallback.argtypes = [
    ZXDocHandle,
    ctypes.CFUNCTYPE(None),
    ZXDoc_VoidP,
]
_dll.ZXDoc_SetOnConnectedCallback.restype = ZXDocErrorCode

_dll.ZXDoc_SetOnDisconnectedCallback.argtypes = [
    ZXDocHandle,
    ctypes.CFUNCTYPE(None),
    ZXDoc_VoidP,
]
_dll.ZXDoc_SetOnDisconnectedCallback.restype = ZXDocErrorCode

_dll.ZXDoc_App_AddUserVariable.argtypes = [
    ZXDocHandle,
    PZXDoc_UserVariable,
]
_dll.ZXDoc_App_AddUserVariable.restype = ZXDocErrorCode

_dll.ZXDoc_App_DelUserVariable.argtypes = [
    ZXDocHandle,
    ZXDoc_CharP,
    ZXDoc_CharP,
]
_dll.ZXDoc_App_DelUserVariable.restype = ZXDocErrorCode

_dll.ZXDoc_App_ShowMainWindow.argtypes = [ZXDocHandle]
_dll.ZXDoc_App_ShowMainWindow.restype = ZXDocErrorCode

_dll.ZXDoc_App_HideMainWindow.argtypes = [ZXDocHandle]
_dll.ZXDoc_App_HideMainWindow.restype = ZXDocErrorCode

_dll.ZXDoc_App_CloseMainWindow.argtypes = [ZXDocHandle]
_dll.ZXDoc_App_CloseMainWindow.restype = ZXDocErrorCode

_dll.ZXDoc_Meas_Start.argtypes = [ZXDocHandle]
_dll.ZXDoc_Meas_Start.restype = ZXDocErrorCode

_dll.ZXDoc_Meas_Stop.argtypes = [ZXDocHandle]
_dll.ZXDoc_Meas_Stop.restype = ZXDocErrorCode

_dll.ZXDoc_Meas_IsStarted.argtypes = [ZXDocHandle, ZXDoc_U8P]
_dll.ZXDoc_Meas_IsStarted.restype = ZXDocErrorCode

ZXDoc_Meas_StatChangedCallbackType = ctypes.CFUNCTYPE(None, ZXDoc_U8)
_dll.ZXDoc_Meas_SetStatChangedCallback.argtypes = [
    ZXDocHandle,
    ZXDoc_Meas_StatChangedCallbackType,
    ZXDoc_VoidP,
]
_dll.ZXDoc_Meas_SetStatChangedCallback.restype = ZXDocErrorCode

ZXDoc_Signal_SetSignalsObserverCallbackType = ctypes.CFUNCTYPE(
    None,
    PPZXDoc_SignalValue,
    ZXDoc_U32,
    ZXDoc_VoidP,
)
_dll.ZXDoc_Signal_SetSignalsObserver.argtypes = [
    ZXDocHandle,
    ZXDoc_Signal_SetSignalsObserverCallbackType,
    ZXDoc_VoidP,
]
_dll.ZXDoc_Signal_SetSignalsObserver.restype = ZXDocErrorCode

_dll.ZXDoc_Signal_Subscribe.argtypes = [
    ZXDocHandle,
    PZXDoc_SignalIdentifier,
    ZXDoc_U32,
]
_dll.ZXDoc_Signal_Subscribe.restype = ZXDocErrorCode

_dll.ZXDoc_Signal_Unsubscribe.argtypes = [
    ZXDocHandle,
    PZXDoc_SignalIdentifier,
    ZXDoc_U32,
]
_dll.ZXDoc_Signal_Unsubscribe.restype = ZXDocErrorCode

_dll.ZXDoc_Signal_SetValues.argtypes = [
    ZXDocHandle,
    PPZXDoc_SignalValue,
    ZXDoc_U32,
]
_dll.ZXDoc_Signal_SetValues.restype = ZXDocErrorCode

_dll.ZXDoc_Signal_SetValueUint.argtypes = [
    ZXDocHandle,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ZXDoc_U64,
    ZXDoc_I32,
    ZXDoc_I32,
]
_dll.ZXDoc_Signal_SetValueUint.restype = ZXDocErrorCode

_dll.ZXDoc_Signal_SetValueInt.argtypes = [
    ZXDocHandle,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ZXDoc_I64,
    ZXDoc_I32,
    ZXDoc_I32,
]
_dll.ZXDoc_Signal_SetValueInt.restype = ZXDocErrorCode

_dll.ZXDoc_Signal_SetValueDouble.argtypes = [
    ZXDocHandle,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ZXDoc_Double,
    ZXDoc_I32,
    ZXDoc_I32,
]
_dll.ZXDoc_Signal_SetValueDouble.restype = ZXDocErrorCode

_dll.ZXDoc_Signal_SetValueString.argtypes = [
    ZXDocHandle,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ZXDoc_I32,
    ZXDoc_I32,
]
_dll.ZXDoc_Signal_SetValueString.restype = ZXDocErrorCode

_dll.ZXDoc_SignalValue_New.restype = PZXDoc_SignalValue

_dll.ZXDoc_SignalValue_Free.argtypes = [PZXDoc_SignalValue]

_dll.ZXDoc_Signal_GetValue.argtypes = [
    ZXDocHandle,
    ZXDoc_CharP,
    ZXDoc_CharP,
    PPZXDoc_SignalValue,
    ZXDoc_I32,
    ZXDoc_I32,
]
_dll.ZXDoc_Signal_GetValue.restype = ZXDocErrorCode

_dll.ZXDoc_Signal_GetValues.argtypes = [
    ZXDocHandle,
    PPZXDoc_SignalValue,
    ZXDoc_U32,
]
_dll.ZXDoc_Signal_GetValues.restype = ZXDocErrorCode

_dll.ZXDoc_Signal_GetInitValue.argtypes = [
    ZXDocHandle,
    ZXDoc_CharP,
    ZXDoc_CharP,
    PPZXDoc_SignalValue,
    ZXDoc_I32,
    ZXDoc_I32,
]
_dll.ZXDoc_Signal_GetInitValue.restype = ZXDocErrorCode

ZXDoc_Chnl_SetDataSinkCallbackType = ctypes.CFUNCTYPE(
    None, PPZXDoc_RawData, ZXDoc_U32, ZXDoc_VoidP
)
_dll.ZXDoc_Chnl_SetDataSinkCallback.argtypes = [
    ZXDocHandle,
    ZXDoc_Chnl_SetDataSinkCallbackType,
    ZXDoc_VoidP,
    ZXDoc_FilterMode,
    PZXDoc_DataSinkFilter,
    ZXDoc_U32,
]
_dll.ZXDoc_Chnl_SetDataSinkCallback.restype = ZXDocErrorCode

_dll.ZXDoc_Chnl_GetDatabases.argtypes = [
    ZXDocHandle,
    ZXDoc_BusType,
    ZXDoc_I32,
    ZXDoc_CharPP,
    ZXDoc_U32P,
]
_dll.ZXDoc_Chnl_GetDatabases.restype = ZXDocErrorCode

_dll.ZXDoc_Chnl_SetDatabases.argtypes = [
    ZXDocHandle,
    ZXDoc_BusType,
    ZXDoc_I32,
    ZXDoc_CharPP,
    ZXDoc_U32,
]
_dll.ZXDoc_Chnl_SetDatabases.restype = ZXDocErrorCode

_dll.ZXDoc_Chnl_LinWakeUp.argtypes = [ZXDocHandle, ZXDoc_I32]
_dll.ZXDoc_Chnl_LinWakeUp.restype = ZXDocErrorCode

_dll.ZXDoc_Cali_AddSignalToMeasList.argtypes = [
    ZXDocHandle,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ZXDoc_MeasEventType,
    ZXDoc_U32,
    ZXDoc_U32,
    ZXDoc_U32,
]
_dll.ZXDoc_Cali_AddSignalToMeasList.restype = ZXDocErrorCode

_dll.ZXDoc_Cali_RemoveSignalFromMeasList.argtypes = [
    ZXDocHandle,
    ZXDoc_CharP,
    ZXDoc_CharP,
]
_dll.ZXDoc_Cali_RemoveSignalFromMeasList.restype = ZXDocErrorCode

ZXDoc_Channel = ZXDoc_VoidP
ZXDoc_ChannelP = ctypes.POINTER(ZXDoc_VoidP)
ZXDoc_ChannelPP = ctypes.POINTER(ZXDoc_ChannelP)

_dll.ZXDoc_Channel_New.argtypes = None
_dll.ZXDoc_Channel_New.restype = ZXDoc_Channel

_dll.ZXDoc_Channel_Free.argtypes = [ZXDoc_Channel]
_dll.ZXDoc_Channel_Free.restype = None

_dll.ZXDoc_Channels_Free.argtypes = [ZXDoc_ChannelP]
_dll.ZXDoc_Channels_Free.restype = None

_dll.ZXDoc_Channel_GetBusType.argtypes = [ZXDoc_Channel]
_dll.ZXDoc_Channel_GetBusType.restype = ZXDoc_BusType

_dll.ZXDoc_Channel_GetLogicalIndex.argtypes = [ZXDoc_Channel]
_dll.ZXDoc_Channel_GetLogicalIndex.restype = ZXDoc_I32

_dll.ZXDoc_Channel_GetPhysicalIndex.argtypes = [ZXDoc_Channel]
_dll.ZXDoc_Channel_GetPhysicalIndex.restype = ZXDoc_I32

_dll.ZXDoc_Channel_IsEnabled.argtypes = [ZXDoc_Channel]
_dll.ZXDoc_Channel_IsEnabled.restype = ZXDoc_Bool

_dll.ZXDoc_Channel_IsActivated.argtypes = [ZXDoc_Channel]
_dll.ZXDoc_Channel_IsActivated.restype = ZXDoc_Bool

_dll.ZXDoc_Channel_GetDatabaseCount.argtypes = [ZXDoc_Channel]
_dll.ZXDoc_Channel_GetDatabaseCount.restype = ZXDoc_U32

_dll.ZXDoc_Channel_GetDatabase.argtypes = [ZXDoc_Channel, ZXDoc_U32]
_dll.ZXDoc_Channel_GetDatabase.restype = ZXDoc_CharP


_dll.ZXDoc_Chnl_GetChannels.argtypes = [ZXDocHandle, ZXDoc_BusType, ZXDoc_ChannelPP]
_dll.ZXDoc_Chnl_GetChannels.restype = ZXDocErrorCode

_dll.ZXDoc_Chnl_Transmit.argtypes = [
    ZXDocHandle,
    PPZXDoc_RawData,
    ZXDoc_U32P,
]
_dll.ZXDoc_Chnl_Transmit.restype = ZXDocErrorCode

_dll.ZXDoc_Chnl_GetAvailableCanTxQueueCount.argtypes = [
    ZXDocHandle,
    ZXDoc_I32,
    ZXDoc_I32P,
]
_dll.ZXDoc_Chnl_GetAvailableCanTxQueueCount.restype = ZXDocErrorCode

_dll.ZXDoc_Chnl_ClearCanTxQueue.argtypes = [ZXDocHandle, ZXDoc_I32, ZXDoc_BoolP]
_dll.ZXDoc_Chnl_ClearCanTxQueue.restype = ZXDocErrorCode

_dll.ZXDoc_Chnl_CanQueueTransmit.argtypes = [
    ZXDocHandle,
    PPZXDoc_RawData,
    ZXDoc_U32,
    ZXDoc_BoolP,
]
_dll.ZXDoc_Chnl_CanQueueTransmit.restype = ZXDocErrorCode

_dll.ZXDoc_Chnl_ScheduledFrameCapacity.argtypes = [ZXDocHandle, ZXDoc_I32, ZXDoc_I32P]
_dll.ZXDoc_Chnl_ScheduledFrameCapacity.restype = ZXDocErrorCode

_dll.ZXDoc_Chnl_StartedScheduledFrameCount.argtypes = [
    ZXDocHandle,
    ZXDoc_I32,
    ZXDoc_I32P,
]
_dll.ZXDoc_Chnl_StartedScheduledFrameCount.restype = ZXDocErrorCode

_dll.ZXDoc_Chnl_StartScheduledFrame.argtypes = [
    ZXDocHandle,
    ZXDoc_I32,
    PZXDoc_CANFDData,
    ZXDoc_I32,
    ZXDoc_I32P,
]
_dll.ZXDoc_Chnl_StartScheduledFrame.restype = ZXDocErrorCode

_dll.ZXDoc_Chnl_RestartScheduledFrame.argtypes = [
    ZXDocHandle,
    ZXDoc_I32,
    ZXDoc_I32P,
    PZXDoc_CANFDData,
    ZXDoc_I32,
]
_dll.ZXDoc_Chnl_RestartScheduledFrame.restype = ZXDocErrorCode

_dll.ZXDoc_Chnl_StopScheduledFrame.argtypes = [ZXDocHandle, ZXDoc_I32]
_dll.ZXDoc_Chnl_StopScheduledFrame.restype = ZXDocErrorCode

_dll.ZXDoc_CANFDData_New.restype = PZXDoc_CANFDData
_dll.ZXDoc_LINData_New.restype = PZXDoc_LINData

_dll.ZXDoc_RawData_New_CANFDData.restype = PZXDoc_RawData
_dll.ZXDoc_RawData_New_LINData.restype = PZXDoc_RawData

_dll.ZXDoc_RawData_Get_CANFDData.argtypes = [PZXDoc_RawData]
_dll.ZXDoc_RawData_Get_CANFDData.restype = PZXDoc_CANFDData

_dll.ZXDoc_RawData_Get_LINData.argtypes = [PZXDoc_RawData]
_dll.ZXDoc_RawData_Get_LINData.restype = PZXDoc_LINData

_dll.ZXDoc_RawData_Free.argtypes = [PZXDoc_RawData]
_dll.ZXDoc_RawData_Free.restype = None

_dll.ZXDoc_Cali_Connect.argtypes = [ZXDocHandle]
_dll.ZXDoc_Cali_Connect.restype = ZXDocErrorCode

_dll.ZXDoc_Cali_Disonnect.argtypes = [ZXDocHandle]
_dll.ZXDoc_Cali_Disonnect.restype = ZXDocErrorCode

_dll.ZXDoc_Cali_StartDataAcquisition.argtypes = [ZXDocHandle]
_dll.ZXDoc_Cali_StartDataAcquisition.restype = ZXDocErrorCode

_dll.ZXDoc_Cali_StopDataAcquisition.argtypes = [ZXDocHandle]
_dll.ZXDoc_Cali_StopDataAcquisition.restype = ZXDocErrorCode

_dll.ZXDoc_Cali_IsNeedCacheSyncAll.argtypes = [ZXDocHandle, ctypes.POINTER(ZXDoc_Bool)]
_dll.ZXDoc_Cali_IsNeedCacheSyncAll.restype = ZXDocErrorCode

_dll.ZXDoc_Cali_CanSyncDownloadAll.argtypes = [ZXDocHandle, ctypes.POINTER(ZXDoc_Bool)]
_dll.ZXDoc_Cali_CanSyncDownloadAll.restype = ZXDocErrorCode

_dll.ZXDoc_Cali_SyncUploadAll.argtypes = [ZXDocHandle, ZXDoc_Bool]
_dll.ZXDoc_Cali_SyncUploadAll.restype = ZXDocErrorCode

_dll.ZXDoc_Cali_SyncDownloadAll.argtypes = [ZXDocHandle]
_dll.ZXDoc_Cali_SyncDownloadAll.restype = ZXDocErrorCode

ZXDoc_EcuMemPageType = ZXDoc_U32
ZXDoc_EcuMemPageType_RAM = 0
ZXDoc_EcuMemPageType_FLASH = 1

_dll.ZXDoc_Cali_SelectMemoryPageAll.argtypes = [ZXDocHandle, ZXDoc_EcuMemPageType]
_dll.ZXDoc_Cali_SelectMemoryPageAll.restype = ZXDocErrorCode

_dll.ZXDoc_Cali_CanSelectMemoryPage.argtypes = [ZXDocHandle, ctypes.POINTER(ZXDoc_Bool)]
_dll.ZXDoc_Cali_CanSelectMemoryPage.restype = ZXDocErrorCode

_dll.ZXDoc_Cali_GetCurrentMemoryPage.argtypes = [
    ZXDocHandle,
    ctypes.POINTER(ZXDoc_EcuMemPageType),
]
_dll.ZXDoc_Cali_GetCurrentMemoryPage.restype = ZXDocErrorCode

_dll.ZXDoc_E2E_GetCrcCalculator.argtypes = [
    ZXDocHandle,
    ZXDoc_U64P,
    ZXDoc_E2ECrcType,
    PZXDoc_E2ECRCCalculatorParameters,
]
_dll.ZXDoc_E2E_GetCrcCalculator.restype = ZXDocErrorCode

_dll.ZXDoc_E2E_CrcCalculate.argtypes = [
    ZXDocHandle,
    ZXDoc_U64,
    ZXDoc_VoidP,
    ZXDoc_U32,
    ZXDoc_U64P,
    ZXDoc_U64,
]
_dll.ZXDoc_E2E_CrcCalculate.restype = ZXDocErrorCode

_dll.ZXDoc_Simu_StartCanSimulation.argtypes = [ZXDocHandle]
_dll.ZXDoc_Simu_StartCanSimulation.restype = ZXDocErrorCode

_dll.ZXDoc_Simu_StopCanSimulation.argtypes = [ZXDocHandle]
_dll.ZXDoc_Simu_StopCanSimulation.restype = ZXDocErrorCode

_dll.ZXDoc_Simu_ActiveCanSend.argtypes = [
    ZXDocHandle,
    ZXDoc_I32,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ZXDoc_Bool,
]
_dll.ZXDoc_Simu_ActiveCanSend.restype = ZXDocErrorCode

_dll.ZXDoc_Simu_ActiveCanSendBySignalId.argtypes = [
    ZXDocHandle,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ZXDoc_Bool,
]
_dll.ZXDoc_Simu_ActiveCanSendBySignalId.restype = ZXDocErrorCode

_dll.ZXDoc_Simu_SetCanSendType.argtypes = [
    ZXDocHandle,
    ZXDoc_I32,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ZXDoc_SimuSendType,
]
_dll.ZXDoc_Simu_SetCanSendType.restype = ZXDocErrorCode

_dll.ZXDoc_Simu_SetCanCycleTime.argtypes = [
    ZXDocHandle,
    ZXDoc_I32,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ZXDoc_U32,
]
_dll.ZXDoc_Simu_SetCanCycleTime.restype = ZXDocErrorCode

_dll.ZXDoc_Simu_SetCanSendRepetitions.argtypes = [
    ZXDocHandle,
    ZXDoc_I32,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ZXDoc_I64,
]
_dll.ZXDoc_Simu_SetCanSendRepetitions.restype = ZXDocErrorCode

_dll.ZXDoc_Simu_SetCanSignalValue.argtypes = [
    ZXDocHandle,
    ZXDoc_I32,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ZXDoc_Double,
]
_dll.ZXDoc_Simu_SetCanSignalValue.restype = ZXDocErrorCode

_dll.ZXDoc_Simu_SetCanSignalValues.argtypes = [
    ZXDocHandle,
    ZXDoc_I32,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ctypes.POINTER(ZXDoc_CharP),
    ZXDoc_DoubleP,
    ZXDoc_U32,
]
_dll.ZXDoc_Simu_SetCanSignalValues.restype = ZXDocErrorCode

_dll.ZXDoc_Simu_SetCanHighPrecisionScheduled.argtypes = [
    ZXDocHandle,
    ZXDoc_Bool,
]
_dll.ZXDoc_Simu_SetCanHighPrecisionScheduled.restype = ZXDocErrorCode

_dll.ZXDoc_Simu_IsCanHighPrecisionScheduled.argtypes = [
    ZXDocHandle,
    ZXDoc_BoolP,
]
_dll.ZXDoc_Simu_IsCanHighPrecisionScheduled.restype = ZXDocErrorCode

_dll.ZXDoc_Simu_SetCanFdType.argtypes = [
    ZXDocHandle,
    ZXDoc_I32,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ZXDoc_Bool,
]
_dll.ZXDoc_Simu_SetCanFdType.restype = ZXDocErrorCode

_dll.ZXDoc_Simu_SetCanFdBrs.argtypes = [
    ZXDocHandle,
    ZXDoc_I32,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ZXDoc_Bool,
]
_dll.ZXDoc_Simu_SetCanFdBrs.restype = ZXDocErrorCode

_dll.ZXDoc_Dev_GetDevices.argtypes = [
    ZXDocHandle,
    ZXDoc_DeviceType,
    PZXDoc_DeviceInfo,
    ZXDoc_U32P,
]
_dll.ZXDoc_Dev_GetDevices.restype = ZXDocErrorCode

_dll.ZXDoc_Diag_CreateDoCANInterface.argtypes = [
    ZXDocHandle,
    PZXDoc_UdsInterfaceHandle,
    PZXDoc_DoCANCfg,
]
_dll.ZXDoc_Diag_CreateDoCANInterface.restype = ZXDocErrorCode

_dll.ZXDoc_Diag_CreateDoLINInterface.argtypes = [
    ZXDocHandle,
    PZXDoc_UdsInterfaceHandle,
    PZXDoc_DoLINCfg,
]
_dll.ZXDoc_Diag_CreateDoLINInterface.restype = ZXDocErrorCode

_dll.ZXDoc_Diag_CreateDoIPInterface.argtypes = [
    ZXDocHandle,
    PZXDoc_UdsInterfaceHandle,
    PZXDoc_DoIPCfg,
]
_dll.ZXDoc_Diag_CreateDoIPInterface.restype = ZXDocErrorCode

_dll.ZXDoc_Diag_FreeUdsInterface.argtypes = [ZXDocHandle, ZXDoc_UdsInterfaceHandle]
_dll.ZXDoc_Diag_FreeUdsInterface.restype = ZXDocErrorCode

_dll.ZXDoc_Diag_UdsRequest.argtypes = [
    ZXDocHandle,
    PZXDoc_UdsRequest,
    ZXDoc_VoidP,
    PZXDoc_UdsResponse,
]
_dll.ZXDoc_Diag_UdsRequest.restype = ZXDocErrorCode

_dll.ZXDoc_Diag_UdsFunctionalRequest.argtypes = [
    ZXDocHandle,
    ZXDoc_U32,
    PZXDoc_UdsRequest,
    ZXDoc_VoidP,
    PZXDoc_UdsResponse,
]
_dll.ZXDoc_Diag_UdsFunctionalRequest.restype = ZXDocErrorCode

_dll.ZXDoc_Diag_CancelRequest.argtypes = [ZXDocHandle, ZXDoc_UdsInterfaceHandle]
_dll.ZXDoc_Diag_CancelRequest.restype = ZXDocErrorCode

_dll.ZXDoc_Diag_GetErrorMssage.argtypes = [
    ZXDocHandle,
    ZXDoc_UdsInterfaceHandle,
    PZXDoc_UdsResponse,
    ZXDoc_CharP,
    ZXDoc_U32,
]
_dll.ZXDoc_Diag_GetErrorMssage.restype = ZXDocErrorCode

_dll.ZXDoc_Diag_SatrtSessionKeep.argtypes = [
    ZXDocHandle,
    ZXDoc_UdsInterfaceHandle,
    ZXDoc_U32,
    ZXDoc_U32,
    ZXDoc_Bool,
    ZXDoc_Bool,
]
_dll.ZXDoc_Diag_SatrtSessionKeep.restype = ZXDocErrorCode

_dll.ZXDoc_Diag_StopSessionKeep.argtypes = [ZXDocHandle, ZXDoc_UdsInterfaceHandle]
_dll.ZXDoc_Diag_StopSessionKeep.restype = ZXDocErrorCode

_dll.ZXDoc_Diag_CalcSecurityKey.argtypes = [
    ZXDocHandle,
    ZXDoc_CharP,  # dllPath
    ZXDoc_U8,  # securityLevel
    ZXDoc_CharP,  # seed
    ZXDoc_U32,  # seedSize
    ZXDoc_CharP,  # variant
    ZXDoc_CharP,  # key
    ZXDoc_U32P,  # keySize
]
_dll.ZXDoc_Diag_CalcSecurityKey.restype = ZXDocErrorCode

_dll.ZXDoc_Diag_StartAutoFlowControl.argtypes = [
    ZXDocHandle,
    ZXDoc_UdsInterfaceHandle,
    ZXDoc_U32,
    ZXDoc_U32,
]
_dll.ZXDoc_Diag_StartAutoFlowControl.restype = ZXDocErrorCode

_dll.ZXDoc_Diag_StopAutoFlowControl.argtypes = [ZXDocHandle, ZXDoc_UdsInterfaceHandle]
_dll.ZXDoc_Diag_StopAutoFlowControl.restype = ZXDocErrorCode

ZXDoc_FlashFileInfo = ZXDoc_VoidP

ZXDoc_FalshFileType = ZXDoc_U32
ZFlashFileType_GAC = 0
ZFlashFileType_GACFlashDriver = 1
ZFlashFileType_SRec = 2  # S19
ZFlashFileType_IHex = 2  # hex
ZFlashFileType_Bin = 3  # raw files
ZFlashFileType_Script = 4
ZFlashFileType_Unknown = 0xFF

_dll.ZXDoc_FlashFileInfo_New.argtypes = None
_dll.ZXDoc_FlashFileInfo_New.restype = ZXDoc_FlashFileInfo

_dll.ZXDoc_FlashFileInfo_Free.argtypes = [ZXDoc_FlashFileInfo]
_dll.ZXDoc_FlashFileInfo_Free.restype = None

_dll.ZXDoc_FlashFileInfo_GetFilePath.argtypes = [ZXDoc_FlashFileInfo]
_dll.ZXDoc_FlashFileInfo_GetFilePath.restype = ZXDoc_CharP

_dll.ZXDoc_FlashFileInfo_GetType.argtypes = [ZXDoc_FlashFileInfo]
_dll.ZXDoc_FlashFileInfo_GetType.restype = ZXDoc_FalshFileType

_dll.ZXDoc_FlashFileInfo_GetBlockCount.argtypes = [ZXDoc_FlashFileInfo]
_dll.ZXDoc_FlashFileInfo_GetBlockCount.restype = ZXDoc_U32

_dll.ZXDoc_FlashFileInfo_GetBlock.argtypes = [
    ZXDoc_FlashFileInfo,
    ZXDoc_I32,
    ZXDoc_U64P,
    ctypes.POINTER(ZXDoc_UByteP),
    ZXDoc_U32P,
]
_dll.ZXDoc_FlashFileInfo_GetBlock.restype = ZXDoc_Bool

_dll.ZXDoc_Diag_LoadFlashFileBlocks.argtypes = [
    ZXDocHandle,
    ZXDoc_CharP,
    ZXDoc_FlashFileInfo,
]
_dll.ZXDoc_Diag_LoadFlashFileBlocks.restype = ZXDoc_Bool

# Database

ZXDoc_DBCMultiplexerIndicator = ZXDoc_U32
ZDBCMultiplexer_Normal = 0
ZDBCMultiplexer_Multiplexed = 1
ZDBCMultiplexer_Multiplexer = 2
ZDBCMultiplexer_Unknown = 0xFF

ZXDoc_DBCSigValueType = ZXDoc_U32
ZDBCSigValType_Int = 0
ZDBCSigValType_Float = 1
ZDBCSigValType_Double = 2
ZDBCSigValType_Unknown = 0xFF

ZXDoc_DBCBusType = ZXDoc_U32
ZDBCBusType_Can = 0
ZDBCBusType_CanFd = 1
ZDBCBusType_Unknown = 2

# ZXDoc_DBCSignalInfo

ZXDoc_DBCSignalInfo = ZXDoc_VoidP
ZXDoc_DBCSignalInfoP = ctypes.POINTER(ZXDoc_DBCSignalInfo)
ZXDoc_DBCSignalInfoPP = ctypes.POINTER(ZXDoc_DBCSignalInfoP)

_dll.ZXDoc_DBCSignalInfo_New.argtypes = None
_dll.ZXDoc_DBCSignalInfo_New.restype = ZXDoc_DBCSignalInfo

_dll.ZXDoc_DBCSignalInfo_Free.argtypes = [ZXDoc_DBCSignalInfo]
_dll.ZXDoc_DBCSignalInfo_Free.restype = None

_dll.ZXDoc_DBCSignalInfos_Free.argtypes = [ZXDoc_DBCSignalInfoP]
_dll.ZXDoc_DBCSignalInfos_Free.restype = None

_dll.ZXDoc_DBCSignalInfo_GetName.argtypes = [ZXDoc_DBCSignalInfo]
_dll.ZXDoc_DBCSignalInfo_GetName.restype = ZXDoc_CharP

_dll.ZXDoc_DBCSignalInfo_GetComment.argtypes = [ZXDoc_DBCSignalInfo]
_dll.ZXDoc_DBCSignalInfo_GetComment.restype = ZXDoc_CharP

_dll.ZXDoc_DBCSignalInfo_GetMultiplexerIndicator.argtypes = [ZXDoc_DBCSignalInfo]
_dll.ZXDoc_DBCSignalInfo_GetMultiplexerIndicator.restype = ZXDoc_DBCMultiplexerIndicator

_dll.ZXDoc_DBCSignalInfo_GetMultiplexerValue.argtypes = [ZXDoc_DBCSignalInfo]
_dll.ZXDoc_DBCSignalInfo_GetMultiplexerValue.restype = ZXDoc_U32

_dll.ZXDoc_DBCSignalInfo_GetStartBit.argtypes = [ZXDoc_DBCSignalInfo]
_dll.ZXDoc_DBCSignalInfo_GetStartBit.restype = ZXDoc_U32

_dll.ZXDoc_DBCSignalInfo_GetLength.argtypes = [ZXDoc_DBCSignalInfo]
_dll.ZXDoc_DBCSignalInfo_GetLength.restype = ZXDoc_U32

_dll.ZXDoc_DBCSignalInfo_IsIntel.argtypes = [ZXDoc_DBCSignalInfo]
_dll.ZXDoc_DBCSignalInfo_IsIntel.restype = ZXDoc_Bool

_dll.ZXDoc_DBCSignalInfo_IsSigned.argtypes = [ZXDoc_DBCSignalInfo]
_dll.ZXDoc_DBCSignalInfo_IsSigned.restype = ZXDoc_Bool

_dll.ZXDoc_DBCSignalInfo_GetFactor.argtypes = [ZXDoc_DBCSignalInfo]
_dll.ZXDoc_DBCSignalInfo_GetFactor.restype = ZXDoc_Double

_dll.ZXDoc_DBCSignalInfo_GetOffset.argtypes = [ZXDoc_DBCSignalInfo]
_dll.ZXDoc_DBCSignalInfo_GetOffset.restype = ZXDoc_Double

_dll.ZXDoc_DBCSignalInfo_GetMinValue.argtypes = [ZXDoc_DBCSignalInfo]
_dll.ZXDoc_DBCSignalInfo_GetMinValue.restype = ZXDoc_Double

_dll.ZXDoc_DBCSignalInfo_GetMaxValue.argtypes = [ZXDoc_DBCSignalInfo]
_dll.ZXDoc_DBCSignalInfo_GetMaxValue.restype = ZXDoc_Double

_dll.ZXDoc_DBCSignalInfo_GetUnit.argtypes = [ZXDoc_DBCSignalInfo]
_dll.ZXDoc_DBCSignalInfo_GetUnit.restype = ZXDoc_CharP

_dll.ZXDoc_DBCSignalInfo_GetValueType.argtypes = [ZXDoc_DBCSignalInfo]
_dll.ZXDoc_DBCSignalInfo_GetValueType.restype = ZXDoc_DBCSigValueType

_dll.ZXDoc_DBCSignalInfo_GetReceiverCount.argtypes = [ZXDoc_DBCSignalInfo]
_dll.ZXDoc_DBCSignalInfo_GetReceiverCount.restype = ZXDoc_U32

_dll.ZXDoc_DBCSignalInfo_GetReceiver.argtypes = [ZXDoc_DBCSignalInfo, ZXDoc_U32]
_dll.ZXDoc_DBCSignalInfo_GetReceiver.restype = ZXDoc_CharP

_dll.ZXDoc_DBCSignalInfo_GetValueTableSize.argtypes = [ZXDoc_DBCSignalInfo]
_dll.ZXDoc_DBCSignalInfo_GetValueTableSize.restype = ZXDoc_U32

_dll.ZXDoc_DBCSignalInfo_GetValueTable.argtypes = [
    ZXDoc_DBCSignalInfo,
    ZXDoc_U32,
    ZXDoc_I64P,
    ZXDoc_CharPP,
]
_dll.ZXDoc_DBCSignalInfo_GetValueTable.restype = ZXDoc_Bool

_dll.ZXDoc_DBCSignalInfo_GetAttrCount.argtypes = [ZXDoc_DBCSignalInfo]
_dll.ZXDoc_DBCSignalInfo_GetAttrCount.restype = ZXDoc_U32

_dll.ZXDoc_DBCSignalInfo_GetAttr.argtypes = [
    ZXDoc_DBCSignalInfo,
    ZXDoc_U32,
    ZXDoc_CharPP,
    ZXDoc_CharPP,
]
_dll.ZXDoc_DBCSignalInfo_GetAttr.restype = ZXDoc_Bool

# ZXDoc_DBCMessageInfo

ZXDoc_DBCMessageInfo = ZXDoc_VoidP
ZXDoc_DBCMessageInfoP = ctypes.POINTER(ZXDoc_DBCMessageInfo)
ZXDoc_DBCMessageInfoPP = ctypes.POINTER(ZXDoc_DBCMessageInfoP)

_dll.ZXDoc_DBCMessageInfo_New.argtypes = None
_dll.ZXDoc_DBCMessageInfo_New.restype = ZXDoc_DBCMessageInfo

_dll.ZXDoc_DBCMessageInfo_Free.argtypes = [ZXDoc_DBCMessageInfo]
_dll.ZXDoc_DBCMessageInfo_Free.restype = None

_dll.ZXDoc_DBCMessageInfos_Free.argtypes = [ZXDoc_DBCMessageInfoP]
_dll.ZXDoc_DBCMessageInfos_Free.restype = None

_dll.ZXDoc_DBCMessageInfo_GetDatabaseId.argtypes = [ZXDoc_DBCMessageInfo]
_dll.ZXDoc_DBCMessageInfo_GetDatabaseId.restype = ZXDoc_CharP

_dll.ZXDoc_DBCMessageInfo_GetId.argtypes = [ZXDoc_DBCMessageInfo]
_dll.ZXDoc_DBCMessageInfo_GetId.restype = ZXDoc_U32

_dll.ZXDoc_DBCMessageInfo_GetLen.argtypes = [ZXDoc_DBCMessageInfo]
_dll.ZXDoc_DBCMessageInfo_GetLen.restype = ZXDoc_U32

_dll.ZXDoc_DBCMessageInfo_GetName.argtypes = [ZXDoc_DBCMessageInfo]
_dll.ZXDoc_DBCMessageInfo_GetName.restype = ZXDoc_CharP

_dll.ZXDoc_DBCMessageInfo_GetComment.argtypes = [ZXDoc_DBCMessageInfo]
_dll.ZXDoc_DBCMessageInfo_GetComment.restype = ZXDoc_CharP

_dll.ZXDoc_DBCMessageInfo_GetBusType.argtypes = [ZXDoc_DBCMessageInfo]
_dll.ZXDoc_DBCMessageInfo_GetBusType.restype = ZXDoc_DBCBusType

_dll.ZXDoc_DBCMessageInfo_IsJ1939Frame.argtypes = [ZXDoc_DBCMessageInfo]
_dll.ZXDoc_DBCMessageInfo_IsJ1939Frame.restype = ZXDoc_Bool

_dll.ZXDoc_DBCMessageInfo_IsBRS.argtypes = [ZXDoc_DBCMessageInfo]
_dll.ZXDoc_DBCMessageInfo_IsBRS.restype = ZXDoc_Bool

_dll.ZXDoc_DBCMessageInfo_GetTransmitter.argtypes = [ZXDoc_DBCMessageInfo]
_dll.ZXDoc_DBCMessageInfo_GetTransmitter.restype = ZXDoc_CharP

_dll.ZXDoc_DBCMessageInfo_GetMultiplexer.argtypes = [ZXDoc_DBCMessageInfo]
_dll.ZXDoc_DBCMessageInfo_GetMultiplexer.restype = ZXDoc_CharP

_dll.ZXDoc_DBCMessageInfo_GetSignalNameCount.argtypes = [ZXDoc_DBCMessageInfo]
_dll.ZXDoc_DBCMessageInfo_GetSignalNameCount.restype = ZXDoc_U32

_dll.ZXDoc_DBCMessageInfo_GetSignalName.argtypes = [ZXDoc_DBCMessageInfo, ZXDoc_U32]
_dll.ZXDoc_DBCMessageInfo_GetSignalName.restype = ZXDoc_CharP

_dll.ZXDoc_DBCMessageInfo_GetSignalGroupCount.argtypes = [ZXDoc_DBCMessageInfo]
_dll.ZXDoc_DBCMessageInfo_GetSignalGroupCount.restype = ZXDoc_U32

_dll.ZXDoc_DBCMessageInfo_GetSignalGroupName.argtypes = [
    ZXDoc_DBCMessageInfo,
    ZXDoc_U32,
]
_dll.ZXDoc_DBCMessageInfo_GetSignalGroupName.restype = ZXDoc_CharP

_dll.ZXDoc_DBCMessageInfo_GetSignalGroupSignalCount.argtypes = [
    ZXDoc_DBCMessageInfo,
    ZXDoc_U32,
]
_dll.ZXDoc_DBCMessageInfo_GetSignalGroupSignalCount.restype = ZXDoc_U32

_dll.ZXDoc_DBCMessageInfo_GetSignalGroupSglName.argtypes = [
    ZXDoc_DBCMessageInfo,
    ZXDoc_U32,
    ZXDoc_U32,
]
_dll.ZXDoc_DBCMessageInfo_GetSignalGroupSglName.restype = ZXDoc_CharP

_dll.ZXDoc_DBCMessageInfo_GetAttrCount.argtypes = [ZXDoc_DBCMessageInfo]
_dll.ZXDoc_DBCMessageInfo_GetAttrCount.restype = ZXDoc_U32

_dll.ZXDoc_DBCMessageInfo_GetAttr.argtypes = [
    ZXDoc_DBCMessageInfo,
    ZXDoc_U32,
    ZXDoc_CharPP,
    ZXDoc_CharPP,
]
_dll.ZXDoc_DBCMessageInfo_GetAttr.restype = ZXDoc_Bool

_dll.ZXDoc_DBCSignalValue_New.argtypes = None
_dll.ZXDoc_DBCSignalValue_New.restype = PZXDoc_DBCSignalValue

_dll.ZXDoc_DBCSignalValue_Free.argtypes = [PZXDoc_DBCSignalValue]
_dll.ZXDoc_DBCSignalValue_Free.restype = None

_dll.ZXDoc_DBCSignalValue_SetName.argtypes = [PZXDoc_DBCSignalValue, ZXDoc_CharP]
_dll.ZXDoc_DBCSignalValue_SetName.restype = None

_dll.ZXDoc_DBCSignalValue_SetPhyValue.argtypes = [PZXDoc_DBCSignalValue, ZXDoc_Double]
_dll.ZXDoc_DBCSignalValue_SetPhyValue.restype = None

_dll.ZXDoc_DBCSignalValue_SetRawInt.argtypes = [PZXDoc_DBCSignalValue, ZXDoc_I64]
_dll.ZXDoc_DBCSignalValue_SetRawInt.restype = None

_dll.ZXDoc_DBCSignalValue_SetRawUint.argtypes = [PZXDoc_DBCSignalValue, ZXDoc_U64]
_dll.ZXDoc_DBCSignalValue_SetRawUint.restype = None

_dll.ZXDoc_DBCSignalValue_SetRawFloat.argtypes = [PZXDoc_DBCSignalValue, ZXDoc_Float]
_dll.ZXDoc_DBCSignalValue_SetRawFloat.restype = None

_dll.ZXDoc_DBCSignalValue_SetRawDouble.argtypes = [PZXDoc_DBCSignalValue, ZXDoc_Double]
_dll.ZXDoc_DBCSignalValue_SetRawDouble.restype = None

_dll.ZXDoc_DBCMsgValue_New.argtypes = None
_dll.ZXDoc_DBCMsgValue_New.restype = PZXDoc_DBCMessageValue

_dll.ZXDoc_DBCMsgValue_Free.argtypes = [PZXDoc_DBCMessageValue]
_dll.ZXDoc_DBCMsgValue_Free.restype = None

_dll.ZXDoc_DBCMsgValue_AppendSignal.argtypes = [
    PZXDoc_DBCMessageValue,
    PZXDoc_DBCSignalValue,
]
_dll.ZXDoc_DBCMsgValue_AppendSignal.restype = None

_dll.ZXDoc_DBCMsgValue_SignalCount.argtypes = [PZXDoc_DBCMessageValue]
_dll.ZXDoc_DBCMsgValue_SignalCount.restype = ZXDoc_U32

_dll.ZXDoc_DBCMsgValue_SignalAt.argtypes = [PZXDoc_DBCMessageValue, ZXDoc_U32]
_dll.ZXDoc_DBCMsgValue_SignalAt.restype = PZXDoc_DBCSignalValue

_dll.ZXDoc_DBCMsgValue_GetSignal.argtypes = [PZXDoc_DBCMessageValue, ZXDoc_CharP]
_dll.ZXDoc_DBCMsgValue_GetSignal.restype = PZXDoc_DBCSignalValue

_dll.ZXDoc_DB_GetDatabases.argtypes = [
    ZXDocHandle,
    PZXDoc_Database,
    ZXDoc_U32P,
]
_dll.ZXDoc_DB_GetDatabases.restype = ZXDocErrorCode

_dll.ZXDoc_DB_AddDatabase.argtypes = [ZXDocHandle, ZXDoc_CharP, PZXDoc_Database]
_dll.ZXDoc_DB_AddDatabase.restype = ZXDocErrorCode

_dll.ZXDoc_DB_RemoveDatabase.argtypes = [ZXDocHandle, ZXDoc_CharP]
_dll.ZXDoc_DB_RemoveDatabase.restype = ZXDocErrorCode

_dll.ZXDoc_DB_GetDatabaseByName.argtypes = [ZXDocHandle, ZXDoc_CharP, PZXDoc_Database]
_dll.ZXDoc_DB_GetDatabaseByName.restype = ZXDocErrorCode

_dll.ZXDoc_DB_GetDatabaseById.argtypes = [ZXDocHandle, ZXDoc_CharP, PZXDoc_Database]
_dll.ZXDoc_DB_GetDatabaseById.restype = ZXDocErrorCode

_dll.ZXDoc_DB_GetDatabaseById.argtypes = [ZXDocHandle, ZXDoc_CharP, PZXDoc_Database]
_dll.ZXDoc_DB_GetDatabaseById.restype = ZXDocErrorCode

_dll.ZXDoc_DB_DbcGetMessages.argtypes = [
    ZXDocHandle,
    ZXDoc_CharP,
    ZXDoc_DBCMessageInfoPP,
]
_dll.ZXDoc_DB_DbcGetMessages.restype = ZXDocErrorCode

_dll.ZXDoc_DB_DbcGetMessageByName.argtypes = [
    ZXDocHandle,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ZXDoc_DBCMessageInfoP,
]
_dll.ZXDoc_DB_DbcGetMessageByName.restype = ZXDocErrorCode

_dll.ZXDoc_DB_DbcGetMessageById.argtypes = [
    ZXDocHandle,
    ZXDoc_CharP,
    ZXDoc_U32,
    ZXDoc_DBCMessageInfoP,
]
_dll.ZXDoc_DB_DbcGetMessageById.restype = ZXDocErrorCode

_dll.ZXDoc_DB_DbcGetSignalsByMsgId.argtypes = [
    ZXDocHandle,
    ZXDoc_CharP,
    ZXDoc_U32,
    ZXDoc_DBCSignalInfoPP,
]
_dll.ZXDoc_DB_DbcGetSignalsByMsgId.restype = ZXDocErrorCode

_dll.ZXDoc_DB_DbcGetSignalsByMsgName.argtypes = [
    ZXDocHandle,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ZXDoc_DBCSignalInfoPP,
]
_dll.ZXDoc_DB_DbcGetSignalsByMsgName.restype = ZXDocErrorCode

_dll.ZXDoc_DB_DbcGetSignalByMsgId.argtypes = [
    ZXDocHandle,
    ZXDoc_CharP,
    ZXDoc_U32,
    ZXDoc_CharP,
    ZXDoc_DBCSignalInfoP,
]
_dll.ZXDoc_DB_DbcGetSignalByMsgId.restype = ZXDocErrorCode

_dll.ZXDoc_DB_DbcGetSignalByMsgName.argtypes = [
    ZXDocHandle,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ZXDoc_CharP,
    ZXDoc_DBCSignalInfoP,
]
_dll.ZXDoc_DB_DbcGetSignalByMsgName.restype = ZXDocErrorCode

_dll.ZXDoc_DB_DbcMessageDecode.argtypes = [
    ZXDocHandle,
    ZXDoc_CharP,
    PZXDoc_DBCData,
    PZXDoc_DBCMessageValue,
]
_dll.ZXDoc_DB_DbcMessageDecode.restype = ZXDocErrorCode

# ZCliResponseHandle
_dll.ZXDoc_CliResp_GetReturnCode.argtypes = [ZCliResponseHandle]
_dll.ZXDoc_CliResp_GetReturnCode.restype = ZXDoc_I32

_dll.ZXDoc_CliResp_GetMessage.argtypes = [ZCliResponseHandle]
_dll.ZXDoc_CliResp_GetMessage.restype = ZXDoc_CharP

_dll.ZXDoc_CliResp_GetValue.argtypes = [
    ZCliResponseHandle,
    ZXDoc_U32,
    ctypes.POINTER(ZXDoc_CharP),
    ctypes.POINTER(ZXDoc_CharP),
]
_dll.ZXDoc_CliResp_GetValue.restype = ZXDoc_Bool

_dll.ZXDoc_CliResp_GetValueByKey.argtypes = [
    ZCliResponseHandle,
    ZXDoc_CharP,
    ctypes.POINTER(ZXDoc_CharP),
]
_dll.ZXDoc_CliResp_GetValueByKey.restype = ZXDoc_Bool

_dll.ZXDoc_CliResp_Free.argtypes = [ZCliResponseHandle]
_dll.ZXDoc_CliResp_Free.restype = None


# ZXDoc_MeasureDataRecorderCfg
ZXDoc_MeasureDataRecorderCfg = ZXDoc_VoidP

_dll.ZXDoc_MeasureDataRecorderCfg_New.argtypes = None
_dll.ZXDoc_MeasureDataRecorderCfg_New.restype = ZXDoc_MeasureDataRecorderCfg

_dll.ZXDoc_MeasureDataRecorderCfg_Free.argtypes = [ZXDoc_MeasureDataRecorderCfg]
_dll.ZXDoc_MeasureDataRecorderCfg_Free.restype = None

_dll.ZXDoc_MeasureDataRecorderCfg_SetRecorderName.argtypes = [
    ZXDoc_MeasureDataRecorderCfg,
    ZXDoc_CharP,
]
_dll.ZXDoc_MeasureDataRecorderCfg_SetRecorderName.restype = None

_dll.ZXDoc_MeasureDataRecorderCfg_SetFilePath.argtypes = [
    ZXDoc_MeasureDataRecorderCfg,
    ZXDoc_CharP,
]
_dll.ZXDoc_MeasureDataRecorderCfg_SetFilePath.restype = None

_dll.ZXDoc_MeasureDataRecorderCfg_SetMaxFileSize.argtypes = [
    ZXDoc_MeasureDataRecorderCfg,
    ZXDoc_U32,
]
_dll.ZXDoc_MeasureDataRecorderCfg_SetMaxFileSize.restype = None

_dll.ZXDoc_MeasureDataRecorderCfg_SetComment.argtypes = [
    ZXDoc_MeasureDataRecorderCfg,
    ZXDoc_CharP,
]
_dll.ZXDoc_MeasureDataRecorderCfg_SetComment.restype = None

_dll.ZXDoc_MeasureDataRecorderCfg_SetFileNameAutoAddTimeSuffix.argtypes = [
    ZXDoc_MeasureDataRecorderCfg,
    ZXDoc_Bool,
]
_dll.ZXDoc_MessageRecorderCfg_SetFileNameAutoAddTimeSuffix.restype = None

_dll.ZXDoc_MeasureDataRecorderCfg_SetMf4Compression.argtypes = [
    ZXDoc_MeasureDataRecorderCfg,
    ZXDoc_Bool,
]
_dll.ZXDoc_MessageRecorderCfg_SetMf4Compression.restype = None

_dll.ZXDoc_MeasureDataRecorderCfg_AddSignal.argtypes = [
    ZXDoc_MeasureDataRecorderCfg,
    PZXDoc_SignalIdentifier,
]
_dll.ZXDoc_MeasureDataRecorderCfg_AddSignal.restype = None


# ZXDoc_MessageRecorderCfg
ZXDoc_MessageRecorderCfg = ZXDoc_VoidP

_dll.ZXDoc_MessageRecorderCfg_New.argtypes = None
_dll.ZXDoc_MessageRecorderCfg_New.restype = ZXDoc_MessageRecorderCfg

_dll.ZXDoc_MessageRecorderCfg_Free.argtypes = [ZXDoc_MessageRecorderCfg]
_dll.ZXDoc_MessageRecorderCfg_Free.restype = None

_dll.ZXDoc_MessageRecorderCfg_SetRecorderName.argtypes = [
    ZXDoc_MessageRecorderCfg,
    ZXDoc_CharP,
]
_dll.ZXDoc_MessageRecorderCfg_SetRecorderName.restype = None

_dll.ZXDoc_MessageRecorderCfg_SetFilePath.argtypes = [
    ZXDoc_MessageRecorderCfg,
    ZXDoc_CharP,
]
_dll.ZXDoc_MessageRecorderCfg_SetFilePath.restype = None

_dll.ZXDoc_MessageRecorderCfg_SetMaxFileSize.argtypes = [
    ZXDoc_MessageRecorderCfg,
    ZXDoc_U32,
]
_dll.ZXDoc_MessageRecorderCfg_SetMaxFileSize.restype = None

_dll.ZXDoc_MessageRecorderCfg_SetComment.argtypes = [
    ZXDoc_MessageRecorderCfg,
    ZXDoc_CharP,
]
_dll.ZXDoc_MessageRecorderCfg_SetComment.restype = None

_dll.ZXDoc_MessageRecorderCfg_SetFileNameAutoAddTimeSuffix.argtypes = [
    ZXDoc_MessageRecorderCfg,
    ZXDoc_Bool,
]
_dll.ZXDoc_MessageRecorderCfg_SetFileNameAutoAddTimeSuffix.restype = None

_dll.ZXDoc_MessageRecorderCfg_SetMf4Compression.argtypes = [
    ZXDoc_MessageRecorderCfg,
    ZXDoc_Bool,
]
_dll.ZXDoc_MessageRecorderCfg_SetMf4Compression.restype = None

_dll.ZXDoc_MessageRecorderCfg_AddChannel.argtypes = [
    ZXDoc_MessageRecorderCfg,
    ZXDoc_BusType,
    ZXDoc_I32,
]
_dll.ZXDoc_MessageRecorderCfg_AddChannel.restype = None

# Data recorder

_dll.ZXDoc_RC_AddMeasureDataRecorder.argtypes = [
    ZCliResponseHandle,
    ZXDoc_MeasureDataRecorderCfg,
]
_dll.ZXDoc_RC_AddMeasureDataRecorder.restype = ZXDocErrorCode

_dll.ZXDoc_RC_AddMessageRecorder.argtypes = [
    ZCliResponseHandle,
    ZXDoc_MessageRecorderCfg,
]
_dll.ZXDoc_RC_AddMessageRecorder.restype = ZXDocErrorCode

_dll.ZXDoc_RC_RemoveDataRecorder.argtypes = [
    ZCliResponseHandle,
    ZXDoc_CharP,
]
_dll.ZXDoc_RC_RemoveDataRecorder.restype = ZXDocErrorCode

_dll.ZXDoc_RC_StartDataRecorder.argtypes = [
    ZCliResponseHandle,
    ZXDoc_CharP,
]
_dll.ZXDoc_RC_StartDataRecorder.restype = ZXDocErrorCode

_dll.ZXDoc_RC_StopDataRecorder.argtypes = [
    ZCliResponseHandle,
    ZXDoc_CharP,
]
_dll.ZXDoc_RC_StopDataRecorder.restype = ZXDocErrorCode


def ZXDoc_MemoryAlloc(size: int) -> ZXDoc_VoidP:
    return _dll.ZXDoc_MemoryAlloc(size)


def ZXDoc_MemoryFree(data: ZXDoc_VoidP):
    _dll.ZXDoc_MemoryFree(data)


def ZXDoc_StrClone(cstr: int) -> ZXDoc_CharP:
    return _dll.ZXDoc_StrClone(cstr)


def ZXDoc_StrFree(cstr: ZXDoc_CharP):
    _dll.ZXDoc_StrFree(cstr)


def ZXDoc_SignalVariant_ArrRowCount(variant) -> int:
    if ZXDOC_SGL_VALUE_TYPE_ARRAY != variant.type:
        return 0
    r = 0
    while bool(variant.data.arr[r]):
        r += 1
    return r


def ZXDoc_SignalVariant_ArrColCount(variant) -> int:
    if ZXDOC_SGL_VALUE_TYPE_ARRAY != variant.type:
        return 0

    if not bool(variant.data.arr[0]):
        return 0

    c = 0
    while bool(variant.data.arr[0][c]):
        c += 1
    return c


def ZXDoc_SignalVariant_ArrAt(variant, row, col) -> ZXDoc_SignalVariant:
    if ZXDOC_SGL_VALUE_TYPE_ARRAY != variant.type:
        return None
    return variant.data.arr[row][col]


########################
def ZXDoc_SignalVariant_New() -> PZXDoc_SignalVariant:
    return _dll.ZXDoc_SignalVariant_New()


def ZXDoc_SignalVariant_Init(v: PZXDoc_SignalVariant):
    return _dll.ZXDoc_SignalVariant_Init(v)


def ZXDoc_SignalVariant_InitArr(
    v: PZXDoc_SignalVariant, row: ZXDoc_I32, col: ZXDoc_I32
):
    return _dll.ZXDoc_SignalVariant_InitArr(v, row, col)


def ZXDoc_SignalVariant_ArrGetRows(v: PZXDoc_SignalVariant) -> ZXDoc_I32:
    return _dll.ZXDoc_SignalVariant_ArrGetRows(v)


def ZXDoc_SignalVariant_ArrGetCols(v: PZXDoc_SignalVariant) -> ZXDoc_I32:
    return _dll.ZXDoc_SignalVariant_ArrGetCols(v)


def ZXDoc_SignalVariant_ArrAt(
    v: PZXDoc_SignalVariant, row: ZXDoc_I32, col: ZXDoc_I32
) -> ZXDoc_I32:
    return _dll.ZXDoc_SignalVariant_ArrAt(v, row, col)


def ZXDoc_SignalVariant_ArrSet(
    v: PZXDoc_SignalVariant, row: ZXDoc_I32, col: ZXDoc_I32, item: PZXDoc_SignalVariant
) -> ZXDoc_I32:
    return _dll.ZXDoc_SignalVariant_ArrSet(v, row, col, item)


def ZXDoc_SignalVariant_SetInt64(v: PZXDoc_SignalVariant, val: ZXDoc_I64):
    return _dll.ZXDoc_SignalVariant_SetInt64(v, val)


def ZXDoc_SignalVariant_SetUint64(v: PZXDoc_SignalVariant, val: ZXDoc_U64):
    return _dll.ZXDoc_SignalVariant_SetUint64(v, val)


def ZXDoc_SignalVariant_SetDouble(v: PZXDoc_SignalVariant, val: ZXDoc_Double):
    return _dll.ZXDoc_SignalVariant_SetDouble(v, val)


def ZXDoc_SignalVariant_SetStr(v: PZXDoc_SignalVariant, val: ZXDoc_CharP):
    return _dll.ZXDoc_SignalVariant_SetStr(v, val)


def ZXDoc_SignalVariant_Clear(v: PZXDoc_SignalVariant):
    return _dll.ZXDoc_SignalVariant_Clear(v)


def ZXDoc_SignalVariant_Free(v: PZXDoc_SignalVariant):
    return _dll.ZXDoc_SignalVariant_Free(v)


def ZXDoc_SignalVariant_SetValue(variant, val) -> ZXDoc_SignalVariant:
    if ZXDOC_SGL_VALUE_TYPE_ARRAY != variant.type:
        return None
    if type(val) == int:
        if val > 0x7FFFFFFFFFFFFFFF:
            return self.set_signal_value_uint(
                sourceId, signalId, val, rowIndex, colIndex
            )
    else:
        return self.set_signal_value_int(sourceId, signalId, val, rowIndex, colIndex)

    if type(val) == float:
        return self.set_signal_value_double(sourceId, signalId, val, rowIndex, colIndex)

    if type(val) == str:
        return self.set_signal_value_str(sourceId, signalId, val, rowIndex, colIndex)
    return variant.data.arr[row][col]


def ZXDoc_SignalVariant_To_SimpleValue(val):
    if val is None or not bool(val):
        return None
    cval = val.contents

    if ZXDOC_SGL_VALUE_TYPE_U8 == cval.type:
        return cval.data.u8
    elif ZXDOC_SGL_VALUE_TYPE_U16 == cval.type:
        return cval.data.u16
    elif ZXDOC_SGL_VALUE_TYPE_U32 == cval.type:
        return cval.data.u32
    elif ZXDOC_SGL_VALUE_TYPE_U64 == cval.type:
        return cval.data.u64
    elif ZXDOC_SGL_VALUE_TYPE_I8 == cval.type:
        return cval.data.i8
    elif ZXDOC_SGL_VALUE_TYPE_I16 == cval.type:
        return cval.data.i16
    elif ZXDOC_SGL_VALUE_TYPE_I32 == cval.type:
        return cval.data.i32
    elif ZXDOC_SGL_VALUE_TYPE_I64 == cval.type:
        return cval.data.i64
    elif ZXDOC_SGL_VALUE_TYPE_FLOAT == cval.type:
        return cval.data.f
    elif ZXDOC_SGL_VALUE_TYPE_DOUBLE == cval.type:
        return cval.data.db
    elif ZXDOC_SGL_VALUE_TYPE_STR == cval.type:
        return cval.data.str.decode("utf-8")
    elif ZXDOC_SGL_VALUE_TYPE_ARRAY == cval.type:
        rowCount = ZXDoc_SignalVariant_ArrRowCount(cval)
        colCount = ZXDoc_SignalVariant_ArrColCount(cval)

        l = list()

        for r in range(rowCount):
            rl = list()
            for c in range(colCount):
                v = ZXDoc_SignalVariant_ArrAt(cval, r, c)
                simpleVal = ZXDoc_SignalVariant_To_SimpleValue(v)
                rl.append(simpleVal)
            l.append(rl)

        return l

    return None


def ZXDoc_SignalValue_New():
    return _dll.ZXDoc_SignalValue_New()


def ZXDoc_SignalValue_Free(val):
    _dll.ZXDoc_SignalValue_Free(val)


def ZXDoc_CANFDData_New():
    return _dll.ZXDoc_CANFDData_New()


def ZXDoc_LINData_New():
    return _dll.ZXDoc_LINData_New()


def ZXDoc_RawData_New_CANFDData():
    return _dll.ZXDoc_RawData_New_CANFDData()


def ZXDoc_RawData_New_LINData():
    return _dll.ZXDoc_RawData_New_LINData()


def ZXDoc_RawData_Get_CANFDData(pRawData) -> PZXDoc_CANFDData:
    return _dll.ZXDoc_RawData_Get_CANFDData(pRawData)


def ZXDoc_RawData_Get_LINData(pRawData) -> PZXDoc_LINData:
    return _dll.ZXDoc_RawData_Get_LINData(pRawData)


def ZXDoc_RawData_Free(pRawData):
    _dll.ZXDoc_RawData_Free(pRawData)


def ZXDoc_BuildTime() -> ZXDoc_CharP:
    return _dll.ZXDoc_BuildTime()


def ZXDoc_Version() -> ZXDoc_CharP:
    return _dll.ZXDoc_Version()


def ZXDoc_Create() -> ZXDocHandle:
    return _dll.ZXDoc_Create()


def ZXDoc_Free(handle: ZXDocHandle) -> ZXDocErrorCode:
    return _dll.ZXDoc_Free(handle)


def ZXDoc_SetServerName(handle: ZXDocHandle) -> ZXDocErrorCode:
    return _dll.ZXDoc_SetServerName(handle)


def ZXDoc_Connect(
    handle: ZXDocHandle,
    projectFilePath: ZXDoc_CharP = None,
    noTrayIcon: ZXDoc_Bool = ZXDoc_False,
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Connect(handle, projectFilePath, noTrayIcon)


def ZXDoc_Disonnect(handle: ZXDocHandle) -> ZXDocErrorCode:
    return _dll.ZXDoc_Disonnect(handle)


def ZXDoc_SetOnConnectedCallback(
    handle: ZXDocHandle, callback: ctypes.CFUNCTYPE(None), context: ZXDoc_VoidP
) -> ZXDocErrorCode:
    return _dll.ZXDoc_SetOnConnectedCallback(handle, callback, context)


def ZXDoc_SetOnDisconnectedCallback(
    handle: ZXDocHandle, callback: ctypes.CFUNCTYPE(None), context: ZXDoc_VoidP
) -> ZXDocErrorCode:
    return _dll.ZXDoc_SetOnDisconnectedCallback(handle, callback, context)


def ZXDoc_App_Log(
    handle: ZXDocHandle, lvl: ZXDoc_LogLevel, msg: ZXDoc_CharP
) -> ZXDocErrorCode:
    return _dll.ZXDoc_App_Log(handle, lvl, msg)


def ZXDoc_App_ClearLog(handle: ZXDocHandle) -> ZXDocErrorCode:
    return _dll.ZXDoc_App_ClearLog(handle)


def ZXDoc_Panel_Log(
    handle: ZXDocHandle, name: ZXDoc_CharP, lvl: ZXDoc_LogLevel, msg: ZXDoc_CharP
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Panel_Log(handle, name, lvl, msg)


def ZXDoc_App_ExportLog(
    handle: ZXDocHandle, logFilePath: ZXDoc_CharP
) -> ZXDocErrorCode:
    return _dll.ZXDoc_App_ExportLog(handle, logFilePath)


def ZXDoc_App_GetCurrentProjectPath(
    handle: ZXDocHandle, buf: ZXDoc_CharP, bufSize: ZXDoc_U32
) -> ZXDocErrorCode:
    return _dll.ZXDoc_App_GetCurrentProjectPath(handle, buf, bufSize)


def ZXDoc_App_GetVersion(
    handle: ZXDocHandle, buf: ZXDoc_CharP, bufSize: ZXDoc_U32
) -> ZXDocErrorCode:
    return _dll.ZXDoc_App_GetVersion(handle, buf, bufSize)


def ZXDoc_App_LoadProject(
    handle: ZXDocHandle, project_file: ZXDoc_CharP
) -> ZXDocErrorCode:
    return _dll.ZXDoc_App_LoadProject(handle, project_file)


def ZXDoc_App_SubwndCmdExec(
    handle: ZXDocHandle,
    subwindowIndex: ZXDoc_U32,
    cmdline: ZXDoc_CharP,
    waitForFinished: ZXDoc_Bool,
    response: ctypes.POINTER(ZCliResponseHandle),
) -> ZXDocErrorCode:
    return _dll.ZXDoc_App_SubwndCmdExec(
        handle, subwindowIndex, cmdline, waitForFinished, response
    )


def ZXDoc_App_AddUserVariable(
    handle: ZXDocHandle, var: PZXDoc_UserVariable
) -> ZXDocErrorCode:
    return _dll.ZXDoc_App_AddUserVariable(handle, var)


def ZXDoc_App_DelUserVariable(
    handle: ZXDocHandle, varName: ZXDoc_CharP, group: ZXDoc_CharP
) -> ZXDocErrorCode:
    return _dll.ZXDoc_App_DelUserVariable(handle, varName, group)


def ZXDoc_App_ShowMainWindow(handle: ZXDocHandle) -> ZXDocErrorCode:
    return _dll.ZXDoc_App_ShowMainWindow(handle)


def ZXDoc_App_HideMainWindow(handle: ZXDocHandle) -> ZXDocErrorCode:
    return _dll.ZXDoc_App_HideMainWindow(handle)


def ZXDoc_App_CloseMainWindow(handle: ZXDocHandle) -> ZXDocErrorCode:
    return _dll.ZXDoc_App_CloseMainWindow(handle)


def ZXDoc_Meas_Start(handle: ZXDocHandle) -> ZXDocErrorCode:
    return _dll.ZXDoc_Meas_Start(handle)


def ZXDoc_Meas_Stop(handle: ZXDocHandle) -> ZXDocErrorCode:
    return _dll.ZXDoc_Meas_Stop(handle)


def ZXDoc_Meas_IsStarted(handle: ZXDocHandle) -> bool:
    re = ZXDoc_U8(0)
    if ZXDOC_E_OK != _dll.ZXDoc_Meas_IsStarted(handle, ctypes.pointer(re)):
        return False
    return 0 != re.value


def ZXDoc_Meas_SetStatChangedCallback(
    handle: ZXDocHandle,
    callback: ZXDoc_Meas_StatChangedCallbackType,
    context: ZXDoc_VoidP,
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Meas_SetStatChangedCallback(handle, callback, context)


def ZXDoc_Signal_SetSignalsObserver(
    handle: ZXDocHandle,
    callback: ZXDoc_Signal_SetSignalsObserverCallbackType,
    context: ZXDoc_VoidP,
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Signal_SetSignalsObserver(handle, callback, context)


def ZXDoc_Signal_Subscribe(
    handle: ZXDocHandle, ids: PZXDoc_SignalIdentifier, count: ZXDoc_U32
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Signal_Subscribe(handle, ids, count)


def ZXDoc_Signal_Unsubscribe(
    handle: ZXDocHandle, ids: PZXDoc_SignalIdentifier, count: ZXDoc_U32
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Signal_Unsubscribe(handle, ids, count)


def ZXDoc_Signal_SetValueUint(
    handle: ZXDocHandle,
    sourceId: ZXDoc_CharP,
    signalId: ZXDoc_CharP,
    phyValue: ZXDoc_U64,
    rowIndex: ZXDoc_I32 = -1,
    colIndex: ZXDoc_I32 = -1,
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Signal_SetValueUint(
        handle, sourceId, signalId, phyValue, rowIndex, colIndex
    )


def ZXDoc_Signal_SetValues(
    handle: ZXDocHandle,
    phyValues: PPZXDoc_SignalValue,
    count: ZXDoc_I32 = -1,
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Signal_SetValues(handle, phyValues, count)


def ZXDoc_Signal_SetValueInt(
    handle: ZXDocHandle,
    sourceId: ZXDoc_CharP,
    signalId: ZXDoc_CharP,
    phyValue: ZXDoc_I64,
    rowIndex: ZXDoc_I32 = -1,
    colIndex: ZXDoc_I32 = -1,
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Signal_SetValueInt(
        handle, sourceId, signalId, phyValue, rowIndex, colIndex
    )


def ZXDoc_Signal_SetValueDouble(
    handle: ZXDocHandle,
    sourceId: ZXDoc_CharP,
    signalId: ZXDoc_CharP,
    phyValue: ZXDoc_Double,
    rowIndex: ZXDoc_I32 = -1,
    colIndex: ZXDoc_I32 = -1,
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Signal_SetValueDouble(
        handle, sourceId, signalId, phyValue, rowIndex, colIndex
    )


def ZXDoc_Signal_SetValueString(
    handle: ZXDocHandle,
    sourceId: ZXDoc_CharP,
    signalId: ZXDoc_CharP,
    phyValue: ZXDoc_CharP,
    rowIndex: ZXDoc_I32 = -1,
    colIndex: ZXDoc_I32 = -1,
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Signal_SetValueString(
        handle, sourceId, signalId, phyValue, rowIndex, colIndex
    )


def ZXDoc_Signal_GetValue(
    handle: ZXDocHandle,
    sourceId: ZXDoc_CharP,
    signalId: ZXDoc_CharP,
    phyValue: PPZXDoc_SignalValue,
    rowIndex: ZXDoc_I32 = -1,
    colIndex: ZXDoc_I32 = -1,
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Signal_GetValue(
        handle, sourceId, signalId, phyValue, rowIndex, colIndex
    )


def ZXDoc_Signal_GetInitValue(
    handle: ZXDocHandle,
    sourceId: ZXDoc_CharP,
    signalId: ZXDoc_CharP,
    phyValue: PPZXDoc_SignalValue,
    rowIndex: ZXDoc_I32 = -1,
    colIndex: ZXDoc_I32 = -1,
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Signal_GetInitValue(
        handle, sourceId, signalId, phyValue, rowIndex, colIndex
    )


def ZXDoc_Chnl_GetChannels(
    handle: ZXDocHandle,
    busType: ZXDoc_BusType,
    channels: ZXDoc_ChannelPP,
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Chnl_GetChannels(handle, busType, channels)


def ZXDoc_Channel_New() -> ZXDoc_Channel:
    return _dll.ZXDoc_Channel_New()


def ZXDoc_Channel_Free(chnl: ZXDoc_Channel) -> ZXDoc_Channel:
    return _dll.ZXDoc_Channel_Free(chnl)


def ZXDoc_Channels_Free(chnls: ZXDoc_ChannelP):
    return _dll.ZXDoc_Channels_Free(chnls)


def ZXDoc_Channel_GetBusType(chnl: ZXDoc_Channel):
    return _dll.ZXDoc_Channel_GetBusType(chnl)


def ZXDoc_Channel_GetLogicalIndex(chnl: ZXDoc_Channel):
    return _dll.ZXDoc_Channel_GetLogicalIndex(chnl)


def ZXDoc_Channel_GetPhysicalIndex(chnl: ZXDoc_Channel):
    return _dll.ZXDoc_Channel_GetPhysicalIndex(chnl)


def ZXDoc_Channel_IsEnabled(chnl: ZXDoc_Channel):
    return _dll.ZXDoc_Channel_IsEnabled(chnl)


def ZXDoc_Channel_IsActivated(chnl: ZXDoc_Channel):
    return _dll.ZXDoc_Channel_IsActivated(chnl)


def ZXDoc_Channel_GetDatabaseCount(chnl: ZXDoc_Channel):
    return _dll.ZXDoc_Channel_GetDatabaseCount(chnl)


def ZXDoc_Channel_GetDatabase(chnl: ZXDoc_Channel, index: ZXDoc_U32):
    return _dll.ZXDoc_Channel_GetDatabase(chnl, index)


def ZXDoc_Chnl_SetDataSinkCallback(
    handle: ZXDocHandle,
    callback: ZXDoc_Chnl_SetDataSinkCallbackType,
    context: ZXDoc_VoidP,
    filterMode: ZXDoc_FilterMode = ZXDoc_FilterMode_NoFilter,
    filters: PZXDoc_DataSinkFilter = None,
    filterCnt: ZXDoc_U32 = 0,
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Chnl_SetDataSinkCallback(
        handle, callback, context, filterMode, filters, filterCnt
    )


def ZXDoc_Chnl_Transmit(
    handle: ZXDocHandle, rawDatas: PPZXDoc_RawData, count: ZXDoc_U32P
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Chnl_Transmit(handle, rawDatas, count)


def ZXDoc_Chnl_GetDatabases(
    handle: ZXDocHandle,
    busType: ZXDoc_BusType,
    logicalIndex: ZXDoc_I32,
    dbIds: ZXDoc_CharPP,
    count: ZXDoc_U32P,
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Chnl_GetDatabases(handle, busType, logicalIndex, dbIds, count)


def ZXDoc_Chnl_SetDatabases(
    handle: ZXDocHandle,
    busType: ZXDoc_BusType,
    logicalIndex: ZXDoc_I32,
    dbIds: ZXDoc_CharPP,
    count: ZXDoc_U32,
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Chnl_SetDatabases(handle, busType, logicalIndex, dbIds, count)


def ZXDoc_Chnl_LinWakeUp(
    handle: ZXDocHandle, logicalIndex: ZXDoc_I32
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Chnl_LinWakeUp(handle, logicalIndex)


def ZXDoc_Chnl_GetAvailableCanTxQueueCount(
    handle: ZXDocHandle, logicalIndex: ZXDoc_I32, count: ZXDoc_I32P
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Chnl_GetAvailableCanTxQueueCount(handle, logicalIndex, count)


def ZXDoc_Chnl_CanQueueTransmit(
    handle: ZXDocHandle,
    rawDatas: PPZXDoc_RawData,
    count: ZXDoc_U32,
    result: ZXDoc_BoolP,
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Chnl_CanQueueTransmit(handle, rawDatas, count, result)


def ZXDoc_Chnl_ClearCanTxQueue(
    handle: ZXDocHandle, logicalIndex: ZXDoc_I32, result: ZXDoc_BoolP
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Chnl_ClearCanTxQueue(handle, logicalIndex, result)


def ZXDoc_Chnl_ScheduledFrameCapacity(
    handle: ZXDocHandle, logicalIndex: ZXDoc_I32, capacity: ZXDoc_I32P
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Chnl_ScheduledFrameCapacity(handle, logicalIndex, capacity)


def ZXDoc_Chnl_StartedScheduledFrameCount(
    handle: ZXDocHandle, logicalIndex: ZXDoc_I32, count: ZXDoc_I32P
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Chnl_StartedScheduledFrameCount(handle, logicalIndex, count)


def ZXDoc_Chnl_StartScheduledFrame(
    handle: ZXDocHandle,
    logicalIndex: ZXDoc_I32,
    data: PZXDoc_CANFDData,
    interval: ZXDoc_I32,
    index: ZXDoc_I32P,
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Chnl_StartScheduledFrame(
        handle, logicalIndex, data, interval, index
    )


def ZXDoc_Chnl_RestartScheduledFrame(
    handle: ZXDocHandle,
    logicalIndex: ZXDoc_I32,
    index: ZXDoc_I32P,
    data: PZXDoc_CANFDData,
    interval: ZXDoc_I32,
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Chnl_RestartScheduledFrame(
        handle, logicalIndex, index, data, interval
    )


def ZXDoc_Chnl_StopScheduledFrame(
    handle: ZXDocHandle, index: ZXDoc_I32
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Chnl_StopScheduledFrame(handle, index)


def ZXDoc_Cali_Connect(handle: ZXDocHandle) -> ZXDocErrorCode:
    return _dll.ZXDoc_Cali_Connect(handle)


def ZXDoc_Cali_Disonnect(handle: ZXDocHandle) -> ZXDocErrorCode:
    return _dll.ZXDoc_Cali_Disonnect(handle)


def ZXDoc_Cali_StartDataAcquisition(handle: ZXDocHandle) -> ZXDocErrorCode:
    return _dll.ZXDoc_Cali_StartDataAcquisition(handle)


def ZXDoc_Cali_StopDataAcquisition(handle: ZXDocHandle) -> ZXDocErrorCode:
    return _dll.ZXDoc_Cali_StopDataAcquisition(handle)


def ZXDoc_Cali_IsNeedCacheSyncAll(
    handle: ZXDocHandle, result: ctypes.POINTER(ZXDoc_Bool)
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Cali_IsNeedCacheSyncAll(handle, result)


def ZXDoc_Cali_CanSyncDownloadAll(
    handle: ZXDocHandle, result: ctypes.POINTER(ZXDoc_Bool)
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Cali_CanSyncDownloadAll(handle, result)


def ZXDoc_Cali_SyncUploadAll(handle: ZXDocHandle, asInit: ZXDoc_Bool) -> ZXDocErrorCode:
    return _dll.ZXDoc_Cali_SyncUploadAll(handle, asInit)


def ZXDoc_Cali_SyncDownloadAll(handle: ZXDocHandle) -> ZXDocErrorCode:
    return _dll.ZXDoc_Cali_SyncDownloadAll(handle)


def ZXDoc_Cali_SelectMemoryPageAll(
    handle: ZXDocHandle, type: ZXDoc_EcuMemPageType
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Cali_SelectMemoryPageAll(handle, type)


def ZXDoc_Cali_CanSelectMemoryPage(
    handle: ZXDocHandle, result: ctypes.POINTER(ZXDoc_Bool)
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Cali_CanSelectMemoryPage(handle, result)


def ZXDoc_Cali_GetCurrentMemoryPage(
    handle: ZXDocHandle, type: ctypes.POINTER(ZXDoc_EcuMemPageType)
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Cali_GetCurrentMemoryPage(handle, type)


def ZXDoc_Cali_AddSignalToMeasList(
    handle: ZXDocHandle,
    deviceId: ZXDoc_CharP,
    signalId: ZXDoc_CharP,
    measEvent: ZXDoc_MeasEventType,
    eventChannel: ZXDoc_U32,
    pollingPeriod: ZXDoc_U32,
    daqCyclic: ZXDoc_U32,
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Cali_AddSignalToMeasList(
        handle, deviceId, signalId, measEvent, eventChannel, pollingPeriod, daqCyclic
    )


def ZXDoc_Cali_RemoveSignalFromMeasList(
    handle: ZXDocHandle, deviceId: ZXDoc_CharP, signalId: ZXDoc_CharP
) -> ZXDocErrorCode:
    return _dll.ZXDoc_Cali_RemoveSignalFromMeasList(handle, deviceId, signalId)


def ZXDoc_E2E_GetCrcCalculator(
    handle: ZXDocHandle,
    calcHandle: ZXDoc_U64P,
    crcType: ZXDoc_E2ECrcType,
    parameters: PZXDoc_E2ECRCCalculatorParameters,
) -> ZXDocErrorCode:
    return _dll.ZXDoc_E2E_GetCrcCalculator(handle, calcHandle, crcType, parameters)


def ZXDoc_E2E_CrcCalculate(
    handle: ZXDocHandle,
    calcHandle: ZXDoc_U64,
    data: ZXDoc_VoidP,
    dataLength: ZXDoc_U32,
    result: ZXDoc_U64P,
    prevResult: ZXDoc_U64,
) -> ZXDocErrorCode:
    return _dll.ZXDoc_E2E_CrcCalculate(
        handle,
        calcHandle,
        data,
        dataLength,
        result,
        0 if prevResult is None else prevResult,
    )


def ZXDoc_Simu_StartCanSimulation(handle: ZXDocHandle):
    return _dll.ZXDoc_Simu_StartCanSimulation(handle)


def ZXDoc_Simu_StopCanSimulation(handle: ZXDocHandle):
    return _dll.ZXDoc_Simu_StopCanSimulation(handle)


def ZXDoc_Simu_ActiveCanSend(
    handle: ZXDocHandle,
    channelIndex: ZXDoc_I32,
    databaseId: ZXDoc_CharP,
    node: ZXDoc_CharP,
    message: ZXDoc_CharP,
    isActive: ZXDoc_Bool,
):
    return _dll.ZXDoc_Simu_ActiveCanSend(
        handle, channelIndex, databaseId, node, message, isActive
    )


def ZXDoc_Simu_SetCanSendType(
    handle: ZXDocHandle,
    channelIndex: ZXDoc_I32,
    databaseId: ZXDoc_CharP,
    node: ZXDoc_CharP,
    message: ZXDoc_CharP,
    type: ZXDoc_SimuSendType,
):
    return _dll.ZXDoc_Simu_SetCanSendType(
        handle, channelIndex, databaseId, node, message, type
    )


def ZXDoc_Simu_SetCanCycleTime(
    handle: ZXDocHandle,
    channelIndex: ZXDoc_I32,
    databaseId: ZXDoc_CharP,
    node: ZXDoc_CharP,
    message: ZXDoc_CharP,
    cycleTime: ZXDoc_U32,
):
    return _dll.ZXDoc_Simu_SetCanCycleTime(
        handle, channelIndex, databaseId, node, message, cycleTime
    )


def ZXDoc_Simu_SetCanSendRepetitions(
    handle: ZXDocHandle,
    channelIndex: ZXDoc_I32,
    databaseId: ZXDoc_CharP,
    node: ZXDoc_CharP,
    message: ZXDoc_CharP,
    repetitions: ZXDoc_I64,
):
    return _dll.ZXDoc_Simu_SetCanSendRepetitions(
        handle, channelIndex, databaseId, node, message, repetitions
    )


def ZXDoc_Simu_SetCanSignalValue(
    handle: ZXDocHandle,
    channelIndex: ZXDoc_I32,
    databaseId: ZXDoc_CharP,
    node: ZXDoc_CharP,
    message: ZXDoc_CharP,
    signal: ZXDoc_CharP,
    value: ZXDoc_Double,
):
    return _dll.ZXDoc_Simu_SetCanSignalValue(
        handle, channelIndex, databaseId, node, message, signal, value
    )


def ZXDoc_Simu_SetCanSignalValues(
    handle: ZXDocHandle,
    channelIndex: ZXDoc_I32,
    databaseId: ZXDoc_CharP,
    node: ZXDoc_CharP,
    message: ZXDoc_CharP,
    signals: ctypes.POINTER(ZXDoc_CharP),
    values: ZXDoc_DoubleP,
    count: ZXDoc_U32,
):
    return _dll.ZXDoc_Simu_SetCanSignalValues(
        handle, channelIndex, databaseId, node, message, signals, values, count
    )


def ZXDoc_Simu_SetCanHighPrecisionScheduled(handle: ZXDocHandle, enabled: ZXDoc_Bool):
    return _dll.ZXDoc_Simu_SetCanHighPrecisionScheduled(handle, enabled)


def ZXDoc_Simu_IsCanHighPrecisionScheduled(handle: ZXDocHandle, enabled: ZXDoc_BoolP):
    return _dll.ZXDoc_Simu_IsCanHighPrecisionScheduled(handle, enabled)


def ZXDoc_Simu_SetCanFdType(
    handle: ZXDocHandle,
    channelIndex: ZXDoc_I32,
    databaseId: ZXDoc_CharP,
    node: ZXDoc_CharP,
    message: ZXDoc_CharP,
    isCANFD: ZXDoc_Bool,
):
    return _dll.ZXDoc_Simu_SetCanFdType(
        handle, channelIndex, databaseId, node, message, isCANFD
    )


def ZXDoc_Simu_SetCanFdBrs(
    handle: ZXDocHandle,
    channelIndex: ZXDoc_I32,
    databaseId: ZXDoc_CharP,
    node: ZXDoc_CharP,
    message: ZXDoc_CharP,
    isBRS: ZXDoc_Bool,
):
    return _dll.ZXDoc_Simu_SetCanFdBrs(
        handle, channelIndex, databaseId, node, message, isBRS
    )


def ZXDoc_Dev_GetDevices(
    handle: ZXDocHandle,
    deviceType: ZXDoc_DeviceType,
    deviceInfos: PZXDoc_DeviceInfo,
    count: ZXDoc_U32P,
):
    return _dll.ZXDoc_Dev_GetDevices(handle, deviceType, deviceInfos, count)


def ZXDoc_Diag_CreateDoCANInterface(
    handle: ZXDocHandle,
    interfaceHandle: PZXDoc_UdsInterfaceHandle,
    cfg: PZXDoc_DoCANCfg,
):
    return _dll.ZXDoc_Diag_CreateDoCANInterface(handle, interfaceHandle, cfg)


def ZXDoc_Diag_CreateDoLINInterface(
    handle: ZXDocHandle,
    interfaceHandle: PZXDoc_UdsInterfaceHandle,
    cfg: PZXDoc_DoLINCfg,
):
    return _dll.ZXDoc_Diag_CreateDoLINInterface(handle, interfaceHandle, cfg)


def ZXDoc_Diag_CreateDoIPInterface(
    handle: ZXDocHandle,
    interfaceHandle: PZXDoc_UdsInterfaceHandle,
    cfg: PZXDoc_DoIPCfg,
):
    return _dll.ZXDoc_Diag_CreateDoIPInterface(handle, interfaceHandle, cfg)


def ZXDoc_Diag_FreeUdsInterface(
    handle: ZXDocHandle, interfaceHandle: ZXDoc_UdsInterfaceHandle
):
    return _dll.ZXDoc_Diag_FreeUdsInterface(handle, interfaceHandle)


def ZXDoc_Diag_UdsRequest(
    handle: ZXDocHandle,
    req: PZXDoc_UdsRequest,
    requestData: ZXDoc_VoidP,
    rsp: PZXDoc_UdsResponse,
):
    return _dll.ZXDoc_Diag_UdsRequest(handle, req, requestData, rsp)


def ZXDoc_Diag_UdsFunctionalRequest(
    handle: ZXDocHandle,
    functAddr: ZXDoc_U32,
    req: PZXDoc_UdsRequest,
    requestData: ZXDoc_VoidP,
    rsp: PZXDoc_UdsResponse,
):
    return _dll.ZXDoc_Diag_UdsFunctionalRequest(
        handle, functAddr, req, requestData, rsp
    )


def ZXDoc_Diag_CancelRequest(
    handle: ZXDocHandle, interfaceHandle: ZXDoc_UdsInterfaceHandle
):
    return _dll.ZXDoc_Diag_CancelRequest(handle, interfaceHandle)


def ZXDoc_Diag_GetErrorMssage(
    handle: ZXDocHandle,
    interfaceHandle: ZXDoc_UdsInterfaceHandle,
    rsp: PZXDoc_UdsResponse,
    str_buf: ZXDoc_CharP,
    buf_len: ZXDoc_U32,
):
    return _dll.ZXDoc_Diag_GetErrorMssage(
        handle, interfaceHandle, rsp, str_buf, buf_len
    )


def ZXDoc_Diag_SatrtSessionKeep(
    handle: ZXDocHandle,
    interfaceHandle: ZXDoc_UdsInterfaceHandle,
    address: ZXDoc_U32,
    interval: ZXDoc_U32,
    suppressResponse: ZXDoc_Bool,
    doNotSendWhenDataTrans: ZXDoc_Bool,
):
    return _dll.ZXDoc_Diag_SatrtSessionKeep(
        handle,
        interfaceHandle,
        address,
        interval,
        suppressResponse,
        doNotSendWhenDataTrans,
    )


def ZXDoc_Diag_StopSessionKeep(
    handle: ZXDocHandle, interfaceHandle: ZXDoc_UdsInterfaceHandle
):
    return _dll.ZXDoc_Diag_StopSessionKeep(handle, interfaceHandle)


def ZXDoc_Diag_CalcSecurityKey(
    handle: ZXDocHandle,
    dllPath: ZXDoc_CharP,
    securityLevel: ZXDoc_U8,
    seed: ZXDoc_CharP,
    seedSize: ZXDoc_U32,
    variant: ZXDoc_CharP,
    key: ZXDoc_CharP,
    keySize: ZXDoc_U32P,
):
    return _dll.ZXDoc_Diag_CalcSecurityKey(
        handle, dllPath, securityLevel, seed, seedSize, variant, key, keySize
    )


def ZXDoc_Diag_StartAutoFlowControl(
    handle: ZXDocHandle,
    interfaceHandle: ZXDoc_UdsInterfaceHandle,
    srcAddr: ZXDoc_U32,
    dstAddr: ZXDoc_U32,
):
    return _dll.ZXDoc_Diag_StartAutoFlowControl(
        handle, interfaceHandle, srcAddr, dstAddr
    )


def ZXDoc_Diag_StopAutoFlowControl(
    handle: ZXDocHandle, interfaceHandle: ZXDoc_UdsInterfaceHandle
):
    return _dll.ZXDoc_Diag_StopAutoFlowControl(handle, interfaceHandle)


def ZXDoc_FlashFileInfo_New() -> ZXDoc_FlashFileInfo:
    return _dll.ZXDoc_FlashFileInfo_New()


def ZXDoc_FlashFileInfo_Free(info: ZXDoc_FlashFileInfo):
    _dll.ZXDoc_FlashFileInfo_Free(info)


def ZXDoc_FlashFileInfo_GetFilePath(info: ZXDoc_FlashFileInfo) -> ZXDoc_CharP:
    return _dll.ZXDoc_FlashFileInfo_GetFilePath(info)


def ZXDoc_FlashFileInfo_GetType(info: ZXDoc_FlashFileInfo) -> ZXDoc_FalshFileType:
    return _dll.ZXDoc_FlashFileInfo_GetType(info)


def ZXDoc_FlashFileInfo_GetBlockCount(info: ZXDoc_FlashFileInfo) -> ZXDoc_U32:
    return _dll.ZXDoc_FlashFileInfo_GetBlockCount(info)


def ZXDoc_FlashFileInfo_GetBlock(
    info: ZXDoc_FlashFileInfo,
    index: ZXDoc_I32,
    address: ZXDoc_U64P,
    data: ctypes.POINTER(ZXDoc_UByteP),
    dataSize: ZXDoc_U32P,
) -> ZXDoc_Bool:
    return _dll.ZXDoc_FlashFileInfo_GetBlock(info, index, address, data, dataSize)


def ZXDoc_Diag_LoadFlashFileBlocks(
    handle: ZCliResponseHandle,
    flashFilePath: ZXDoc_CharP,
    info: ZXDoc_FlashFileInfo,
) -> ZXDoc_Bool:
    return _dll.ZXDoc_Diag_LoadFlashFileBlocks(handle, flashFilePath, info)


# Database


def ZXDoc_DBCSignalValue_New() -> PZXDoc_DBCSignalValue:
    return _dll.ZXDoc_DBCSignalValue_New()


def ZXDoc_DBCSignalValue_Free(sgl: PZXDoc_DBCSignalValue):
    return _dll.ZXDoc_DBCSignalValue_Free(sgl)


def ZXDoc_DBCSignalValue_SetName(sgl: PZXDoc_DBCSignalValue, name: ZXDoc_CharP):
    return _dll.ZXDoc_DBCSignalValue_SetName(sgl, name)


def ZXDoc_DBCSignalValue_SetPhyValue(sgl: PZXDoc_DBCSignalValue, value: ZXDoc_Double):
    return _dll.ZXDoc_DBCSignalValue_SetPhyValue(sgl, value)


def ZXDoc_DBCSignalValue_SetRawInt(sgl: PZXDoc_DBCSignalValue, value: ZXDoc_I64):
    return _dll.ZXDoc_DBCSignalValue_SetRawInt(sgl, value)


def ZXDoc_DBCSignalValue_SetRawUint(sgl: PZXDoc_DBCSignalValue, value: ZXDoc_U64):
    return _dll.ZXDoc_DBCSignalValue_SetRawUint(sgl, value)


def ZXDoc_DBCSignalValue_SetRawFloat(sgl: PZXDoc_DBCSignalValue, value: ZXDoc_Float):
    return _dll.ZXDoc_DBCSignalValue_SetRawFloat(sgl, value)


def ZXDoc_DBCSignalValue_SetRawDouble(sgl: PZXDoc_DBCSignalValue, value: ZXDoc_Double):
    return _dll.ZXDoc_DBCSignalValue_SetRawDouble(sgl, value)


def ZXDoc_DBCMsgValue_New(id: int = 0) -> PZXDoc_DBCMessageValue:
    msg = _dll.ZXDoc_DBCMsgValue_New()
    msg.contents.id = id
    return msg


def ZXDoc_DBCMsgValue_Free(msg: PZXDoc_DBCMessageValue):
    return _dll.ZXDoc_DBCMsgValue_Free(msg)


def ZXDoc_DBCMsgValue_AppendSignal(
    msg: PZXDoc_DBCMessageValue, sgl: PZXDoc_DBCSignalValue
):
    return _dll.ZXDoc_DBCMsgValue_AppendSignal(msg, sgl)


def ZXDoc_DBCMsgValue_SignalCount(msg: PZXDoc_DBCMessageValue):
    return _dll.ZXDoc_DBCMsgValue_SignalCount(msg, sgl)


def ZXDoc_DBCMsgValue_SignalAt(
    msg: PZXDoc_DBCMessageValue, index: int
) -> PZXDoc_DBCSignalValue:
    return _dll.ZXDoc_DBCMsgValue_SignalAt(msg, index)


def ZXDoc_DBCMsgValue_GetSignal(
    msg: PZXDoc_DBCMessageValue, name: ZXDoc_CharP
) -> PZXDoc_DBCSignalValue:
    return _dll.ZXDoc_DBCMsgValue_GetSignal(msg, name)


def ZXDoc_DB_GetDatabases(
    handle: ZXDocHandle, databases: PZXDoc_Database, count: ZXDoc_U32P
):
    return _dll.ZXDoc_DB_GetDatabases(handle, databases, count)


def ZXDoc_DB_AddDatabase(
    handle: ZXDocHandle, filePath: ZXDoc_CharP, database: PZXDoc_Database
):
    return _dll.ZXDoc_DB_AddDatabase(handle, filePath, database)


def ZXDoc_DB_RemoveDatabase(handle: ZXDocHandle, dbId: ZXDoc_CharP):
    return _dll.ZXDoc_DB_RemoveDatabase(handle, dbId)


def ZXDoc_DB_GetDatabaseByName(
    handle: ZXDocHandle, name: ZXDoc_CharP, database: PZXDoc_Database
):
    return _dll.ZXDoc_DB_GetDatabaseByName(handle, name, database)


def ZXDoc_DB_GetDatabaseById(
    handle: ZXDocHandle, dbId: ZXDoc_CharP, database: PZXDoc_Database
):
    return _dll.ZXDoc_DB_GetDatabaseById(handle, dbId, database)


def ZXDoc_DB_DbcGetMessages(
    handle: ZXDocHandle, dbId: ZXDoc_CharP, msgs: ZXDoc_DBCMessageInfoPP
):
    return _dll.ZXDoc_DB_DbcGetMessages(handle, dbId, msgs)


def ZXDoc_DB_DbcGetMessageByName(
    handle: ZXDocHandle,
    dbId: ZXDoc_CharP,
    node: ZXDoc_CharP,
    msgName: ZXDoc_CharP,
    msg: ZXDoc_DBCMessageInfoP,
):
    return _dll.ZXDoc_DB_DbcGetMessageByName(handle, dbId, node, msgName, msg)


def ZXDoc_DB_DbcGetMessageById(
    handle: ZXDocHandle,
    dbId: ZXDoc_CharP,
    id: ZXDoc_U32,
    msg: ZXDoc_DBCMessageInfoP,
):
    return _dll.ZXDoc_DB_DbcGetMessageById(handle, dbId, id, msg)


def ZXDoc_DB_DbcGetSignalsByMsgId(
    handle: ZXDocHandle, dbId: ZXDoc_CharP, msgId: int, sgls: ZXDoc_DBCSignalInfoPP
):
    return _dll.ZXDoc_DB_DbcGetSignalsByMsgId(handle, dbId, msgId, sgls)


def ZXDoc_DB_DbcGetSignalsByMsgName(
    handle: ZXDocHandle,
    dbId: ZXDoc_CharP,
    node: ZXDoc_CharP,
    msgName: ZXDoc_CharP,
    sgls: ZXDoc_DBCSignalInfoPP,
):
    return _dll.ZXDoc_DB_DbcGetSignalsByMsgName(handle, dbId, node, msgName, sgls)


def ZXDoc_DB_DbcGetSignalByMsgId(
    handle: ZXDocHandle,
    dbId: ZXDoc_CharP,
    msgId: int,
    sglName: ZXDoc_CharP,
    sgl: ZXDoc_DBCSignalInfoP,
):
    return _dll.ZXDoc_DB_DbcGetSignalByMsgId(handle, dbId, msgId, sglName, sgl)


def ZXDoc_DB_DbcGetSignalByMsgName(
    handle: ZXDocHandle,
    dbId: ZXDoc_CharP,
    node: ZXDoc_CharP,
    msgName: ZXDoc_CharP,
    sglName: ZXDoc_CharP,
    sgl: ZXDoc_DBCSignalInfoP,
):
    return _dll.ZXDoc_DB_DbcGetSignalByMsgName(
        handle, dbId, node, msgName, sglName, sgl
    )


def ZXDoc_DB_DbcMessageEncode(
    handle: ZXDocHandle,
    dbId: ZXDoc_CharP,
    msg: PZXDoc_DBCMessageValue,
    dbcData: PZXDoc_DBCData,
    encodeObj: ZXDoc_DBCEncodeObject,
):
    return _dll.ZXDoc_DB_DbcMessageEncode(handle, dbId, msg, dbcData, encodeObj)


def ZXDoc_DB_DbcMessageDecode(
    handle: ZXDocHandle,
    dbId: ZXDoc_CharP,
    dbcData: PZXDoc_DBCData,
    msg: PZXDoc_DBCMessageValue,
):
    return _dll.ZXDoc_DB_DbcMessageDecode(handle, dbId, dbcData, msg)


def ZXDoc_CliResp_GetReturnCode(handle: ZCliResponseHandle):
    return _dll.ZXDoc_CliResp_GetReturnCode(handle)


def ZXDoc_CliResp_GetMessage(handle: ZCliResponseHandle):
    return _dll.ZXDoc_CliResp_GetMessage(handle)


def ZXDoc_CliResp_GetValue(
    handle: ZCliResponseHandle,
    index: ZXDoc_U32,
    key: ctypes.POINTER(ZXDoc_CharP),
    value: ctypes.POINTER(ZXDoc_CharP),
):
    return _dll.ZXDoc_CliResp_GetValue(handle, index, key, value)


def ZXDoc_CliResp_GetValueByKey(
    handle: ZCliResponseHandle,
    key: ZXDoc_CharP,
    value: ctypes.POINTER(ZXDoc_CharP),
):
    return _dll.ZXDoc_CliResp_GetValueByKey(handle, key, value)


def ZXDoc_CliResp_Free(handle: ZCliResponseHandle):
    return _dll.ZXDoc_CliResp_Free(handle)


def ZXDoc_MeasureDataRecorderCfg_New() -> ZXDoc_MeasureDataRecorderCfg:
    return _dll.ZXDoc_MeasureDataRecorderCfg_New()


def ZXDoc_MeasureDataRecorderCfg_Free(cfg: ZXDoc_MeasureDataRecorderCfg):
    _dll.ZXDoc_MeasureDataRecorderCfg_Free(cfg)


def ZXDoc_MeasureDataRecorderCfg_SetRecorderName(
    cfg: ZXDoc_MeasureDataRecorderCfg, recorderName: ZXDoc_CharP
):
    return _dll.ZXDoc_MeasureDataRecorderCfg_SetRecorderName(cfg, recorderName)


def ZXDoc_MeasureDataRecorderCfg_SetFilePath(
    cfg: ZXDoc_MeasureDataRecorderCfg, filePath: ZXDoc_CharP
):
    return _dll.ZXDoc_MeasureDataRecorderCfg_SetFilePath(cfg, filePath)


def ZXDoc_MeasureDataRecorderCfg_SetMaxFileSize(
    cfg: ZXDoc_MeasureDataRecorderCfg, maxFileSize: ZXDoc_U32
):
    return _dll.ZXDoc_MeasureDataRecorderCfg_SetMaxFileSize(cfg, maxFileSize)


def ZXDoc_MeasureDataRecorderCfg_SetComment(
    cfg: ZXDoc_MeasureDataRecorderCfg, comment: ZXDoc_CharP
):
    return _dll.ZXDoc_MeasureDataRecorderCfg_SetComment(cfg, comment)


def ZXDoc_MeasureDataRecorderCfg_SetFileNameAutoAddTimeSuffix(
    cfg: ZXDoc_MeasureDataRecorderCfg, fileNameAutoAddTimeSuffix: ZXDoc_Bool
):
    return _dll.ZXDoc_MeasureDataRecorderCfg_SetFileNameAutoAddTimeSuffix(
        cfg, fileNameAutoAddTimeSuffix
    )


def ZXDoc_MeasureDataRecorderCfg_SetMf4Compression(
    cfg: ZXDoc_MeasureDataRecorderCfg, mf4Compression: ZXDoc_Bool
):
    return _dll.ZXDoc_MeasureDataRecorderCfg_SetMf4Compression(cfg, mf4Compression)


def ZXDoc_MeasureDataRecorderCfg_AddSignal(
    cfg: ZXDoc_MeasureDataRecorderCfg, sglIdentifier: PZXDoc_SignalIdentifier
):
    return _dll.ZXDoc_MeasureDataRecorderCfg_AddSignal(cfg, sglIdentifier)


##################


def ZXDoc_MessageRecorderCfg_New() -> ZXDoc_MessageRecorderCfg:
    return _dll.ZXDoc_MessageRecorderCfg_New()


def ZXDoc_MessageRecorderCfg_Free(cfg: ZXDoc_MessageRecorderCfg):
    _dll.ZXDoc_MessageRecorderCfg_Free(cfg)


def ZXDoc_MessageRecorderCfg_SetRecorderName(
    cfg: ZXDoc_MessageRecorderCfg, recorderName: ZXDoc_CharP
):
    return _dll.ZXDoc_MessageRecorderCfg_SetRecorderName(cfg, recorderName)


def ZXDoc_MessageRecorderCfg_SetFilePath(
    cfg: ZXDoc_MessageRecorderCfg, filePath: ZXDoc_CharP
):
    return _dll.ZXDoc_MessageRecorderCfg_SetFilePath(cfg, filePath)


def ZXDoc_MessageRecorderCfg_SetMaxFileSize(
    cfg: ZXDoc_MessageRecorderCfg, maxFileSize: ZXDoc_U32
):
    return _dll.ZXDoc_MessageRecorderCfg_SetMaxFileSize(cfg, maxFileSize)


def ZXDoc_MessageRecorderCfg_SetComment(
    cfg: ZXDoc_MessageRecorderCfg, comment: ZXDoc_CharP
):
    return _dll.ZXDoc_MessageRecorderCfg_SetComment(cfg, comment)


def ZXDoc_MessageRecorderCfg_SetFileNameAutoAddTimeSuffix(
    cfg: ZXDoc_MessageRecorderCfg, fileNameAutoAddTimeSuffix: ZXDoc_Bool
):
    return _dll.ZXDoc_MessageRecorderCfg_SetFileNameAutoAddTimeSuffix(
        cfg, fileNameAutoAddTimeSuffix
    )


def ZXDoc_MessageRecorderCfg_SetMf4Compression(
    cfg: ZXDoc_MessageRecorderCfg, mf4Compression: ZXDoc_Bool
):
    return _dll.ZXDoc_MessageRecorderCfg_SetMf4Compression(cfg, mf4Compression)


def ZXDoc_MessageRecorderCfg_AddChannel(
    cfg: ZXDoc_MessageRecorderCfg, bysType: ZXDoc_BusType, logicalIndex: ZXDoc_I32
):
    return _dll.ZXDoc_MessageRecorderCfg_AddChannel(cfg, bysType, logicalIndex)


def ZXDoc_RC_AddMeasureDataRecorder(
    handle: ZCliResponseHandle, cfg: ZXDoc_MeasureDataRecorderCfg
):
    return _dll.ZXDoc_RC_AddMeasureDataRecorder(handle, cfg)


def ZXDoc_RC_AddMessageRecorder(
    handle: ZCliResponseHandle, cfg: ZXDoc_MessageRecorderCfg
):
    return _dll.ZXDoc_RC_AddMessageRecorder(handle, cfg)


def ZXDoc_RC_RemoveDataRecorder(handle: ZCliResponseHandle, recorderName: ZXDoc_CharP):
    return _dll.ZXDoc_RC_RemoveDataRecorder(handle, recorderName)


def ZXDoc_RC_StartDataRecorder(handle: ZCliResponseHandle, recorderName: ZXDoc_CharP):
    return _dll.ZXDoc_RC_StartDataRecorder(handle, recorderName)


def ZXDoc_RC_StopDataRecorder(handle: ZCliResponseHandle, recorderName: ZXDoc_CharP):
    return _dll.ZXDoc_RC_StopDataRecorder(handle, recorderName)


# ZXDoc_DBCSignalInfo


def ZXDoc_DBCSignalInfo_New() -> ZXDoc_DBCSignalInfo:
    return _dll.ZXDoc_DBCSignalInfo_New(info)


def ZXDoc_DBCSignalInfo_Free(info: ZXDoc_DBCSignalInfo):
    _dll.ZXDoc_DBCSignalInfo_Free(info)


def ZXDoc_DBCSignalInfos_Free(infos: ZXDoc_DBCSignalInfoP):
    _dll.ZXDoc_DBCSignalInfos_Free(infos)


def ZXDoc_DBCSignalInfo_GetName(info: ZXDoc_DBCSignalInfo):
    return _dll.ZXDoc_DBCSignalInfo_GetName(info)


def ZXDoc_DBCSignalInfo_GetComment(info: ZXDoc_DBCSignalInfo):
    return _dll.ZXDoc_DBCSignalInfo_GetComment(info)


def ZXDoc_DBCSignalInfo_GetMultiplexerIndicator(info: ZXDoc_DBCSignalInfo):
    return _dll.ZXDoc_DBCSignalInfo_GetMultiplexerIndicator(info)


def ZXDoc_DBCSignalInfo_GetMultiplexerValue(info: ZXDoc_DBCSignalInfo):
    return _dll.ZXDoc_DBCSignalInfo_GetMultiplexerValue(info)


def ZXDoc_DBCSignalInfo_GetStartBit(info: ZXDoc_DBCSignalInfo):
    return _dll.ZXDoc_DBCSignalInfo_GetStartBit(info)


def ZXDoc_DBCSignalInfo_GetLength(info: ZXDoc_DBCSignalInfo):
    return _dll.ZXDoc_DBCSignalInfo_GetLength(info)


def ZXDoc_DBCSignalInfo_IsIntel(info: ZXDoc_DBCSignalInfo):
    return _dll.ZXDoc_DBCSignalInfo_IsIntel(info)


def ZXDoc_DBCSignalInfo_IsSigned(info: ZXDoc_DBCSignalInfo):
    return _dll.ZXDoc_DBCSignalInfo_IsSigned(info)


def ZXDoc_DBCSignalInfo_GetFactor(info: ZXDoc_DBCSignalInfo):
    return _dll.ZXDoc_DBCSignalInfo_GetFactor(info)


def ZXDoc_DBCSignalInfo_GetOffset(info: ZXDoc_DBCSignalInfo):
    return _dll.ZXDoc_DBCSignalInfo_GetOffset(info)


def ZXDoc_DBCSignalInfo_GetMinValue(info: ZXDoc_DBCSignalInfo):
    return _dll.ZXDoc_DBCSignalInfo_GetMinValue(info)


def ZXDoc_DBCSignalInfo_GetMaxValue(info: ZXDoc_DBCSignalInfo):
    return _dll.ZXDoc_DBCSignalInfo_GetMaxValue(info)


def ZXDoc_DBCSignalInfo_GetUnit(info: ZXDoc_DBCSignalInfo):
    return _dll.ZXDoc_DBCSignalInfo_GetUnit(info)


def ZXDoc_DBCSignalInfo_GetValueType(info: ZXDoc_DBCSignalInfo):
    return _dll.ZXDoc_DBCSignalInfo_GetValueType(info)


def ZXDoc_DBCSignalInfo_GetReceiverCount(info: ZXDoc_DBCSignalInfo):
    return _dll.ZXDoc_DBCSignalInfo_GetReceiverCount(info)


def ZXDoc_DBCSignalInfo_GetReceiver(info: ZXDoc_DBCSignalInfo, index: ZXDoc_U32):
    return _dll.ZXDoc_DBCSignalInfo_GetReceiver(info, index)


def ZXDoc_DBCSignalInfo_GetValueTableSize(info: ZXDoc_DBCSignalInfo):
    return _dll.ZXDoc_DBCSignalInfo_GetValueTableSize(info)


def ZXDoc_DBCSignalInfo_GetValueTable(
    info: ZXDoc_DBCSignalInfo, index: ZXDoc_U32, key: ZXDoc_I64P, value: ZXDoc_CharPP
):
    return _dll.ZXDoc_DBCSignalInfo_GetValueTable(info, index, key, value)


def ZXDoc_DBCSignalInfo_GetAttrCount(info: ZXDoc_DBCSignalInfo):
    return _dll.ZXDoc_DBCSignalInfo_GetAttrCount(info)


def ZXDoc_DBCSignalInfo_GetAttr(
    info: ZXDoc_DBCSignalInfo, index: ZXDoc_U32, key: ZXDoc_I64P, value: ZXDoc_CharPP
):
    return _dll.ZXDoc_DBCSignalInfo_GetAttr(info, index, key, value)


# ZXDoc_DBCMessageInfo


def ZXDoc_DBCMessageInfo_New() -> ZXDoc_DBCMessageInfo:
    return _dll.ZXDoc_DBCMessageInfo_New(info)


def ZXDoc_DBCMessageInfo_Free(info: ZXDoc_DBCMessageInfo):
    _dll.ZXDoc_DBCMessageInfo_Free(info)


def ZXDoc_DBCMessageInfos_Free(infos: ZXDoc_DBCMessageInfoP):
    _dll.ZXDoc_DBCMessageInfos_Free(infos)


def ZXDoc_DBCMessageInfo_GetDatabaseId(info: ZXDoc_DBCMessageInfo):
    return _dll.ZXDoc_DBCMessageInfo_GetDatabaseId(info)


def ZXDoc_DBCMessageInfo_GetId(info: ZXDoc_DBCMessageInfo):
    return _dll.ZXDoc_DBCMessageInfo_GetId(info)


def ZXDoc_DBCMessageInfo_GetLen(info: ZXDoc_DBCMessageInfo):
    return _dll.ZXDoc_DBCMessageInfo_GetLen(info)


def ZXDoc_DBCMessageInfo_GetName(info: ZXDoc_DBCMessageInfo):
    return _dll.ZXDoc_DBCMessageInfo_GetName(info)


def ZXDoc_DBCMessageInfo_GetComment(info: ZXDoc_DBCMessageInfo):
    return _dll.ZXDoc_DBCMessageInfo_GetComment(info)


def ZXDoc_DBCMessageInfo_GetBusType(info: ZXDoc_DBCMessageInfo):
    return _dll.ZXDoc_DBCMessageInfo_GetBusType(info)


def ZXDoc_DBCMessageInfo_IsJ1939Frame(info: ZXDoc_DBCMessageInfo):
    return _dll.ZXDoc_DBCMessageInfo_IsJ1939Frame(info)


def ZXDoc_DBCMessageInfo_IsBRS(info: ZXDoc_DBCMessageInfo):
    return _dll.ZXDoc_DBCMessageInfo_IsBRS(info)


def ZXDoc_DBCMessageInfo_GetTransmitter(info: ZXDoc_DBCMessageInfo):
    return _dll.ZXDoc_DBCMessageInfo_GetTransmitter(info)


def ZXDoc_DBCMessageInfo_GetMultiplexer(info: ZXDoc_DBCMessageInfo):
    return _dll.ZXDoc_DBCMessageInfo_GetMultiplexer(info)


def ZXDoc_DBCMessageInfo_GetSignalNameCount(info: ZXDoc_DBCMessageInfo):
    return _dll.ZXDoc_DBCMessageInfo_GetSignalNameCount(info)


def ZXDoc_DBCMessageInfo_GetSignalName(info: ZXDoc_DBCMessageInfo, index: ZXDoc_U32):
    return _dll.ZXDoc_DBCMessageInfo_GetSignalName(info, index)


def ZXDoc_DBCMessageInfo_GetSignalGroupCount(info: ZXDoc_DBCMessageInfo):
    return _dll.ZXDoc_DBCMessageInfo_GetSignalGroupCount(info)


def ZXDoc_DBCMessageInfo_GetSignalGroupName(
    info: ZXDoc_DBCMessageInfo, index: ZXDoc_U32
):
    return _dll.ZXDoc_DBCMessageInfo_GetSignalGroupName(info, index)


def ZXDoc_DBCMessageInfo_GetSignalGroupSignalCount(
    info: ZXDoc_DBCMessageInfo, index: ZXDoc_U32
):
    return _dll.ZXDoc_DBCMessageInfo_GetSignalGroupSignalCount(info, index)


def ZXDoc_DBCMessageInfo_GetSignalGroupSglName(
    info: ZXDoc_DBCMessageInfo, index: ZXDoc_U32, sglIndex: ZXDoc_U32
):
    return _dll.ZXDoc_DBCMessageInfo_GetSignalGroupSglName(info, index, sglIndex)


def ZXDoc_DBCMessageInfo_GetAttrCount(info: ZXDoc_DBCMessageInfo):
    return _dll.ZXDoc_DBCMessageInfo_GetAttrCount(info)


def ZXDoc_DBCMessageInfo_GetAttr(
    info: ZXDoc_DBCMessageInfo, index: ZXDoc_U32, key: ZXDoc_CharPP, value: ZXDoc_CharPP
):
    return _dll.ZXDoc_DBCMessageInfo_GetAttr(info, index, key, value)
