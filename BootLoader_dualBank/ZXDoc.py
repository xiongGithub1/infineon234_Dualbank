# -*- coding: utf-8 -*-

import ctypes
from ctypes import Structure
from enum import IntEnum, unique
from dataclasses import dataclass, field
from typing import *
import platform
import time
import struct
import ZXDocCommApi as ZXDocComm

STR_ENCODING = "utf-8"


class ZUtility:
    @classmethod
    def MinLengthSize(cls, n: int):
        size = 0
        while n > 0:
            n = n >> 8
            size += 1
        return size if size > 0 else 1


@unique
class ZErrorCode(IntEnum):
    OK = 0
    FAILED = 1
    INVALID_HANDLE = 2
    NULL_POINTER = 3
    INVALID_DATA_TYPE = 4
    INVALID_PARAM = 5
    BUFFER_IS_TO_SMALL = 6

    def __bool__(self):
        return ZErrorCode.OK == self.value


@unique
class ZBusType(IntEnum):
    Unknown = 0
    CAN = 1
    LIN = 2
    Ethernet = 3


@dataclass
class ZChannel:
    busType: ZBusType = ZBusType.Unknown
    logicalIndex: int = 0
    physicalIndex: int = 0
    enabled: bool = False
    activated: bool = False
    databases: List[str] = field(default_factory=list)


@unique
class ZTransmitDirection(IntEnum):
    Rx = 0
    Tx = 1


@unique
class TxDelayUnitType(IntEnum):
    """
    队列发送延时的单位
    """

    NoDelay = 0  # 无发送延时
    MS = 1  # 发送延时单位ms
    US100 = 2  # 发送延时单位100us


@dataclass
class ZCANFDData:
    can_id: int
    EFF: bool = False
    FDF: bool = False
    RTR: bool = False
    BRS: bool = False
    ESI: bool = False
    direction: ZTransmitDirection = ZTransmitDirection.Rx
    transmitType: int = 0
    tx_delay_unit_type: TxDelayUnitType = TxDelayUnitType.NoDelay  # 队列发送延时的单位
    tx_delay: int = 0  # 队列发送延时，仅队列发送时有效，单位由 tx_delay_unit_type 指定
    data: bytes = field(default_factory=bytes)


@dataclass
class ZCANFDErrorData:
    errType: int = 0
    errSubType: int = 0
    nodeState: int = 0
    rxErrCount: int = 0
    txErrCount: int = 0
    errData: int = 0


@dataclass
class ZGPSData:
    @dataclass
    class Time:
        year: int = 0
        mon: int = 0
        day: int = 0
        hour: int = 0
        min: int = 0
        sec: int = 0
        milsec: int = 0

    time: Time = field(default_factory=Time)  # 时间
    latitude: float = 0  # 纬度 正数表示北纬, 负数表示南纬
    longitude: float = 0  # 经度 正数表示东经, 负数表示西经
    altitude: float = 0  # 海拔 单位: 米
    speed: float = 0  # 速度 单位: km/h
    courseAngle: float = 0  # 航向角


@unique
class ZLINFrameType(IntEnum):
    """
    LIN帧类型
    """

    HeaderAndResponse = 0  # 发送头部发响应
    HeaderOnly = 1  # 仅发送头部
    ResponseOnly = 2  # 仅发送响应


@unique
class ZLINChecksumType(IntEnum):
    """
    LIN的校验和类型
    """

    Classic = 1  # 经典
    Enhanced = 2  # 增强


@dataclass
class ZLINData:
    ID: int = 0
    timestamp: int = 0
    direction: ZTransmitDirection = ZTransmitDirection.Rx
    chksum: int = 0
    chksum_type: ZLINChecksumType = ZLINChecksumType.Classic
    frameType: ZLINFrameType = ZLINFrameType.HeaderAndResponse
    data: bytes = field(default_factory=bytes)


@dataclass
class ZLINErrorData:
    ID: int = 0
    direction: int = 0
    errorState: int = 0
    errorReason: int = 0
    checksum: int = 0
    data: bytes = field(default_factory=bytes)


@unique
class ZLINEventType(IntEnum):
    Unknown = 0
    WakeUp = 1


@dataclass
class ZLINEventData:
    type: ZLINEventType = ZLINEventType.Unknown


@dataclass
class ZBusUsageData:
    duration: int = 0
    busUsage: int = 0
    frameCount: int = 0


@unique
class ZEthernetType(IntEnum):
    IP = 0x0800
    ARP = 0x0806
    ETHBRIDGE = 0x6558
    REVARP = 0x8035
    AT = 0x809B
    AARP = 0x80F3
    VLAN = 0x8100
    IPX = 0x8137
    IPV6 = 0x86DD
    LOOPBACK = 0x9000
    PPPOED = 0x8863
    PPPOES = 0x8864
    MPLS = 0x8847
    PPP = 0x880B
    ROCEV1 = 0x8915
    IEEE_802_1AD = 0x88A8
    WAKE_ON_LAN = 0x0842


@unique
class ZEthernetFamily(IntEnum):
    INET = 2
    NS = 6
    ISO = 7
    APPLETALK = 16
    IPX = 23
    INET6_BSD = 24
    INET6_FREEBSD = 28
    INET6_DARWIN = 30


@dataclass
class ZEthernetDataBase:
    data: bytes = field(default_factory=bytes)


@dataclass
class ZEthernetLoopbackData(ZEthernetDataBase):
    family: ZEthernetFamily = ZEthernetFamily.INET


@dataclass
class ZEthernetData(ZEthernetDataBase):
    dstMac: bytes = field(default_factory=bytes)
    srcMac: bytes = field(default_factory=bytes)
    etherType: ZEthernetType = ZEthernetType.IP


@dataclass
class ZEthernetDot3Data(ZEthernetData):
    length: int = 0


@dataclass
class ZRawData:
    """
    合并接收数据数据结构，支持CAN/CANFD/LIN等不同类型数据
    """

    number: int = 0  # 数据编号
    absoluteTimestamp: int = 0  # 绝对时间戳,单位微秒(us)
    relativeTimestamp: int = 0  # 相对时间戳,单位微秒(us)
    channel: int = 0  # 通道号
    data: Union[
        ZCANFDData,
        ZLINData,
        ZLINErrorData,
        ZLINEventData,
        ZBusUsageData,
        ZEthernetLoopbackData,
        ZEthernetData,
        ZEthernetDot3Data,
        ZGPSData,
    ] = None  # 数据


@unique
class ZUserVariableType(IntEnum):
    Undefined = 0
    Int = 1
    Uint = 2
    Double = 4
    String = 5


@dataclass
class ZUserVariable:
    name: str
    valueType: ZUserVariableType
    group: str = ""
    unit: str = ""
    initValue: str = ""
    minValue: str = ""
    maxValue: str = ""
    comment: str = ""


@dataclass
class ZSignalValue:
    sourceId: str
    signalId: str
    timestamp: int = 0
    frameNumber: int = 0
    rawValue: Optional[Union[int, float, str, list]] = None
    phyValue: Optional[Union[int, float, str, list]] = None
    rowIndex: int = -1
    colIndex: int = -1


@unique
class ZMeasEvent(IntEnum):
    POLLING = 1  # 轮询获取
    DAQ_EVENT = 2  # ECU按指定DAQ事件上送
    DAQ_CYCLIC = 3  # ECU按DAQ事件的整数倍周期上送, 周期小于1ms的事件不支持Cyclic模式
    ON_INPUT = 4  # 依赖输入信号，自己不轮询


@unique
class ZE2ECrcType(IntEnum):
    Crc8 = 0
    Crc8Sae = 1
    Crc8H2f = 2
    Crc16 = 3
    Crc16Ccitt = 4
    Crc16CcittFalse = 5
    Crc32 = 6
    Crc32P4 = 7
    Crc64Ecma = 8
    Custom = 9


@dataclass
class ZE2ECRCCalculatorParameters:
    width: int
    polynomial: int
    initialValue: int
    xorValue: int
    reflectInput: bool
    reflectOutput: bool


@unique
class ZSimuSendType(IntEnum):
    Cycle = 0  # 循环发送
    OnChanged = 1  # 当信号值变更时
    OnWritten = 2  # 当信号值被设置时（即使是相同值）


@unique
class ZDeviceType(IntEnum):
    CCP = 0  # CCP协议的ECU
    XCP = 1  # XCP协议的ECU
    Diagnostic = 2  # 诊断
    CANFD = 3  # CAN或CANFD总线监测
    LIN = 4  # LIN总线监测
    OBDDiagnostic = 5  # OBD诊断
    Acquistion = 6  # 传感器设备采集
    Unknown = 0xFF


@dataclass
class ZDeviceInfo:
    id: str = ""
    type: ZDeviceType = ZDeviceType.Unknown
    name: str = ""
    busType: ZBusType = ZBusType.Unknown
    logicalChannel: int = 0
    enabled: bool = False
    databaseId: str = ""
    databaseName: str = ""


@unique
class ZCANFrameType(IntEnum):
    CAN = 0
    CANFD = 1
    CANFD_BRS = 2


@unique
class ZCANTpVersion(IntEnum):
    ISO15765_2_2004 = 0
    ISO15765_2_2016 = 1


@unique
class ZUdsPort(IntEnum):
    AutoDetect = 0  # 自适应
    Hardware = 1  # zlgcan
    Software = 2  # zuds


@dataclass
class ZDoCANCfg:
    udsPort: ZUdsPort = ZUdsPort.AutoDetect
    channelIndex: int = 0
    frameType: ZCANFrameType = ZCANFrameType.CAN
    protocolVersion: ZCANTpVersion = ZCANTpVersion.ISO15765_2_2004
    fillByte: int = 0x00
    isfillByte: bool = False
    p2Timeout: int = 2000
    p2xTimeout: int = 5000
    isModifyEcuSTmin: bool = False
    remoteSTmin: int = 0
    localSTmin: int = 0
    blockSize: int = 0
    fcTimeout: int = 1000


@dataclass
class ZDoLINCfg:
    udsPort: ZUdsPort = ZUdsPort.AutoDetect
    channelIndex: int = 0
    fillByte: int = 0x00
    isfillByte: bool = False
    p2Timeout: int = 2000
    p2xTimeout: int = 5000
    isModifyEcuSTmin: bool = False
    remoteSTmin: int = 0


@unique
class ZDoipProtocolVersion(IntEnum):
    ISO_13400_2_2010 = 1
    ISO_13400_2_2012 = 2
    ISO_13400_2_2019 = 3
    ISO_13400_2_2019_AMD_1 = 4


@unique
class ZDoipRoutingActivationType(IntEnum):
    Default = 0x00  # 默认激活类型
    WWH_OBD = 0x01  # WWH-OBD
    CentralSecurity = 0xE0  # 安全模式


@dataclass
class ZDoIPCfg:
    udsPort: ZUdsPort = ZUdsPort.AutoDetect
    vehicleIp: str = ""
    localIp: str = ""
    localPort: int = 0
    protocolVersion: ZDoipProtocolVersion = ZDoipProtocolVersion.ISO_13400_2_2019
    testerAddress: int = 0xE00
    routingActivationType: ZDoipRoutingActivationType = (
        ZDoipRoutingActivationType.Default
    )
    withOEMSpecificData: bool = False
    oemSpecificData: bytes = field(default_factory=bytes)
    aliveCheckCycle: int = 0
    isResponseAliveCheck: bool = False
    p2Timeout: int = 2000
    p2xTimeout: int = 5000
    waitForACK: bool = True
    ackTimeoutMs: int = 0
    connectTimeout: int = 2000


@dataclass
class ZUdsRequest:
    reqAddr: int = 0
    rspAddr: int = 0
    sid: int = 0
    data: bytes = field(default_factory=bytes)
    extend: bool = False
    suppressResponse: bool = False


@unique
class ZUdsResponseStatus(IntEnum):
    Ok = 0  # 成功
    Canceled = 1  # 取消操作
    SuppressResponse = 2  # 抑制响应
    Failed = 3  # 失败


@unique
class ZUdsResponseType(IntEnum):
    Negative = 0  # 消极响应
    Positive = 1  # 积极响应
    Unknown = 0xFF


@dataclass
class ZUdsResponse:
    status: ZUdsResponseStatus = ZUdsResponseStatus.Failed
    responseType: ZUdsResponseType = ZUdsResponseType.Unknown
    errorCode: int = 0
    sid: int = 0
    data: bytes = field(default_factory=bytes)
    NRC: int = 0

    def __bool__(self):
        return (
            self.status == ZUdsResponseStatus.Ok
            and self.responseType == ZUdsResponseType.Positive
        )


class ZCRCCalculator:
    """
    CRC计算器
    """

    def __init__(self, handle: int, calcHandle: int):
        self.__handle = handle
        self.__calcHandle = calcHandle

    def calculate(self, data: bytes, prev_result: Optional[int] = None) -> int:
        data_arr = (ZXDocComm.ZXDoc_UByte * len(data))()
        for i in range(len(data)):
            data_arr[i] = data[i]
        result = ZXDocComm.ZXDoc_U64(0)
        ZXDocComm.ZXDoc_E2E_CrcCalculate(
            self.__handle,
            self.__calcHandle,
            data_arr,
            len(data),
            ctypes.pointer(result),
            prev_result,
        )
        return result.value


@unique
class ZDBType(IntEnum):
    UNKNOWN = 0x00
    DBC = 0x01
    ODX = 0x02
    A2L = 0x04
    LDF = 0x08
    SysVar = 0x10
    ExprVar = 0x20
    UserVar = 0x40
    AcqPlugin = 0x80
    BasicDiag = 0x100
    FuncVar = 0x200
    ARXML = 0x400
    ArxmlCAN = 0x800
    ArxmlETH = 0x1000
    ALL = 0xFFFF


@unique
class ZDBCMultiplexerIndicator(IntEnum):
    Normal = 0  # 普通信号
    Multiplexed = 1  # 复用信号
    Multiplexer = 2  # 复用开关


@unique
class ZDBCSigValueType(IntEnum):
    Int = 0  # int
    Float = 1  # float
    Double = 2  # double


@dataclass
class ZDBCSignal:
    name: str = ""  # 名称
    comment: str = ""  # 注释
    multiplexedIndicator: ZDBCMultiplexerIndicator = (
        ZDBCMultiplexerIndicator.Normal
    )  # 复用器标志
    multiplexerValue: int = 0  # 复用器开关值，仅为复用器类型为复用信号时有效有效
    startBit: int = 0  # 起始位
    length: int = 0  # 位长度
    isIntel: bool = False  # 0:大端 1:小端
    isSigned: bool = False  # 0:无符号数据 1:有符号数据
    factor: float = 0  # 比例
    offset: float = 0  # 偏移
    minValue: float = 0  # 最小值
    maxValue: float = 0  # 最大值
    unit: str = ""  # 单位
    valueType: ZDBCSigValueType = ZDBCSigValueType.Int  # 信号原始值类型
    receivers: List[str] = field(
        default_factory=list
    )  # 信号的接收节点,注意：若节点名为Vector__XXX则表示任意节点
    valueTable: Dict[int, str] = field(default_factory=dict)  # 信号值描述
    attrs: Dict[str, str] = field(default_factory=dict)  # 属性


@dataclass
class ZDBCMessage:
    __handle: int = field(repr=False, default=0)
    databaseId: str = ""  # 数据库ID
    id: int = 0  # 消息ID (带扩展帧标识位)
    len: int = 0  # 消息的字节数目
    name: str = ""  # 名称
    comment: str = ""  # 注释
    busType: str = "CAN"  # 报文总线类型 CAN/CAN FD
    isJ1939Frame: bool = False  # 是否为J1939帧类型
    isBRS: bool = False  # CANFD_BRS
    transmitter: str = ""  # 消息的发送节点
    multiplexer: str = ""  # 复用器开关
    signalNames: List[str] = field(default_factory=list)  # 所有信号名称
    signalGroups: Dict[str, List[str]] = field(default_factory=dict)  # 所有信号组
    attrs: Dict[str, str] = field(default_factory=dict)  # 属性

    def __wrap_signal(self, pSgl: ZXDocComm.ZXDoc_DBCSignalInfo):
        if pSgl is None:
            return None

        receivers = []
        valueTable = {}
        attrs = {}

        cnt = ZXDocComm.ZXDoc_DBCSignalInfo_GetReceiverCount(pSgl)
        for n in range(cnt):
            receivers.append(
                ZXDocComm.ZXDoc_DBCSignalInfo_GetReceiver(pSgl, n).decode(STR_ENCODING)
            )

        cnt = ZXDocComm.ZXDoc_DBCSignalInfo_GetValueTableSize(pSgl)
        for n in range(cnt):
            key = ZXDocComm.ZXDoc_I64()
            value = ZXDocComm.ZXDoc_CharP()
            ZXDocComm.ZXDoc_DBCSignalInfo_GetValueTable(
                pSgl, n, ctypes.byref(key), ctypes.byref(value)
            )
            valueTable[key.value] = value.value.decode(STR_ENCODING)

        cnt = ZXDocComm.ZXDoc_DBCSignalInfo_GetAttrCount(pSgl)
        for n in range(cnt):
            key = ZXDocComm.ZXDoc_CharP()
            value = ZXDocComm.ZXDoc_CharP()
            ZXDocComm.ZXDoc_DBCSignalInfo_GetAttr(
                pSgl, n, ctypes.byref(key), ctypes.byref(value)
            )
            attrs[key.value.decode(STR_ENCODING)] = value.value.decode(STR_ENCODING)

        return ZDBCSignal(
            ZXDocComm.ZXDoc_DBCSignalInfo_GetName(pSgl).decode(STR_ENCODING),
            ZXDocComm.ZXDoc_DBCSignalInfo_GetComment(pSgl).decode(STR_ENCODING),
            ZDBCMultiplexerIndicator(
                ZXDocComm.ZXDoc_DBCSignalInfo_GetMultiplexerIndicator(pSgl)
            ),
            ZXDocComm.ZXDoc_DBCSignalInfo_GetMultiplexerValue(pSgl),
            ZXDocComm.ZXDoc_DBCSignalInfo_GetStartBit(pSgl),
            ZXDocComm.ZXDoc_DBCSignalInfo_GetLength(pSgl),
            bool(ZXDocComm.ZXDoc_DBCSignalInfo_IsIntel(pSgl)),
            bool(ZXDocComm.ZXDoc_DBCSignalInfo_IsSigned(pSgl)),
            ZXDocComm.ZXDoc_DBCSignalInfo_GetFactor(pSgl),
            ZXDocComm.ZXDoc_DBCSignalInfo_GetOffset(pSgl),
            ZXDocComm.ZXDoc_DBCSignalInfo_GetMinValue(pSgl),
            ZXDocComm.ZXDoc_DBCSignalInfo_GetMaxValue(pSgl),
            ZXDocComm.ZXDoc_DBCSignalInfo_GetUnit(pSgl).decode(STR_ENCODING),
            ZDBCSigValueType(ZXDocComm.ZXDoc_DBCSignalInfo_GetValueType(pSgl)),
            receivers,
            valueTable,
            attrs,
        )

    def get_signals(self) -> List[ZDBCSignal]:
        pSgls = ZXDocComm.ZXDoc_DBCSignalInfoP()
        if ZXDocComm.ZXDOC_E_OK != ZXDocComm.ZXDoc_DB_DbcGetSignalsByMsgId(
            self.__handle,
            self.databaseId.encode(STR_ENCODING),
            self.id,
            ctypes.byref(pSgls),
        ):
            return None

        if not bool(pSgls):
            return []

        sgls = []

        i = 0
        while pSgls[i]:
            sgls.append(self.__wrap_signal(pSgls[i]))
            i += 1

        ZXDocComm.ZXDoc_DBCSignalInfos_Free(pSgls)

        return sgls

    def get_signal(self, signalName: str) -> ZDBCSignal:
        pSgl = ZXDocComm.ZXDoc_DBCSignalInfo()
        if ZXDocComm.ZXDOC_E_OK != ZXDocComm.ZXDoc_DB_DbcGetSignalByMsgId(
            self.__handle,
            self.databaseId.encode(STR_ENCODING),
            self.id,
            signalName.encode(STR_ENCODING),
            ctypes.byref(pSgl),
        ):
            return None

        sgl = self.__wrap_signal(pSgl)

        ZXDocComm.ZXDoc_DBCSignalInfo_Free(pSgl)

        return sgl


@dataclass
class ZDBCData:
    id: int = 0
    data: bytes = field(default_factory=bytes)


@dataclass
class ZDBCSignalValue:
    # name:str = ""
    raw: Union[int, float] = 0
    phy: float = 0


@unique
class ZDBCEncodeObject(IntEnum):
    RawValue = 1
    PhyValue = 2


@dataclass
class ZDBCMessageValue:
    id: int = 0
    name: str = ""
    isCanFd: bool = False
    isJ1939Frame: bool = False
    signalValues: Dict[str, ZDBCSignalValue] = field(default_factory=dict)

    # def set(self, name: str, value: ZDBCSignalValue):
    #     self.signalValues[name] = value

    # def get(self, name: str) -> ZDBCSignalValue:
    #     return self.signalValues[name]

    def __getattr__(self, name: str):
        if name in self.signalValues:
            return self.signalValues[name]
        return super().__getattribute__(name)

    def __setattr__(self, name, value):
        if isinstance(value, ZDBCSignalValue):
            self.signalValues[name] = value
            return
        super().__setattr__(name, value)

    def __delattr__(self, name):
        if name in self.signalValues:
            del self.signalValues[name]
            return
        if name in self.__dataclass_fields__:
            return
        super().__delattr__(name)

    def __getitem__(self, key: str):
        if not isinstance(key, str):
            raise TypeError("the key must be str")

        return self.signalValues[key]

    def __setitem__(self, key: str, value: ZDBCSignalValue):
        if not isinstance(key, str):
            raise TypeError("the key must be str")

        if isinstance(value, ZDBCSignalValue):
            self.signalValues[key] = value
            return
        raise TypeError("the value must be a ZDBCSignalValue")

    def __delitem__(self, key: str):
        if not isinstance(key, str):
            raise TypeError("the key must be str")

        if key not in self.signalValues:
            return
        del self.signalValues[key]


@dataclass
class ZSignalIdentifier:
    sourceId: str
    signalId: str


@dataclass
class ZDataRecorderCfg:
    """
    记录器配置基类
    """

    recorderName: str = ""  # 记录器名称
    filePath: str = ""  # 记录文件路径
    maxFileSize: int = 0  # 记录文件的最大大小，超出该值时会自动分文件
    comment = ""  # 注释
    fileNameAutoAddTimeSuffix: bool = False  # 文件名是否自动添加时间后缀
    mf4Compression: bool = False  # mf4压缩


@dataclass
class ZMeasureDataRecorderCfg(ZDataRecorderCfg):
    """
    信号记录器配置
    """

    signals: List[ZSignalIdentifier] = field(default_factory=list)  # 记录的信号列表


@dataclass
class ZMessageRecorderCfg(ZDataRecorderCfg):
    channels: List[ZChannel] = field(default_factory=list)  # 记录的通道列表


@unique
class ZEcuMemPageType(IntEnum):
    """
    内存页类型
    """

    Memory = 0
    Flash = 1


@unique
class ZFilterMode(IntEnum):
    """
    过滤模式
    """

    Accept = 0  # 通过
    Reject = 1  # 阻止


class ZFilterDirection(IntEnum):
    """
    过滤方向
    """

    Rx = 0x01  # 接收
    Tx = 0x02  # 发送
    All = 0xFF  # 全部


class ZCanFilterFrameType(IntEnum):
    """
    过滤CAN帧类型
    """

    Std = 0x01  # 标准帧
    Ext = 0x02  # 扩展帧
    All = 0xFF  # 全部


@dataclass
class ZDataSinkFilterRule:
    """
    过滤配置项的基类
    """

    channelIndex: int = -1  # 通道号
    direction: ZFilterDirection = ZFilterDirection.Rx  # 数据方向


@dataclass
class ZCANErrorFilter(ZDataSinkFilterRule):
    """
    CAN错误帧
    """

    ...


@dataclass
class ZCANIDRangeFilter(ZDataSinkFilterRule):
    """
    CAN帧
    """

    frameType: ZCanFilterFrameType = ZCanFilterFrameType.All  # CAN帧类型
    idMin: int = 0  # 最小CAN ID
    idMax: int = 0x1FFFFFFF  # 最大CAN ID


@dataclass
class ZLINErrorFilter(ZDataSinkFilterRule):
    """
    LIN错误帧
    """

    ...


@dataclass
class ZLINWakeUpEventFilter(ZDataSinkFilterRule):
    """
    LIN事件
    """

    ...


@dataclass
class ZLINIDRangeFilter(ZDataSinkFilterRule):
    """
    LIN帧
    """

    idMin: int = 0  # 最小LIN ID
    idMax: int = 0x3F  # 最大LIN ID


@dataclass
class ZDataSinkFilter:
    """
    数据接收过滤器配置

    成员：
        filterMode:
            Accept: 任意过滤条目条件满足即通过
            Reject: 任意过滤条目条件满足即阻止

        items: 过滤条目列表
    """

    filterMode: ZFilterMode = ZFilterMode.Accept  # 过滤模式
    rules: List[ZDataSinkFilterRule] = field(default_factory=list)  # 过滤规则


@dataclass
class ZDatabase:
    type: ZDBType = ZDBType.UNKNOWN
    id: str = ""
    name: str = ""
    filePath: str = ""
    __handle: int = field(repr=False, default=0)

    def zxdoc_handle(self):
        return self.__handle


class ZDbcDatabase(ZDatabase):
    def __wrap_message(self, pMsg):
        attrs = {}
        cnt = ZXDocComm.ZXDoc_DBCMessageInfo_GetAttrCount(pMsg)
        for n in range(cnt):
            key = ZXDocComm.ZXDoc_CharP()
            value = ZXDocComm.ZXDoc_CharP()
            ZXDocComm.ZXDoc_DBCMessageInfo_GetAttr(
                pMsg, n, ctypes.byref(key), ctypes.byref(value)
            )
            attrs[key.value.decode(STR_ENCODING)] = value.value.decode(STR_ENCODING)

        busType = ZXDocComm.ZXDoc_DBCMessageInfo_GetBusType(pMsg)
        busTypeStr = (
            "CAN"
            if ZXDocComm.ZDBCBusType_Can == busType
            else "CAN FD" if ZXDocComm.ZDBCBusType_CanFd == busType else ""
        )

        signalNames = []
        cnt = ZXDocComm.ZXDoc_DBCMessageInfo_GetSignalNameCount(pMsg)
        for n in range(cnt):
            signalNames.append(
                ZXDocComm.ZXDoc_DBCMessageInfo_GetSignalName(pMsg, n).decode(
                    STR_ENCODING
                )
            )

        signalGroups = {}
        cnt = ZXDocComm.ZXDoc_DBCMessageInfo_GetSignalGroupCount(pMsg)
        for n in range(cnt):
            gpName = ZXDocComm.ZXDoc_DBCMessageInfo_GetSignalGroupName(pMsg, n).decode(
                STR_ENCODING
            )

            sglNames = []
            sglCnt = ZXDocComm.ZXDoc_DBCMessageInfo_GetSignalGroupSignalCount(pMsg, n)
            for m in range(sglCnt):
                sglNames.append(
                    ZXDocComm.ZXDoc_DBCMessageInfo_GetSignalGroupSglName(
                        pMsg, n, m
                    ).decode(STR_ENCODING)
                )
            signalGroups[gpName] = sglNames

        return ZDBCMessage(
            self.zxdoc_handle(),
            self.id,
            ZXDocComm.ZXDoc_DBCMessageInfo_GetId(pMsg),
            ZXDocComm.ZXDoc_DBCMessageInfo_GetLen(pMsg),
            ZXDocComm.ZXDoc_DBCMessageInfo_GetName(pMsg).decode(STR_ENCODING),
            ZXDocComm.ZXDoc_DBCMessageInfo_GetComment(pMsg).decode(STR_ENCODING),
            busTypeStr,
            bool(ZXDocComm.ZXDoc_DBCMessageInfo_IsJ1939Frame(pMsg)),
            bool(ZXDocComm.ZXDoc_DBCMessageInfo_IsBRS(pMsg)),
            ZXDocComm.ZXDoc_DBCMessageInfo_GetTransmitter(pMsg).decode(STR_ENCODING),
            ZXDocComm.ZXDoc_DBCMessageInfo_GetMultiplexer(pMsg).decode(STR_ENCODING),
            signalNames,
            signalGroups,
            attrs,
        )

    def get_messages(self) -> List[ZDBCMessage]:
        pMsgs = ZXDocComm.ZXDoc_DBCMessageInfoP()
        if ZXDocComm.ZXDOC_E_OK != ZXDocComm.ZXDoc_DB_DbcGetMessages(
            self.zxdoc_handle(), self.id.encode(STR_ENCODING), ctypes.byref(pMsgs)
        ):
            return None

        if not bool(pMsgs):
            return []

        msgs = []

        i = 0
        while pMsgs[i]:
            msgs.append(self.__wrap_message(pMsgs[i]))
            i += 1

        ZXDocComm.ZXDoc_DBCMessageInfos_Free(pMsgs)
        return msgs

    def get_message_by_name(self, node: str, name: str) -> ZDBCMessage:
        pMsg = ZXDocComm.ZXDoc_DBCMessageInfo()
        if ZXDocComm.ZXDOC_E_OK != ZXDocComm.ZXDoc_DB_DbcGetMessageByName(
            self.zxdoc_handle(),
            self.id.encode(STR_ENCODING),
            node.encode(STR_ENCODING),
            name.encode(STR_ENCODING),
            ctypes.byref(pMsg),
        ):
            return None

        msg = self.__wrap_message(pMsg)
        ZXDocComm.ZXDoc_DBCMessageInfo_Free(pMsg)
        return msg

    def get_message_by_id(self, id: int) -> ZDBCMessage:
        pMsg = ZXDocComm.ZXDoc_DBCMessageInfo()
        if ZXDocComm.ZXDOC_E_OK != ZXDocComm.ZXDoc_DB_DbcGetMessageById(
            self.zxdoc_handle(), self.id.encode(STR_ENCODING), id, ctypes.byref(pMsg)
        ):
            return None

        msg = self.__wrap_message(pMsg)
        ZXDocComm.ZXDoc_DBCMessageInfo_Free(pMsg)
        return msg

    def encode(
        self,
        message: ZDBCMessageValue,
        encodeObj: ZDBCEncodeObject = ZDBCEncodeObject.PhyValue,
    ) -> ZDBCData:
        pMsg = ZXDocComm.ZXDoc_DBCMsgValue_New(message.id)
        dbcData = ZXDocComm.ZXDoc_DBCData()

        for name, sgl in message.signalValues.items():
            pSgl = ZXDocComm.ZXDoc_DBCSignalValue_New()
            ZXDocComm.ZXDoc_DBCSignalValue_SetName(pSgl, name.encode(STR_ENCODING))
            ZXDocComm.ZXDoc_DBCSignalValue_SetPhyValue(pSgl, sgl.phy)

            if isinstance(sgl.raw, int):
                if sgl.raw >= 0xFFFFFFFFFFFFFFFF:
                    ZXDocComm.ZXDoc_DBCSignalValue_SetRawUint(pSgl, sgl.raw)
                else:
                    ZXDocComm.ZXDoc_DBCSignalValue_SetRawInt(pSgl, sgl.raw)
            elif isinstance(sgl.raw, float):
                ZXDocComm.ZXDoc_DBCSignalValue_SetRawDouble(pSgl, sgl.raw)

            ZXDocComm.ZXDoc_DBCMsgValue_AppendSignal(pMsg, pSgl)

        if ZXDocComm.ZXDOC_E_OK != ZXDocComm.ZXDoc_DB_DbcMessageEncode(
            self.zxdoc_handle(),
            self.id.encode(STR_ENCODING),
            pMsg,
            ctypes.byref(dbcData),
            encodeObj,
        ):
            ZXDocComm.ZXDoc_DBCMsgValue_Free(pMsg)
            return None

        ZXDocComm.ZXDoc_DBCMsgValue_Free(pMsg)
        return ZDBCData(
            dbcData.id,
            bytes(dbcData.data[0 : dbcData.length]),
        )

    def decode(self, data: ZDBCData) -> ZDBCMessageValue:
        dbcData = ZXDocComm.ZXDoc_DBCData()
        dbcData.id = data.id
        dbcData.length = min(len(dbcData.data), len(data.data))
        for i in range(dbcData.length):
            dbcData.data[i] = data.data[i]

        pMsg = ZXDocComm.ZXDoc_DBCMsgValue_New()

        if ZXDocComm.ZXDOC_E_OK != ZXDocComm.ZXDoc_DB_DbcMessageDecode(
            self.zxdoc_handle(),
            self.id.encode(STR_ENCODING),
            ctypes.byref(dbcData),
            pMsg,
        ):
            ZXDocComm.ZXDoc_DBCMsgValue_Free(pMsg)
            return None

        msg = pMsg.contents

        _msg = ZDBCMessageValue()
        _msg.id = msg.id
        _msg.isCanFd = False if 0 == msg.isFd else True
        _msg.isJ1939Frame = False if 0 == msg.isJ1939Frame else True
        _msg.name = msg.name.decode(STR_ENCODING)

        i = 0
        while bool(msg.signalValues) and bool(msg.signalValues[i]):
            pSgl = msg.signalValues[i]
            if not bool(pSgl):
                break
            i += 1

            _sgl = pSgl.contents
            sgl = ZDBCSignalValue(
                raw=(
                    _sgl.rawValue.n
                    if _sgl.rawValueType == ZXDocComm.ZXDoc_DBCSignalValue_int
                    else (
                        _sgl.rawValue.f
                        if _sgl.rawValueType == ZXDocComm.ZXDoc_DBCSignalValue_float
                        else _sgl.rawValue.d
                    )
                ),
                phy=_sgl.phyValue,
            )
            _msg.signalValues[_sgl.name.decode(STR_ENCODING)] = sgl

        ZXDocComm.ZXDoc_DBCMsgValue_Free(msg)
        return _msg


class ZOdxDatabase(ZDatabase):
    pass


class ZA2lDatabase(ZDatabase):
    pass


class ZLdfDatabase(ZDatabase):
    pass


class ZSysVarDatabase(ZDatabase):
    pass


class ZFuncVarDatabase(ZDatabase):
    pass


class ZUserVarDatabase(ZDatabase):
    pass


class ZPluginDatabase(ZDatabase):
    pass


class ZSeedKeyDll:
    """
    安全访问DLL
    """

    __dll = None
    __GenerateKeyEx = None
    __GenerateKeyExOpt = None
    __ZLGKey = None

    def __init__(self, dllPath: str):
        self.__dll = ctypes.WinDLL(dllPath)
        if hasattr(self.__dll, "GenerateKeyEx"):
            self.__dll.GenerateKeyEx.argtypes = [
                ctypes.c_char_p,  # ipSeedArray
                ctypes.c_uint16,  # iSeedArraySize
                ctypes.c_uint32,  # iSecurityLevel
                ctypes.c_char_p,  # ipVarnant
                ctypes.c_char_p,  # iopKeyArray
                ctypes.c_uint32,  # iMaxKeyArraySize
                ctypes.POINTER(ctypes.c_uint32),  # oActualKeyArraySize
            ]
            self.__dll.GenerateKeyEx.restype = ctypes.c_int32
            self.__GenerateKeyEx = self.__dll.GenerateKeyEx

        if hasattr(self.__dll, "GenerateKeyExOpt"):
            self.__dll.GenerateKeyExOpt.argtypes = [
                ctypes.c_char_p,  # ipSeedArray
                ctypes.c_uint32,  # iSeedArraySize
                ctypes.c_uint32,  # iSecurityLevel
                ctypes.c_char_p,  # ipVarnant
                ctypes.c_char_p,  # ipOptions
                ctypes.c_char_p,  # iopKeyArray
                ctypes.c_uint32,  # iMaxKeyArraySize
                ctypes.POINTER(ctypes.c_uint32),  # oActualKeyArraySize
            ]
            self.__dll.GenerateKeyExOpt.restype = ctypes.c_int32
            self.__GenerateKeyExOpt = self.__dll.GenerateKeyExOpt

        if hasattr(self.__dll, "ZLGKey"):
            self.__dll.ZLGKey.argtypes = [
                ctypes.c_char_p,  # iSeedArray
                ctypes.c_uint16,  # iSeedArraySize
                ctypes.c_uint32,  # iSecurityLevel
                ctypes.c_char_p,  # ipVarnant
                ctypes.c_char_p,  # iKeyArray
                ctypes.POINTER(ctypes.c_uint16),  # iKeyArraySize
            ]
            self.__dll.ZLGKey.restype = ctypes.c_int32
            self.__ZLGKey = self.__dll.ZLGKey

    def generate_key(
        self, seed: bytes, lvl: int, ipVarnant: bytes = None, ipOptions: bytes = None
    ) -> bytes:
        if self.__GenerateKeyEx is not None:
            keySize = ctypes.c_uint32(128)
            key = (ctypes.c_char * keySize.value)()

            self.__GenerateKeyEx(
                seed, len(seed), lvl, ipVarnant, key, len(key), ctypes.byref(keySize)
            )
            return key.value[: keySize.value]

        if self.__GenerateKeyExOpt is not None:
            keySize = ctypes.c_uint32(128)
            key = (ctypes.c_char * keySize.value)()

            self.__GenerateKeyExOpt(
                seed,
                len(seed),
                lvl,
                ipVarnant,
                ipOptions,
                key,
                len(key),
                ctypes.byref(keySize),
            )
            return key.value[: keySize.value]

        if self.__ZLGKey is not None:
            keySize = ctypes.c_uint16(128)
            key = (ctypes.c_char * keySize.value)()
            self.__ZLGKey(seed, len(seed), lvl, ipVarnant, key, ctypes.byref(keySize))
            return key.value[: keySize.value]

        return None

    def __call__(
        self, seed: bytes, lvl: int, ipVarnant: bytes = None, ipOptions: bytes = None
    ) -> bytes:
        return self.generate_key(seed, lvl, ipVarnant, ipOptions)


@dataclass
class ZSecurityAccessReq:
    """
    安全访问请求
    """

    srcAddr: int = 0x731  # 请求地址
    dstAddr: int = 0x7B1  # 响应地址
    isExtend: bool = False  # 是否扩展帧（仅CAN通道有效）
    keyDllPath: str = ""  # 安全算法链接库路径
    securityLevel: int = 1  # 安全级别
    reqData: bytes = field(default_factory=bytes)  # 拼接在种子请求后面的数据
    variant: str = ""  # 密钥生成函数使用的参数


@unique
class ZFlashFileType(IntEnum):
    """
    刷写文件类型
    """

    GAC = 0  # GAC app
    GACFlashDriver = 1  # GAC flash driver
    SRec = 2  # S19
    IHex = 3  # hex
    Bin = 4  # raw files
    Script = 5  # python
    Unknown = 0xFF


@dataclass
class ZFlashFileBlock:
    """
    刷写文件块
    """

    address: int = 0  # 文件块的起始地址
    data: bytes = field(default_factory=bytes)  # 文件块的数据


@dataclass
class ZFlashFileInfo:
    """
    刷写文件的信息
    """

    filePath: str = ""  # 文件路径
    fileType: ZFlashFileType = ZFlashFileType.Unknown  # 文件类型
    blocks: List[ZFlashFileBlock] = field(default_factory=list)  # 文件块列表


@unique
class ZCrcType(IntEnum):
    """
    CRC类型
    """

    CRC8 = 0
    CRC16 = 1
    CRC32 = 2


@unique
class ZMemEraseType(IntEnum):
    """
    内存擦除类型
    """

    NoErase = 0  # 不擦除
    WithParam = 1  # 带参数擦除
    EveryBlock = 2  # 按块擦除
    WithoutParam = 3  # 不带参数擦除


@dataclass
class ZCrcAlgorithm:
    """
    CRC算法参数
    """

    type: ZCrcType = ZCrcType.CRC32  # CRC类型
    polynomial: int = 0x4C11DB7  # 多项式
    initValue: int = 0xFFFFFFFF  # 初始值
    xorOutput: int = 0xFFFFFFFF  #
    reflectInput: bool = True  # 输入反转
    reflectOutput: bool = True  # 输出反转

    def crcValueBitLength(self):
        return (
            32
            if ZCrcType.CRC32 == self.type
            else 16 if ZCrcType.CRC16 == self.type else 8
        )

    def crcValueByteLength(self):
        return (
            4
            if ZCrcType.CRC32 == self.type
            else 2 if ZCrcType.CRC16 == self.type else 1
        )


class ZCrcBase:
    crc = 0
    algorithm: ZCrcAlgorithm = None
    table = None

    @classmethod
    def byteReverse(cls, byte):
        byte = ((byte & 0xF0) >> 4) | ((byte & 0x0F) << 4)
        byte = ((byte & 0xCC) >> 2) | ((byte & 0x33) << 2)
        byte = ((byte & 0xAA) >> 1) | ((byte & 0x55) << 1)
        return byte

    def __init__(self, algorithm: ZCrcAlgorithm):
        self.algorithm = algorithm
        self.crc = algorithm.initValue
        self.table = self.generate_table()

    def reset(self):
        self.crc = algorithm.initValue

    def update(self, data: Iterable[int]): ...
    def final(self): ...


class ZCrc8(ZCrcBase):
    def generate_table(self):
        table = []
        poly = self.algorithm.polynomial & 0xFF
        for i in range(256):
            crc = i
            for _ in range(8):
                if crc & 0x80:
                    crc = (crc << 1) ^ poly
                else:
                    crc = crc << 1
            table.append(crc & 0xFF)
        return table

    def update(self, data: Iterable[int]):
        for byte in data:
            if self.algorithm.reflectInput:
                byte = ZCrcBase.byteReverse(byte)

            index = (self.crc ^ byte) & 0xFF
            self.crc = self.table[index]

    def final(self):
        if self.algorithm.reflectOutput:
            self.crc = ZCrcBase.byteReverse(self.crc)

        return self.crc ^ (self.algorithm.xorOutput & 0xFF)


class ZCrc16(ZCrcBase):
    def generate_table(self):
        table = []
        for i in range(256):
            crc = i << 8
            for _ in range(8):
                if crc & 0x8000:
                    crc = (crc << 1) ^ self.algorithm.polynomial
                else:
                    crc = crc << 1
            table.append(crc & 0xFFFF)
        return table

    def update(self, data: Iterable[int]):
        for byte in data:
            if self.algorithm.reflectInput:
                byte = ZCrcBase.byteReverse(byte)

            index = ((self.crc >> 8) ^ byte) & 0xFF
            self.crc = (self.crc << 8) ^ self.table[index]

            self.crc &= 0xFFFF

    def final(self):
        if self.algorithm.reflectOutput:
            h = ZCrcBase.byteReverse((self.crc >> 8) & 0xFF)
            l = ZCrcBase.byteReverse(self.crc & 0xFF)
            self.crc = (l << 8) | h

        return self.crc ^ (self.algorithm.xorOutput & 0xFFFF)


class ZCrc32(ZCrcBase):
    def generate_table(self):
        table = []
        poly = self.algorithm.polynomial & 0xFFFFFFFF
        for i in range(256):
            crc = i << 24
            for j in range(8):
                if crc & 0x80000000:
                    crc = (crc << 1) ^ poly
                else:
                    crc = crc << 1
            table.append(crc & 0xFFFFFFFF)
        return table

    def update(self, data: Iterable[int]):
        for byte in data:
            if self.algorithm.reflectInput:
                byte = ZCrcBase.byteReverse(byte)

            index = ((self.crc >> 24) ^ byte) & 0xFF
            self.crc = (self.crc << 8) ^ self.table[index]
            self.crc &= 0xFFFFFFFF

    def final(self):
        if self.algorithm.reflectOutput:
            re = 0
            for i in range(4):
                byte = (self.crc >> (i * 8)) & 0xFF
                re |= ZCrcBase.byteReverse(byte) << ((3 - i) * 8)
            self.crc = re

        return self.crc ^ self.algorithm.xorOutput


class ZCrc:
    crcBase: ZCrcBase = None

    def __init__(self, algorithm: ZCrcAlgorithm):
        self.algorithm = algorithm
        if self.algorithm.type == ZCrcType.CRC32:
            self.crcBase = ZCrc32(algorithm)
        elif self.algorithm.type == ZCrcType.CRC16:
            self.crcBase = ZCrc16(algorithm)
        elif self.algorithm.type == ZCrcType.CRC8:
            self.crcBase = ZCrc8(algorithm)

    def reset(self):
        if self.crcBase:
            self.crcBase.reset()

    def update(self, data: Iterable[int]):
        if self.crcBase:
            self.crcBase.update(data)

    def final(self):
        if self.crcBase:
            return self.crcBase.final()
        return 0


@unique
class ZTransExitCmdType(IntEnum):
    """
    $37 command
    """

    Normal = 0  # 常规命令，不带参数
    Custom = 1  # 自定义参数
    WithBlockCrc = 2  # 带块校验码


@dataclass
class FlashDataBlockCfg:
    startAddr: int = 0  # 文件块的起始地址
    dataLen: int = 0  # 文件块长度
    crc: int = 0  # CRC校验码
    fillByte: int = 0x00  # 位填充，未覆盖任何文件块的数据会使用该值填充
    mappedAddr: int = 0  # 映射地址，将文件块的起始地址映射为该地址，该地址即刷写的地址

    def dataLenSize(self):
        return ZUtility.MinLengthSize(self.dataLen)

    def endAddr(self):
        return self.startAddr + self.dataLen

    def mappedEndAddr(self):
        return self.mappedAddr + self.dataLen - 1

    def mappedEndAddrSize(self):
        return ZUtility.MinLengthSize(self.mappedEndAddr())

    def getData(self, blocks: List[ZFlashFileBlock]) -> bytearray:
        """
        从文件块内提取块数据
        """
        blockData = bytearray(self.dataLen)
        for i in range(self.dataLen):
            blockData[i] = self.fillByte

        startAddr = self.startAddr
        endAddr = self.endAddr()
        dataLen = self.dataLen

        for block in blocks:
            blockStartAddr = block.address
            blockEndAddr = blockStartAddr + len(block.data) - 1

            if startAddr <= blockEndAddr and endAddr >= blockStartAddr:
                offset = 0
                blockOffset = 0
                cpLen = 0
                if startAddr >= blockStartAddr:
                    blockOffset = startAddr - blockStartAddr
                    cpLen = min(len(block.data) - blockOffset, dataLen)

                if startAddr < blockStartAddr:
                    offset = blockStartAddr - startAddr
                    cpLen = min(len(block.data), dataLen - offset)

                for i in range(cpLen):
                    blockData[offset + i] = block.data[blockOffset + i]

        return blockData

    def updateCrc(self, crcAlgorithm: ZCrcAlgorithm, blocks: List[ZFlashFileBlock]):
        """
        更新本文件块配置的CRC值
        """
        self.crc = self.calculateCrc(crcAlgorithm, blocks)

    def calculateCrc(
        self, crcAlgorithm: ZCrcAlgorithm, blocks: List[ZFlashFileBlock]
    ) -> int:
        """
        计算并返回本文件块配置的CRC值
        """
        if crcAlgorithm is None:
            return 0
        crc = ZCrc(crcAlgorithm)
        crc.update(self.getData(blocks))
        return crc.final()


@dataclass
class ZFileDownloadReq:
    """
    文件下载请求
    """

    srcAddr: int = 0  # 请求地址
    dstAddr: int = 0  # 响应地址
    isExtend: bool = False  # 是否扩展帧（仅CAN通道有效）
    filePath: str = ""  # 文件路径，必填
    flashFileInfo: ZFlashFileInfo = None  # 从filePath加载的刷写文件的信息
    fileBlockCfgs: List[FlashDataBlockCfg] = field(default_factory=list)  # 文件块配置
    crcAlgorithm: ZCrcAlgorithm = field(default_factory=ZCrcAlgorithm)  # CRC算法
    memEraseType: ZMemEraseType = ZMemEraseType.NoErase  # 内存擦除类型
    dataFormatIdentifier: int = 0x00  # 数据格式标识
    blockTransDelayMs: int = 0  # 文件块传输延时
    blockTransRetryCount: int = 3  # 文件块传输失败的重试次数
    s36IntervalMs: int = 0  # 数据传输间隔
    addressSize: int = 4  # 地址值的字节长度
    lengthSize: int = 4  # 长度值的字节长度
    # 当 transExitCmdType 为 Normal 时，将发送请求：0x37
    # 当 transExitCmdType 为 WithBlockCrc 时，将发送请求：0x37 + 文件块CRC
    # 当 transExitCmdType 为 Custom 时，将发送请求：0x37 + customTransExitCmd
    transExitCmdType: ZTransExitCmdType = ZTransExitCmdType.Normal
    customTransExitCmd: bytes = field(default_factory=bytes)
    # 如果 customBlockCrcCheckCmd 不为空， 每个数据块传输完成后将发送: customBlockCrcCheckCmd + 文件块CRC
    customBlockCrcCheckCmd: bytes = field(default_factory=bytes)
    # 如果 totalCheckCmd 不为空，文件传输文件后将发送: totalCheckCmd + totalCrc
    totalCheckCmd: bytes = field(default_factory=bytes)
    totalCrc: int = None  # 所有文件块的总CRC

    def preLoadFlashFile(self, udsInterface) -> bool:
        """
        预加载刷写文件
        """
        if len(self.filePath) == 0:
            self.flashFileInfo = None
            return False

        # 加载刷写文件
        self.flashFileInfo = udsInterface.load_file_blocks(self.filePath)
        if self.flashFileInfo is None or len(self.flashFileInfo.blocks) == 0:
            print(f'Failed to load flash file: "{self.filePath}"')
            return False

        # 更新块配置
        if self.fileBlockCfgs is None or len(self.fileBlockCfgs) == 0:
            self.fileBlockCfgs = []
            for block in self.flashFileInfo.blocks:
                blockCfg = FlashDataBlockCfg()
                blockCfg.startAddr = block.address
                blockCfg.dataLen = len(block.data)
                blockCfg.mappedAddr = block.address
                blockCfg.updateCrc(self.crcAlgorithm, self.flashFileInfo.blocks)
                self.fileBlockCfgs.append(blockCfg)
        else:
            # 更新块配置
            for blockCfg in self.fileBlockCfgs:
                blockCfg.updateCrc(self.crcAlgorithm, self.flashFileInfo.blocks)

        self.updateTotalCrc()
        return True

    def isFlashFileLoaded(self) -> bool:
        """
        文件块是否已加载
        """
        return not (self.flashFileInfo is None or len(self.flashFileInfo.blocks))

    def updateTotalCrc(self):
        """
        更新总CRC
        """
        if self.crcAlgorithm is None:
            return
        crc = ZCrc(self.crcAlgorithm)
        for blockCfg in self.fileBlockCfgs:
            crc.update(blockCfg.getData(self.flashFileInfo.blocks))
        self.totalCrc = crc.final()


class ZUdsInterface:
    def __init__(self, handle: ZXDocComm.ZXDocHandle, udsHandle: int):
        self.__handle = handle
        self.__udsHandle = udsHandle

    def __del__(self):
        ZXDocComm.ZXDoc_Diag_FreeUdsInterface(self.__handle, self.__udsHandle)

    def request(self, req: ZUdsRequest):
        _req = ZXDocComm.ZXDoc_UdsRequest()
        _req.handle = self.__udsHandle
        _req.reqAddr = req.reqAddr
        _req.rspAddr = req.rspAddr
        _req.extend = req.extend
        _req.suppressResponse = req.suppressResponse
        _req.sid = req.sid
        _req.dataLen = len(req.data)

        data = None if 0 == _req.dataLen else (ctypes.c_ubyte * _req.dataLen)()
        for i in range(_req.dataLen):
            data[i] = req.data[i]

        rsp = ZXDocComm.ZXDoc_UdsResponse()
        if ZXDocComm.ZXDOC_E_OK != ZXDocComm.ZXDoc_Diag_UdsRequest(
            self.__handle,
            ctypes.byref(_req),
            None if 0 == _req.dataLen else ctypes.byref(data),
            ctypes.byref(rsp),
        ):
            return None

        rspData = None
        if bool(rsp.data):
            b = []
            for i in range(rsp.dataLen):
                b.append(rsp.data[i])
            rspData = bytes(b)
            ZXDocComm.ZXDoc_MemoryFree(rsp.data)

        return ZUdsResponse(
            status=ZUdsResponseStatus(rsp.status),
            responseType=ZUdsResponseType(rsp.responseType),
            errorCode=rsp.errorCode,
            sid=rsp.sid,
            data=rspData,
            NRC=rsp.NRC,
        )

    def functional_request(self, functAddr: int, req: ZUdsRequest):
        _req = ZXDocComm.ZXDoc_UdsRequest()
        _req.handle = self.__udsHandle
        _req.reqAddr = req.reqAddr
        _req.rspAddr = req.rspAddr
        _req.extend = req.extend
        _req.suppressResponse = req.suppressResponse
        _req.sid = req.sid
        _req.dataLen = len(req.data)

        data = None if 0 == _req.dataLen else (ctypes.c_ubyte * _req.dataLen)()
        for i in range(_req.dataLen):
            data[i] = req.data[i]

        rsp = ZXDocComm.ZXDoc_UdsResponse()
        if ZXDocComm.ZXDOC_E_OK != ZXDocComm.ZXDoc_Diag_UdsFunctionalRequest(
            self.__handle,
            functAddr,
            ctypes.byref(_req),
            None if 0 == _req.dataLen else ctypes.byref(data),
            ctypes.byref(rsp),
        ):
            return None

        rspData = None
        if bool(rsp.data):
            b = []
            for i in range(rsp.dataLen):
                b.append(rsp.data[i])
            rspData = bytes(b)
            ZXDocComm.ZXDoc_MemoryFree(rsp.data)

        return ZUdsResponse(
            status=ZUdsResponseStatus(rsp.status),
            responseType=ZUdsResponseType(rsp.responseType),
            errorCode=rsp.errorCode,
            sid=rsp.sid,
            data=rspData,
            NRC=rsp.NRC,
        )

    def cancel_request(self):
        ZXDocComm.ZXDoc_Diag_CancelRequest(self.__handle, self.__udsHandle)

    def get_error_message(self, rsp: ZUdsResponse, maxStrLen: int = 64):
        _rsp = ZXDocComm.ZXDoc_UdsResponse()
        _rsp.status = rsp.status
        _rsp.responseType = rsp.responseType
        _rsp.errorCode = rsp.errorCode
        _rsp.sid = rsp.sid
        _rsp.NRC = rsp.NRC
        str_buf = (ctypes.c_char * maxStrLen)()
        if ZXDocComm.ZXDOC_E_OK != ZXDocComm.ZXDoc_Diag_GetErrorMssage(
            self.__handle, self.__udsHandle, ctypes.byref(_rsp), str_buf, maxStrLen
        ):
            return ""

        return str_buf.value.decode(STR_ENCODING)

    def start_session_keep(
        self,
        address: int,
        interval: int,
        suppressResponse: bool,
        doNotSendWhenDataTrans: bool,
    ):
        ZXDocComm.ZXDoc_Diag_SatrtSessionKeep(
            self.__handle,
            self.__udsHandle,
            address,
            interval,
            suppressResponse,
            doNotSendWhenDataTrans,
        )

    def stop_session_keep(self):
        ZXDocComm.ZXDoc_Diag_StopSessionKeep(self.__handle, self.__udsHandle)

    def generate_security_key(
        self, dllPath: str, securityLevel: int, seed: Iterable[int], variant: str = None
    ) -> bytes:
        variant = "" if variant is None else variant
        keySize = ZXDocComm.ZXDoc_U32(2048)
        keyBuf = (ZXDocComm.ZXDoc_Char * keySize.value)()
        seedBuf = (ZXDocComm.ZXDoc_Char * len(seed))()
        if ZXDocComm.ZXDOC_E_OK != ZXDocComm.ZXDoc_Diag_CalcSecurityKey(
            self.__handle,
            dllPath.encode(STR_ENCODING),
            securityLevel,
            seedBuf,
            len(seedBuf),
            variant.encode(STR_ENCODING),
            keyBuf,
            ctypes.byref(keySize),
        ):
            return None

        return keyBuf.value[0 : keySize.value]

    def start_auto_flow_control(self, srcAddr: int, dstAddr: int):
        return ZErrorCode(
            ZXDocComm.ZXDoc_Diag_StartAutoFlowControl(
                self.__handle, self.__udsHandle, srcAddr, dstAddr
            )
        )

    def stop_auto_flow_control(self):
        return ZErrorCode(
            ZXDocComm.ZXDoc_Diag_StopAutoFlowControl(self.__handle, self.__udsHandle)
        )

    def security_access(
        self,
        saReq: ZSecurityAccessReq,
        customGenKeyFunc: Callable[[str, int, bytes, bytes], bytes] = None,
    ) -> bool:
        """
        安全访问流程
        """
        seedReq = ZUdsRequest(
            reqAddr=saReq.srcAddr,
            rspAddr=saReq.dstAddr,
            extend=saReq.isExtend,
            suppressResponse=False,
            sid=0x27,
            data=bytes([saReq.securityLevel]) + bytes(saReq.reqData),
        )

        rsp = self.request(seedReq)
        if not rsp:
            msg = self.get_error_message(rsp)
            if msg:
                print(msg)
            else:
                print(f"Failed to request, response({rsp})")
            return False

        key = None
        if customGenKeyFunc:
            key = customGenKeyFunc(
                saReq.keyDllPath,
                saReq.securityLevel,
                rsp.data[1:],
                None if saReq.variant is None else saReq.variant.encode(STR_ENCODING),
            )
        else:
            key = self.generate_security_key(
                saReq.keyDllPath, saReq.securityLevel, rsp.data[1:], saReq.variant
            )

        if key is None:
            print("Failed to generate security key.")
            return False

        keyReq = ZUdsRequest(
            reqAddr=saReq.srcAddr,
            rspAddr=saReq.dstAddr,
            extend=saReq.isExtend,
            suppressResponse=False,
            sid=0x27,
            data=bytes([saReq.securityLevel + 1]) + key,
        )

        rsp = self.request(keyReq)
        if not rsp:
            msg = self.get_error_message(rsp)
            if msg:
                print(msg)
            else:
                print(f"Failed to request, response({rsp})")
            return False

        return True

    def memoryErase(self, req: ZFileDownloadReq) -> bool:
        """
        擦除存储器
        """
        if req.memEraseType in [ZMemEraseType.NoErase, ZMemEraseType.EveryBlock]:
            return True

        if req.fileBlockCfgs is None or len(req.fileBlockCfgs) == 0:
            return False

        minMappedAddr = req.fileBlockCfgs[0].mappedAddr
        maxMappedAddr = req.fileBlockCfgs[0].mappedEndAddr()
        for blockCfg in req.fileBlockCfgs:
            minMappedAddr = min(minMappedAddr, blockCfg.mappedAddr)
            maxMappedAddr = max(maxMappedAddr, blockCfg.mappedEndAddr())

        memSize = maxMappedAddr - minMappedAddr + 1

        addrSize = req.fileBlockCfgs[0].mappedEndAddrSize()
        lenSize = ZUtility.MinLengthSize(memSize)
        addrSize = addrSize if req.addressSize < addrSize else req.addressSize
        lenSize = lenSize if req.lengthSize < lenSize else req.lengthSize
        requestDataLen = addrSize + lenSize + 4
        requestData = bytearray(requestDataLen)
        requestData[0] = 0x01
        requestData[1] = 0xFF
        requestData[2] = 0x00
        requestData[3] = (lenSize << 4) | addrSize
        for i in range(addrSize):
            requestData[4 + i] = (minMappedAddr >> ((addrSize - i - 1) * 8)) & 0xFF
        for i in range(lenSize):
            requestData[4 + addrSize + i] = (memSize >> ((lenSize - i - 1) * 8)) & 0xFF

        if ZMemEraseType.WithoutParam == req.memEraseType:
            requestData = requestData[:3]

        rsp = self.request(
            ZUdsRequest(
                reqAddr=req.srcAddr,
                rspAddr=req.dstAddr,
                extend=req.isExtend,
                suppressResponse=False,
                sid=0x31,
                data=requestData,
            )
        )

        return bool(rsp)

    @classmethod
    def MakeDlReqPdu(
        cls,
        dataFormatIdentifier: int,
        memSizeLen: int,
        memAddrLen: int,
        memoryAddress: int,
        memorySize: int,
    ) -> bytearray:
        memSizeLen &= 0x0F
        memAddrLen &= 0x0F
        dataLen = memSizeLen + memAddrLen + 2
        payload = bytearray()
        payload.append(dataFormatIdentifier)
        payload.append((memSizeLen << 4) | memAddrLen)
        for i in range(memAddrLen):
            payload.append((memoryAddress >> (memAddrLen - i - 1) * 8) & 0xFF)
        for i in range(memSizeLen):
            payload.append((memorySize >> (memSizeLen - i - 1) * 8) & 0xFF)

        return payload

    def load_file_blocks(self, filePath: str):
        blockInfo = ZXDocComm.ZXDoc_FlashFileInfo_New()
        if ZErrorCode.OK != ZXDocComm.ZXDoc_Diag_LoadFlashFileBlocks(
            self.__handle, filePath.encode(STR_ENCODING), blockInfo
        ):
            ZXDocComm.ZXDoc_FlashFileInfo_Free(blockInfo)
            return None

        flashFileInfo = ZFlashFileInfo(
            filePath=filePath,
            fileType=ZFlashFileType(ZXDocComm.ZXDoc_FlashFileInfo_GetType(blockInfo)),
        )
        blockCnt = ZXDocComm.ZXDoc_FlashFileInfo_GetBlockCount(blockInfo)
        for i in range(blockCnt):
            address = ZXDocComm.ZXDoc_U64(0)
            data = ZXDocComm.ZXDoc_UByteP()
            dataSize = ZXDocComm.ZXDoc_U32(0)
            ZXDocComm.ZXDoc_FlashFileInfo_GetBlock(
                blockInfo,
                i,
                ctypes.byref(address),
                ctypes.byref(data),
                ctypes.byref(dataSize),
            )

            block = ZFlashFileBlock()
            block.address = address.value
            block.data = bytes(data[0 : dataSize.value])
            flashFileInfo.blocks.append(block)

        ZXDocComm.ZXDoc_FlashFileInfo_Free(blockInfo)

        return flashFileInfo

    def file_download(
        self,
        req: ZFileDownloadReq,
        progressCallback: Callable[[int, int], Any] = None,
        eventCallback: Callable[[str, Dict], Any] = None,
    ) -> bool:
        """
        文件下载
        """
        if not req.isFlashFileLoaded():
            if not req.preLoadFlashFile(self):
                return False

        # 内存擦除
        if not self.memoryErase(req):
            print("Failed to erase memory")
            return False

        totalSize = 0  # 总大小
        for blockCfg in req.fileBlockCfgs:
            totalSize = totalSize + blockCfg.dataLen

        if eventCallback:
            eventCallback(
                "DownloadBegin",
                {"downloadRequest": req, "totalBytes": totalSize},
            )

        transSize = 0  # 已发送大小
        for blockCfg in req.fileBlockCfgs:
            if req.blockTransDelayMs > 0:
                time.sleep(blockTransDelayMs / 1000)

            # 发送下载请求
            lenSize = max(req.lengthSize, blockCfg.dataLenSize())
            addrSize = max(req.addressSize, blockCfg.mappedEndAddrSize())
            dlReqPdu = ZUdsInterface.MakeDlReqPdu(
                req.dataFormatIdentifier,
                lenSize,
                addrSize,
                blockCfg.mappedAddr,
                blockCfg.dataLen,
            )
            dlReq = ZUdsRequest(
                reqAddr=req.srcAddr,
                rspAddr=req.dstAddr,
                extend=req.isExtend,
                suppressResponse=False,
                sid=0x34,
                data=dlReqPdu,
            )

            if eventCallback:
                eventCallback(
                    "BeforeDownloadRequest",
                    {
                        "downloadRequest": req,
                        "blockCfg": blockCfg,
                        "request": dlReq,
                    },
                )

            rsp = self.request(dlReq)

            if not rsp or len(rsp.data) == 0:
                print(f"Invalid response: {rsp}")
                return False

            if eventCallback:
                eventCallback(
                    "AfterDownloadRequest",
                    {
                        "downloadRequest": req,
                        "blockCfg": blockCfg,
                        "request": dlReq,
                        "response": rsp,
                    },
                )

            # 处理返回的块大小，作一些兼容处理
            lenSize = rsp.data[0] >> 4
            if 0 == lenSize:
                lenSize = min(len(rsp.data) - 1, 8)
                tmpMaxBlockLen = 0
                for i in range(lenSize):
                    tmpMaxBlockLen = (tmpMaxBlockLen << 8) | rsp.data[1 + i]

                if tmpMaxBlockLen > 0xFFFFFFFF:
                    print(f"Invalid block size: {tmpMaxBlockLen}")
                    return False
            else:
                if lenSize > (len(rsp.data) - 1):
                    lenSize = len(rsp.data) - 1
                    if 0 == lenSize:
                        return False

            maxBlockLen = 0
            for i in range(lenSize):
                maxBlockLen = (maxBlockLen << 8) | rsp.data[1 + i]

            if 0 == maxBlockLen:
                print(f"Block size can not be zero.")
                return False

            # 发送每个文件块
            requestData = bytearray(maxBlockLen)
            index = 0
            len2send = 0
            blocknumber = 1
            blockData = blockCfg.getData(req.flashFileInfo.blocks)

            if eventCallback:
                eventCallback(
                    "BeforeBlockDataTransmit",
                    {
                        "downloadRequest": req,
                        "blockCfg": blockCfg,
                        "blockData": blockData,
                        "blocknumber": blocknumber,
                    },
                )

            while index < blockCfg.dataLen:
                len2send = (
                    (maxBlockLen - 2)
                    if (blockCfg.dataLen - index > maxBlockLen - 2)
                    else (blockCfg.dataLen - index)
                )
                data = blockData[index : (index + len2send)]

                reqData = bytes([blocknumber]) + data

                retry = 0
                dataTransfered = False
                while retry <= req.blockTransRetryCount:
                    retry += 1

                    rsp = self.request(
                        ZUdsRequest(
                            reqAddr=req.srcAddr,
                            rspAddr=req.dstAddr,
                            extend=req.isExtend,
                            suppressResponse=False,
                            sid=0x36,
                            data=reqData,
                        )
                    )

                    if not rsp or len(rsp.data) < 1:
                        print(f"Invalid response: {rsp}")
                        continue

                    if rsp.data[0] != blocknumber:
                        continue

                    dataTransfered = True
                    break

                if not dataTransfered:
                    print(f"Failed to send file block")
                    return False

                index += len2send
                blocknumber = (blocknumber + 1) % 256
                transSize += len2send
                if progressCallback:
                    progressCallback(transSize, totalSize)

                if req.s36IntervalMs > 0:
                    time.sleep(req.s36IntervalMs / 1000)

            if eventCallback:
                eventCallback(
                    "AfterBlockDataTransmit",
                    {
                        "downloadRequest": req,
                        "blockCfg": blockCfg,
                        "blockData": blockData,
                        "blocknumber": blocknumber,
                    },
                )

            crcBuffer = struct.pack(">Q", blockCfg.crc)[
                8 - req.crcAlgorithm.crcValueByteLength() :
            ]

            # 发送结束传输指令 $37
            if ZTransExitCmdType.Normal == req.transExitCmdType:
                rsp = self.request(
                    ZUdsRequest(
                        reqAddr=req.srcAddr,
                        rspAddr=req.dstAddr,
                        extend=req.isExtend,
                        suppressResponse=False,
                        sid=0x37,
                    )
                )

                if not rsp:
                    return False
            # 发送结束传输指令 $37 + 自定义数据
            elif ZTransExitCmdType.Custom == req.transExitCmdType:
                rsp = self.request(
                    ZUdsRequest(
                        reqAddr=req.srcAddr,
                        rspAddr=req.dstAddr,
                        extend=req.isExtend,
                        suppressResponse=False,
                        sid=0x37,
                        data=bytes(req.customTransExitCmd),
                    )
                )

                if not rsp:
                    return False
            # 发送结束传输指令 $37 + 文件块CRC
            elif ZTransExitCmdType.WithBlockCrc == req.transExitCmdType:
                rsp = self.request(
                    ZUdsRequest(
                        reqAddr=req.srcAddr,
                        rspAddr=req.dstAddr,
                        extend=req.isExtend,
                        suppressResponse=False,
                        sid=0x37,
                        data=crcBuffer,
                    )
                )

                if not rsp:
                    return False

            if eventCallback:
                eventCallback(
                    "AfterBlockTransExit",
                    {
                        "downloadRequest": req,
                        "blockCfg": blockCfg,
                        "blockData": blockData,
                        "blocknumber": blocknumber,
                    },
                )

            # 自定义的块校验指令
            if (
                req.customBlockCrcCheckCmd is not None
                and len(req.customBlockCrcCheckCmd) > 0
            ):
                cmd = bytes(req.customBlockCrcCheckCmd)
                if len(cmd) < 2:
                    print("Invalid custom block CRC check command.")
                    return False

                if eventCallback:
                    eventCallback(
                        "BeforeCustomBlockCrcCheck",
                        {
                            "downloadRequest": req,
                            "blockCfg": blockCfg,
                            "blockData": blockData,
                            "blocknumber": blocknumber,
                        },
                    )

                rsp = self.request(
                    ZUdsRequest(
                        reqAddr=req.srcAddr,
                        rspAddr=req.dstAddr,
                        extend=req.isExtend,
                        suppressResponse=False,
                        sid=cmd[0],
                        data=cmd[1:] + crcBuffer,
                    )
                )

                if not rsp:
                    print("Block CRC verification failed.")
                    return False

                if eventCallback:
                    eventCallback(
                        "AfterCustomBlockCrcCheck",
                        {
                            "downloadRequest": req,
                            "blockCfg": blockCfg,
                            "blockData": blockData,
                            "blocknumber": blocknumber,
                        },
                    )

        # 文件总校验
        if req.totalCheckCmd is not None and len(req.totalCheckCmd) > 0:
            cmd = bytes(req.totalCheckCmd)
            if len(cmd) < 2:
                print("Invalid total check command.")
                return False

            if eventCallback:
                eventCallback(
                    "BeforeTotalCheck",
                    {"downloadRequest": req, "crc": req.totalCrc},
                )

            crcBuffer = struct.pack(">Q", req.totalCrc)[
                8 - req.crcAlgorithm.crcValueByteLength() :
            ]

            rsp = self.request(
                ZUdsRequest(
                    reqAddr=req.srcAddr,
                    rspAddr=req.dstAddr,
                    extend=req.isExtend,
                    suppressResponse=False,
                    sid=cmd[0],
                    data=cmd[1:] + crcBuffer,
                )
            )

            if not rsp:
                print("Total CRC verification failed.")
                return False

            if eventCallback:
                eventCallback(
                    "AfterTotalCheck",
                    {"downloadRequest": req, "crc": req.totalCrc},
                )

        if eventCallback:
            eventCallback(
                "DownloadFinished",
                {"downloadRequest": req},
            )

        return True


# CLI
@unique
class ZCliReturnCode(IntEnum):
    Success = 0
    Failed = 1
    NotSupported = 2
    ParamIsInvalid = 3
    UserDefine = 100
    Unknown = -1


@dataclass
class ZCliResponse:
    returnCode: ZCliReturnCode = ZCliReturnCode.Unknown
    message: str = ""
    values: Dict[str, str] = field(default_factory=dict)


class ZXDoc:
    """
    ZXDoc主类
    """

    def __init__(self):
        self.__handle = ZXDocComm.ZXDoc_Create()
        if ZXDocComm.ZXDOC_INVALID_HANDLE == self.__handle:
            raise RuntimeError("Failed to create ZXDoc handle.")

    def __del__(self):
        ZXDocComm.ZXDoc_Free(self.__handle)

    @classmethod
    def _build_time(cls) -> str:
        return ZXDocComm.ZXDoc_BuildTime().decode(STR_ENCODING)

    @classmethod
    def _version(cls) -> str:
        return ZXDocComm.ZXDoc_Version().decode(STR_ENCODING)

    def connect(
        self, projectFilePath: str = "", noTrayIcon: bool = False
    ) -> ZErrorCode:
        """
        连接ZXDoc

        如果指定了projectFilePath参数，则连接之后加载该工程。
        """
        return ZErrorCode(
            ZXDocComm.ZXDoc_Connect(
                self.__handle,
                (
                    None
                    if projectFilePath is None
                    else projectFilePath.encode(STR_ENCODING)
                ),
                ZXDocComm.ZXDoc_True if noTrayIcon else ZXDocComm.ZXDoc_False,
            )
        )

    def disconnect(self) -> ZErrorCode:
        """
        断开ZXDoc连接
        """
        return ZErrorCode(ZXDocComm.ZXDoc_Disonnect(self.__handle))

    def on_connected(self, callback) -> ZErrorCode:
        """
        设置连接回调
        """
        self.__on_connected_callback = ctypes.CFUNCTYPE(None)(callback)
        return ZErrorCode(
            ZXDocComm.ZXDoc_SetOnConnectedCallback(
                self.__handle, self.__on_connected_callback, ctypes.c_void_p(0)
            )
        )

    def on_disconnected(self, callback) -> ZErrorCode:
        """
        设置连接断开回调
        """
        self.__on_disconnected_callback = ctypes.CFUNCTYPE(None)(callback)
        return ZErrorCode(
            ZXDocComm.ZXDoc_SetOnDisconnectedCallback(
                self.__handle, self.__on_disconnected_callback, ctypes.c_void_p(0)
            )
        )

    def log_d(self, o: Any) -> ZErrorCode:
        """
        在ZXDoc上输出日志
        """
        return ZErrorCode(
            ZXDocComm.ZXDoc_App_Log(
                self.__handle, ZXDocComm.ZXDOC_LOG_LVL_DBG, str(o).encode(STR_ENCODING)
            )
        )

    def log_i(self, o: Any) -> ZErrorCode:
        """
        在ZXDoc上输出日志
        """
        return ZErrorCode(
            ZXDocComm.ZXDoc_App_Log(
                self.__handle, ZXDocComm.ZXDOC_LOG_LVL_INFO, str(o).encode(STR_ENCODING)
            )
        )

    def log_w(self, o: Any) -> ZErrorCode:
        """
        在ZXDoc上输出日志
        """
        return ZErrorCode(
            ZXDocComm.ZXDoc_App_Log(
                self.__handle,
                ZXDocComm.ZXDOC_LOG_LVL_WARNING,
                str(o).encode(STR_ENCODING),
            )
        )

    def log_e(self, o: Any) -> ZErrorCode:
        """
        在ZXDoc上输出日志
        """
        return ZErrorCode(
            ZXDocComm.ZXDoc_App_Log(
                self.__handle, ZXDocComm.ZXDOC_LOG_LVL_ERR, str(o).encode(STR_ENCODING)
            )
        )

    def log_c(self, o: Any) -> ZErrorCode:
        """
        在ZXDoc上输出日志
        """
        return ZErrorCode(
            ZXDocComm.ZXDoc_App_Log(
                self.__handle,
                ZXDocComm.ZXDOC_LOG_LVL_CRITICAL,
                str(o).encode(STR_ENCODING),
            )
        )

    def clear_log(self) -> ZErrorCode:
        """
        清空ZXDoc界面上的日志
        """
        return ZErrorCode(ZXDocComm.ZXDoc_App_ClearLog(self.__handle))

    def export_log(self, logFilePath: str) -> ZErrorCode:
        """
        导出ZXDoc的日志
        """
        return ZErrorCode(
            ZXDocComm.ZXDoc_App_ExportLog(
                self.__handle, logFilePath.encode(STR_ENCODING)
            )
        )

    def panel_log_d(self, name: str, o: Any) -> ZErrorCode:
        """
        在ZXDoc面板上输出日志
        """
        return ZErrorCode(
            ZXDocComm.ZXDoc_Panel_Log(
                self.__handle,
                str(name).encode(STR_ENCODING),
                ZXDocComm.ZXDOC_LOG_LVL_DBG,
                str(o).encode(STR_ENCODING),
            )
        )

    def panel_log_i(self, name: str, o: Any) -> ZErrorCode:
        """
        在ZXDoc面板上输出日志
        """
        return ZErrorCode(
            ZXDocComm.ZXDoc_Panel_Log(
                self.__handle,
                str(name).encode(STR_ENCODING),
                ZXDocComm.ZXDOC_LOG_LVL_INFO,
                str(o).encode(STR_ENCODING),
            )
        )

    def panel_log_w(self, name: str, o: Any) -> ZErrorCode:
        """
        在ZXDoc面板上输出日志
        """
        return ZErrorCode(
            ZXDocComm.ZXDoc_Panel_Log(
                self.__handle,
                str(name).encode(STR_ENCODING),
                ZXDocComm.ZXDOC_LOG_LVL_WARNING,
                str(o).encode(STR_ENCODING),
            )
        )

    def panel_log_e(self, name: str, o: Any) -> ZErrorCode:
        """
        在ZXDoc面板上输出日志
        """
        return ZErrorCode(
            ZXDocComm.ZXDoc_Panel_Log(
                self.__handle,
                str(name).encode(STR_ENCODING),
                ZXDocComm.ZXDOC_LOG_LVL_ERR,
                str(o).encode(STR_ENCODING),
            )
        )

    def panel_log_c(self, name: str, o: Any) -> ZErrorCode:
        """
        在ZXDoc面板上输出日志
        """
        return ZErrorCode(
            ZXDocComm.ZXDoc_Panel_Log(
                self.__handle,
                str(name).encode(STR_ENCODING),
                ZXDocComm.ZXDOC_LOG_LVL_CRITICAL,
                str(o).encode(STR_ENCODING),
            )
        )

    def get_current_project_path(self) -> str:
        bufSize = ZXDocComm.ZXDoc_U32(1024)
        buf = (ZXDocComm.ZXDoc_Char * bufSize.value)()
        errCode = ZErrorCode(
            ZXDocComm.ZXDoc_App_GetCurrentProjectPath(self.__handle, buf, bufSize)
        )
        if ZErrorCode.OK != errCode:
            return ""
        return buf.value.decode(STR_ENCODING)

    def get_app_version(self) -> str:
        """
        获取ZXDoc的软件版本号
        版本号的格式为“1.2.5-beta.1”或者“1.2.5”
        """
        bufSize = ZXDocComm.ZXDoc_U32(256)
        buf = (ZXDocComm.ZXDoc_Char * bufSize.value)()
        errCode = ZErrorCode(
            ZXDocComm.ZXDoc_App_GetVersion(self.__handle, buf, bufSize)
        )
        if ZErrorCode.OK != errCode:
            return ""
        return buf.value.decode(STR_ENCODING)

    def load_project(self, projectFilePath: str) -> bool:
        return ZErrorCode.OK == ZErrorCode(
            ZXDocComm.ZXDoc_App_LoadProject(
                self.__handle, projectFilePath.encode(STR_ENCODING)
            )
        )

    def show_main_window(self) -> bool:
        return ZErrorCode.OK == ZErrorCode(
            ZXDocComm.ZXDoc_App_ShowMainWindow(self.__handle)
        )

    def hide_main_window(self) -> bool:
        return ZErrorCode.OK == ZErrorCode(
            ZXDocComm.ZXDoc_App_HideMainWindow(self.__handle)
        )

    def close_main_window(self) -> bool:
        return ZErrorCode.OK == ZErrorCode(
            ZXDocComm.ZXDoc_App_CloseMainWindow(self.__handle)
        )

    def subwnd_cmd_exec(
        self,
        subwindowIndex: int,
        cmeline: str,
        waitForFinished: bool = False,
        maxResponseSize: int = 1024,
    ) -> str:
        handle = ZXDocComm.ZCliResponseHandle()
        errCode = ZErrorCode(
            ZXDocComm.ZXDoc_App_SubwndCmdExec(
                self.__handle,
                subwindowIndex,
                cmeline.encode(STR_ENCODING),
                waitForFinished,
                ctypes.pointer(handle),
            )
        )

        if ZErrorCode.OK != errCode:
            return ""

        rsp = ZCliResponse()
        rsp.returnCode = ZCliReturnCode(ZXDocComm.ZXDoc_CliResp_GetReturnCode(handle))
        rsp.message = ZXDocComm.ZXDoc_CliResp_GetMessage(handle).decode(STR_ENCODING)
        i = 0
        key = ZXDocComm.ZXDoc_CharP()
        value = ZXDocComm.ZXDoc_CharP()
        while ZXDocComm.ZXDoc_CliResp_GetValue(
            handle, i, ctypes.pointer(key), ctypes.pointer(value)
        ):
            v = None
            if value:
                v = value.value.decode(STR_ENCODING)
            rsp.values[key.value.decode(STR_ENCODING)] = v
            i += 1

        ZXDocComm.ZXDoc_CliResp_Free(handle)

        return rsp

    def add_user_variable(self, var: ZUserVariable) -> ZErrorCode:
        """
        添加一个自定义变量
        """
        return ZErrorCode(
            ZXDocComm.ZXDoc_App_AddUserVariable(
                self.__handle,
                ZXDocComm.ZXDoc_UserVariable(
                    name=var.name.encode(STR_ENCODING),
                    group=var.group.encode(STR_ENCODING),
                    valueType=var.valueType.value,
                    unit=var.unit.encode(STR_ENCODING),
                    initValue=var.initValue.encode(STR_ENCODING),
                    minValue=var.minValue.encode(STR_ENCODING),
                    maxValue=var.maxValue.encode(STR_ENCODING),
                    comment=var.comment.encode(STR_ENCODING),
                ),
            )
        )

    def del_user_variable(self, varName: str, group: str) -> ZErrorCode:
        """
        删除一个自定义变量
        """
        return ZErrorCode(
            ZXDocComm.ZXDoc_App_DelUserVariable(
                self.__handle, varName.encode(STR_ENCODING), group.encode(STR_ENCODING)
            )
        )

    def start_measurement(self) -> ZErrorCode:
        """
        启动测量
        """
        return ZErrorCode(ZXDocComm.ZXDoc_Meas_Start(self.__handle))

    def stop_measurement(self) -> ZErrorCode:
        """
        停止测量
        """
        return ZErrorCode(ZXDocComm.ZXDoc_Meas_Stop(self.__handle))

    def is_measurement_started(self) -> bool:
        """
        测量是否已启动
        """
        return ZXDocComm.ZXDoc_Meas_IsStarted(self.__handle)

    def on_measurement_state_changed(self, callback) -> ZErrorCode:
        """
        设置测量状态变更回调

        'callback'参数定义如下:
            def measurement_state_changed(status: int):
                pass

            status: 1 测量启动， 0 测量停止
        """
        self.__on_measurement_state_changed_callback = (
            ZXDocComm.ZXDoc_Meas_StatChangedCallbackType(callback)
        )
        return ZErrorCode(
            ZXDocComm.ZXDoc_Meas_SetStatChangedCallback(
                self.__handle,
                self.__on_measurement_state_changed_callback,
                ctypes.c_void_p(0),
            )
        )

    def __signals_observer_handler(self, signalValues, count, context):
        if self.__signals_observer_callback is None:
            return

        l = list()
        for i in range(count):
            cval = signalValues[i].contents
            val = ZSignalValue(
                sourceId=cval.sourceId.decode(STR_ENCODING),
                signalId=cval.signalId.decode(STR_ENCODING),
                frameNumber=cval.frameNumber,
                timestamp=cval.timestamp,
                rawValue=ZXDocComm.ZXDoc_SignalVariant_To_SimpleValue(cval.rawValue),
                phyValue=ZXDocComm.ZXDoc_SignalVariant_To_SimpleValue(cval.phyValue),
                rowIndex=cval.rowIndex,
                colIndex=cval.colIndex,
            )
            l.append(val)
        self.__signals_observer_callback(l)

    def set_signals_observer(self, callback) -> ZErrorCode:
        """
        设置信号观察者回调

        'callback'参数定义如下:
            def on_data_sink(rawDatas: List[ZSignalValue]):
                pass
        """
        self.__signals_observer_callback = callback
        self.__signals_observer_callback_obj = (
            ZXDocComm.ZXDoc_Signal_SetSignalsObserverCallbackType()
            if callback is None
            else ZXDocComm.ZXDoc_Signal_SetSignalsObserverCallbackType(
                self.__signals_observer_handler
            )
        )
        return ZErrorCode(
            ZXDocComm.ZXDoc_Signal_SetSignalsObserver(
                self.__handle,
                self.__signals_observer_callback_obj,
                ctypes.c_void_p(0),
            )
        )

    def subscribe_signals(self, signalIds: List[Tuple[str, str]]) -> ZErrorCode:
        """
        订阅信号
        """
        cnt = len(signalIds)
        ids = (ZXDocComm.ZXDoc_SignalIdentifier * cnt)()

        for i in range(cnt):
            ids[i] = ZXDocComm.ZXDoc_SignalIdentifier(
                signalIds[i][0].encode(STR_ENCODING),
                signalIds[i][1].encode(STR_ENCODING),
            )

        return ZErrorCode(
            ZXDocComm.ZXDoc_Signal_Subscribe(
                self.__handle,
                ids,
                cnt,
            )
        )

    def unsubscribe_signals(self, signalIds: List[Tuple[str, str]]) -> ZErrorCode:
        """
        取消订阅信号
        """
        cnt = len(signalIds)
        ids = (ZXDocComm.ZXDoc_SignalIdentifier * cnt)()

        for i in range(cnt):
            ids[i] = ZXDocComm.ZXDoc_SignalIdentifier(
                signalIds[i][0].encode(STR_ENCODING),
                signalIds[i][1].encode(STR_ENCODING),
            )

        return ZErrorCode(
            ZXDocComm.ZXDoc_Signal_Unsubscribe(
                self.__handle,
                ids,
                cnt,
            )
        )

    def set_signal_value_int(
        self,
        sourceId: str,
        signalId: str,
        phyValue: int,
        rowIndex: int = -1,
        colIndex: int = -1,
    ) -> ZErrorCode:
        """
        设置信号的值（有符号整数值）
        """
        return ZErrorCode(
            ZXDocComm.ZXDoc_Signal_SetValueInt(
                self.__handle,
                sourceId.encode(STR_ENCODING),
                signalId.encode(STR_ENCODING),
                phyValue,
                rowIndex,
                colIndex,
            )
        )

    def set_signal_value_uint(
        self,
        sourceId: str,
        signalId: str,
        phyValue: int,
        rowIndex: int = -1,
        colIndex: int = -1,
    ) -> ZErrorCode:
        """
        设置信号的值（无符号整数值）
        """
        return ZErrorCode(
            ZXDocComm.ZXDoc_Signal_SetValueUint(
                self.__handle,
                sourceId.encode(STR_ENCODING),
                signalId.encode(STR_ENCODING),
                phyValue,
                rowIndex,
                colIndex,
            )
        )

    def set_signal_value_double(
        self,
        sourceId: str,
        signalId: str,
        phyValue: float,
        rowIndex: int = -1,
        colIndex: int = -1,
    ) -> ZErrorCode:
        """
        设置信号的值（浮点数）
        """
        return ZErrorCode(
            ZXDocComm.ZXDoc_Signal_SetValueDouble(
                self.__handle,
                sourceId.encode(STR_ENCODING),
                signalId.encode(STR_ENCODING),
                phyValue,
                rowIndex,
                colIndex,
            )
        )

    def set_signal_value_str(
        self,
        sourceId: str,
        signalId: str,
        phyValue: str,
        rowIndex: int = -1,
        colIndex: int = -1,
    ) -> ZErrorCode:
        """
        设置信号的值（字符串）
        """
        return ZErrorCode(
            ZXDocComm.ZXDoc_Signal_SetValueString(
                self.__handle,
                sourceId.encode(STR_ENCODING),
                signalId.encode(STR_ENCODING),
                phyValue.encode(STR_ENCODING),
                rowIndex,
                colIndex,
            )
        )

    def set_signal_value(
        self,
        sourceId: str,
        signalId: str,
        phyValue: Any,
        rowIndex: int = -1,
        colIndex: int = -1,
    ) -> ZErrorCode:
        """
        设置信号的值
        """
        if type(phyValue) == int:
            if phyValue > 0x7FFFFFFFFFFFFFFF:
                return self.set_signal_value_uint(
                    sourceId, signalId, phyValue, rowIndex, colIndex
                )
            else:
                return self.set_signal_value_int(
                    sourceId, signalId, phyValue, rowIndex, colIndex
                )

        if type(phyValue) == float:
            return self.set_signal_value_double(
                sourceId, signalId, phyValue, rowIndex, colIndex
            )

        if type(phyValue) == str:
            return self.set_signal_value_str(
                sourceId, signalId, phyValue, rowIndex, colIndex
            )

        return ZErrorCode.INVALID_PARAM

    def set_signals_value(self, signalValues: List[ZSignalValue]) -> ZErrorCode:
        """
        设置信号的值
        """
        size = len(signalValues)
        values = (ZXDocComm.PZXDoc_SignalValue * size)()

        for i in range(size):
            srcVal = signalValues[i]
            v = ZXDocComm.ZXDoc_SignalValue_New()
            values[i] = v

            v.contents.sourceId = srcVal.sourceId.encode(STR_ENCODING)
            v.contents.signalId = srcVal.signalId.encode(STR_ENCODING)
            v.contents.timestamp = srcVal.timestamp
            v.contents.frameNumber = srcVal.frameNumber
            v.contents.rowIndex = srcVal.rowIndex
            v.contents.colIndex = srcVal.colIndex
            v.contents.rawValue = None
            v.contents.phyValue = ZXDocComm.ZXDoc_SignalVariant_New()
            ZXDocComm.ZXDoc_SignalVariant_Init(v.contents.phyValue)

            phyValue = srcVal.phyValue

            if type(phyValue) == int:
                if phyValue > 0x7FFFFFFFFFFFFFFF:
                    ZXDocComm.ZXDoc_SignalVariant_SetUint64(
                        v.contents.phyValue, phyValue
                    )
                else:
                    ZXDocComm.ZXDoc_SignalVariant_SetInt64(
                        v.contents.phyValue, phyValue
                    )

            if type(phyValue) == float:
                ZXDocComm.ZXDoc_SignalVariant_SetDouble(v.contents.phyValue, phyValue)

            if type(phyValue) == str:
                ZXDocComm.ZXDoc_SignalVariant_SetStr(
                    v.contents.phyValue, phyValue.encode(STR_ENCODING)
                )

        ZXDocComm.ZXDoc_Signal_SetValues(self.__handle, values, size)

        for v in values:
            ZXDocComm.ZXDoc_SignalValue_Free(v)

        return ZErrorCode.INVALID_PARAM

    def get_signal_value(
        self,
        sourceId: str,
        signalId: str,
        rowIndex: int = -1,
        colIndex: int = -1,
    ) -> Optional[Union[int, str, float, List[List[Union[int, str, float]]]]]:
        """
        获取信号的值
        """
        pSglValue = ZXDocComm.PZXDoc_SignalValue()

        re = ZXDocComm.ZXDoc_Signal_GetValue(
            self.__handle,
            sourceId.encode(STR_ENCODING),
            signalId.encode(STR_ENCODING),
            ctypes.byref(pSglValue),
            rowIndex,
            colIndex,
        )
        if ZErrorCode.OK != re:
            return None

        re = ZXDocComm.ZXDoc_SignalVariant_To_SimpleValue(pSglValue.contents.phyValue)
        ZXDocComm.ZXDoc_SignalValue_Free(pSglValue)

        return re

    def get_signal_init_value(
        self,
        sourceId: str,
        signalId: str,
        rowIndex: int = -1,
        colIndex: int = -1,
    ) -> Optional[Union[int, str, float, List[List[Union[int, str, float]]]]]:
        """
        获取信号的值
        """
        pSglValue = ZXDocComm.PZXDoc_SignalValue()

        re = ZXDocComm.ZXDoc_Signal_GetInitValue(
            self.__handle,
            sourceId.encode(STR_ENCODING),
            signalId.encode(STR_ENCODING),
            ctypes.byref(pSglValue),
            rowIndex,
            colIndex,
        )
        if ZErrorCode.OK != re:
            return None

        re = ZXDocComm.ZXDoc_SignalVariant_To_SimpleValue(pSglValue.contents.phyValue)
        ZXDocComm.ZXDoc_SignalValue_Free(pSglValue)

        return re

    def get_channels(self, busType: ZBusType) -> List[ZChannel]:
        """
        获取已有通道
        """
        pChnls = ZXDocComm.ZXDoc_ChannelP()
        if ZErrorCode.OK != ZXDocComm.ZXDoc_Chnl_GetChannels(
            self.__handle, busType, ctypes.byref(pChnls)
        ):
            return None

        l = list()
        i = 0
        while pChnls[i]:
            chnl = pChnls[i]

            dbs = []
            for n in range(ZXDocComm.ZXDoc_Channel_GetDatabaseCount(chnl)):
                dbs.append(
                    ZXDocComm.ZXDoc_Channel_GetDatabase(chnl, n).decode(STR_ENCODING)
                )

            l.append(
                ZChannel(
                    busType=ZXDocComm.ZXDoc_Channel_GetBusType(chnl),
                    logicalIndex=ZXDocComm.ZXDoc_Channel_GetLogicalIndex(chnl),
                    physicalIndex=ZXDocComm.ZXDoc_Channel_GetPhysicalIndex(chnl),
                    enabled=ZXDocComm.ZXDoc_Channel_IsEnabled(chnl),
                    activated=ZXDocComm.ZXDoc_Channel_IsActivated(chnl),
                    databases=dbs,
                )
            )
            i += 1

        ZXDocComm.ZXDoc_Channels_Free(pChnls)

        return l

    def lin_wakeup(self, logicalIndex: int) -> bool:
        return ZErrorCode.OK == ZXDocComm.ZXDoc_Chnl_LinWakeUp(
            self.__handle, logicalIndex
        )

    def get_available_can_tx_queue_count(self, logicalIndex: int) -> int:
        """
        获取发送队列可用数量
        """
        count = ZXDocComm.ZXDoc_I32(0)
        if ZErrorCode.OK != ZXDocComm.ZXDoc_Chnl_GetAvailableCanTxQueueCount(
            self.__handle, logicalIndex, ctypes.byref(count)
        ):
            return -1
        return count.value

    def clear_can_tx_queue(self, logicalIndex: int) -> int:
        """
        清空CAN发送队列
        """
        result = ZXDocComm.ZXDoc_Bool(ZXDocComm.ZXDoc_False)
        if ZErrorCode.OK != ZXDocComm.ZXDoc_Chnl_ClearCanTxQueue(
            self.__handle, logicalIndex, ctypes.byref(result)
        ):
            return False
        return ZXDocComm.ZXDoc_True == result.value

    def can_queue_transmit(
        self, channelIndex: int, canFdDatas: List[ZCANFDData]
    ) -> bool:
        """
        CAN队列发送
        """
        rawDatas = []
        for data in canFdDatas:
            if data is None:
                continue

            _rawData = ZXDocComm.ZXDoc_RawData_New_CANFDData()
            _rawData[0].channel = channelIndex

            pCanfdData = ZXDocComm.ZXDoc_RawData_Get_CANFDData(_rawData)
            _canfdData = pCanfdData.contents
            _canfdData.can_id = data.can_id
            _canfdData.EFF(data.EFF)
            _canfdData.FDF(data.FDF)
            _canfdData.RTR(data.RTR)
            _canfdData.BRS(data.BRS)
            _canfdData.ESI(data.ESI)
            _canfdData.txDelayUnitType(int(data.tx_delay_unit_type))
            _canfdData.txDelayTime(data.tx_delay)
            _canfdData.len = len(data.data)
            length = min(len(_canfdData.data), _canfdData.len)
            for i in range(length):
                _canfdData.data[i] = data.data[i]

            rawDatas.append(_rawData)

        cnt = len(rawDatas)
        if 0 == cnt:
            return False

        _rawDatas = (ZXDocComm.PZXDoc_RawData * cnt)()
        for i in range(cnt):
            _rawDatas[i] = rawDatas[i]
        result = ZXDocComm.ZXDoc_Bool(ZXDocComm.ZXDoc_False)

        re = ZXDocComm.ZXDoc_Chnl_CanQueueTransmit(
            self.__handle, _rawDatas, cnt, ctypes.byref(result)
        )

        # TODO Don't forget to free the memory
        for rd in _rawDatas:
            ZXDocComm.ZXDoc_RawData_Free(rd)

        if ZErrorCode.OK != re:
            return False

        return result.value

    def scheduled_frame_capacity(self, logicalIndex: int) -> int:
        """获取设备定时发送的报文容量"""
        capacity = ZXDocComm.ZXDoc_I32(0)
        ZXDocComm.ZXDoc_Chnl_ScheduledFrameCapacity(
            self.__handle, logicalIndex, ctypes.byref(capacity)
        )
        return capacity.value

    def started_scheduled_frame_count(self, logicalIndex: int) -> int:
        """获取已启动定时发送的报文数量"""
        count = ZXDocComm.ZXDoc_I32(0)
        ZXDocComm.ZXDoc_Chnl_StartedScheduledFrameCount(
            self.__handle, logicalIndex, ctypes.byref(count)
        )
        return count.value

    def start_scheduled_frame(
        self, logicalIndex: int, data: ZCANFDData, interval: int
    ) -> int:
        """启动定时发送"""
        _canFdData = ZXDocComm.ZXDoc_CANFDData()
        index = ZXDocComm.ZXDoc_I32(0)

        _canFdData.can_id = data.can_id
        _canFdData.EFF(data.EFF)
        _canFdData.FDF(data.FDF)
        _canFdData.RTR(data.RTR)
        _canFdData.BRS(data.BRS)
        _canFdData.ESI(data.ESI)
        _canFdData.len = min(len(data.data), 64)
        for i in range(_canFdData.len):
            _canFdData.data[i] = data.data[i]

        ZXDocComm.ZXDoc_Chnl_StartScheduledFrame(
            self.__handle,
            logicalIndex,
            ctypes.byref(_canFdData),
            interval,
            ctypes.byref(index),
        )
        return index.value

    def restart_scheduled_frame(
        self, logicalIndex: int, index: int, data: ZCANFDData, interval: int
    ) -> int:
        """重新启动定时发送"""
        _canFdData = ZXDocComm.ZXDoc_CANFDData()
        _index = ZXDocComm.ZXDoc_I32(index)

        _canFdData.can_id = data.can_id
        _canFdData.EFF(data.EFF)
        _canFdData.FDF(data.FDF)
        _canFdData.RTR(data.RTR)
        _canFdData.BRS(data.BRS)
        _canFdData.ESI(data.ESI)
        _canFdData.len = min(len(data.data), 64)
        for i in range(_canFdData.len):
            _canFdData.data[i] = data.data[i]

        ZXDocComm.ZXDoc_Chnl_RestartScheduledFrame(
            self.__handle,
            logicalIndex,
            ctypes.byref(_index),
            ctypes.byref(_canFdData),
            interval,
        )
        return _index.value

    def stop_scheduled_frame(self, index: int) -> bool:
        """停止定时发送"""
        return ZErrorCode.OK == ZXDocComm.ZXDoc_Chnl_StopScheduledFrame(
            self.__handle, index
        )

    def transmit(self, rawDatas: List[ZRawData]) -> int:
        """
        报文发送
        """
        cnt = len(rawDatas)
        if 0 == cnt:
            return 0

        _rawDatas = (ZXDocComm.PZXDoc_RawData * cnt)()
        _cnt = ctypes.c_uint32(0)
        for i in range(cnt):
            data = rawDatas[i]
            if data.data is None:
                continue

            if type(data.data) == ZCANFDData:
                _rawData = ZXDocComm.ZXDoc_RawData_New_CANFDData()
                _rawData[0].number = data.number
                _rawData[0].absoluteTimestamp = data.absoluteTimestamp
                _rawData[0].relativeTimestamp = data.relativeTimestamp
                _rawData[0].channel = data.channel

                pCanfdData = ZXDocComm.ZXDoc_RawData_Get_CANFDData(_rawData)
                canfdData = pCanfdData.contents
                canfdData.can_id = data.data.can_id
                canfdData.EFF(data.data.EFF)
                canfdData.FDF(data.data.FDF)
                canfdData.RTR(data.data.RTR)
                canfdData.BRS(data.data.BRS)
                canfdData.ESI(data.data.ESI)
                canfdData.len = len(data.data.data)
                length = min(len(canfdData.data), canfdData.len)
                for i in range(length):
                    canfdData.data[i] = data.data.data[i]

                _rawDatas[_cnt.value] = _rawData
                _cnt.value += 1
            elif type(data.data) == ZLINData:
                _rawData = ZXDocComm.ZXDoc_RawData_New_LINData()
                _rawData[0].number = data.number
                _rawData[0].absoluteTimestamp = data.absoluteTimestamp
                _rawData[0].relativeTimestamp = data.relativeTimestamp
                _rawData[0].channel = data.channel

                pLinData = ZXDocComm.ZXDoc_RawData_Get_LINData(_rawData)
                linData = pLinData.contents
                linData.ID = data.data.ID
                linData.timestamp = data.data.timestamp
                linData.chksum = data.data.chksum
                linData.direction = data.data.direction
                linData.frameType = data.data.frameType
                linData.dataLen = len(data.data.data)
                length = min(len(linData.data), linData.dataLen)
                for i in range(length):
                    linData.data[i] = data.data.data[i]

                _rawDatas[_cnt.value] = _rawData
                _cnt.value += 1
            else:
                continue

        if 0 == _cnt:
            return 0

        re = ZXDocComm.ZXDoc_Chnl_Transmit(self.__handle, _rawDatas, ctypes.byref(_cnt))

        # TODO Don't forget to free the memory
        for rd in _rawDatas:
            ZXDocComm.ZXDoc_RawData_Free(rd)

        if ZErrorCode.OK != re:
            return 0
        return _cnt.value

    def __on_data_sink_handler(self, rawDatas, count, context):
        l = list()
        for i in range(count):
            _rawData = rawDatas[i].contents
            rawData = ZRawData(
                number=_rawData.number,
                absoluteTimestamp=_rawData.absoluteTimestamp,
                relativeTimestamp=_rawData.relativeTimestamp,
                channel=_rawData.channel,
            )

            if ZXDocComm.ZXDoc_RawDataType_CANFD == _rawData.type:
                _canFdData = ZXDocComm.PZXDoc_CANFDData(_rawData.data).contents
                canFdData = ZCANFDData(
                    can_id=_canFdData.can_id,
                    EFF=_canFdData.EFF(),
                    FDF=_canFdData.FDF(),
                    RTR=_canFdData.RTR(),
                    BRS=_canFdData.BRS(),
                    ESI=_canFdData.ESI(),
                    direction=ZTransmitDirection(_canFdData.direction()),
                    tx_delay_unit_type=_canFdData.txDelayUnitType(),
                    tx_delay=_canFdData.txDelayTime(),
                    transmitType=_canFdData.transmitType(),
                    data=bytes(_canFdData.data[: _canFdData.len]),
                )
                rawData.data = canFdData
                l.append(rawData)
                continue

            if ZXDocComm.ZXDoc_RawDataType_LIN == _rawData.type:
                _linData = ZXDocComm.PZXDoc_LINData(_rawData.data).contents
                linData = ZLINData(
                    ID=_linData.ID,
                    timestamp=_linData.timestamp,
                    chksum=_linData.chksum,
                    direction=ZTransmitDirection(_linData.direction),
                    data=bytes(_linData.data[: _linData.dataLen]),
                )
                rawData.data = linData
                l.append(rawData)
                continue

            if ZXDocComm.ZXDoc_RawDataType_Ethernet == _rawData.type:
                _ethData = ZXDocComm.PZXDoc_EthernetData(_rawData.data).contents
                ethData = None

                if ZXDocComm.ZXDoc_EthernetDataType_ETHERNET == _ethData.type:
                    ethData = ZEthernetData(
                        dstMac=bytes(_ethData.header.ethernet.dstMac),
                        srcMac=bytes(_ethData.header.ethernet.srcMac),
                        etherType=ZEthernetType(_ethData.header.ethernet.etherType),
                    )
                elif ZXDocComm.ZXDoc_EthernetDataType_ETHERNET_DOT_3 == _ethData.type:
                    ethData = ZEthernetDot3Data(
                        dstMac=bytes(_ethData.header.ethernetDot3.dstMac),
                        srcMac=bytes(_ethData.header.ethernetDot3.srcMac),
                        length=_ethData.header.ethernet.length,
                    )
                elif ZXDocComm.ZXDoc_EthernetDataType_LOOPBACK == _ethData.type:
                    ethData = ZEthernetLoopbackData(
                        family=ZEthernetFamily(_ethData.header.loopback.family)
                    )
                else:
                    continue

                b = ctypes.cast(_ethData.data, ctypes.POINTER(ctypes.c_ubyte))
                ethData.data = bytes(b[: _ethData.length])
                rawData.data = ethData
                l.append(rawData)
                continue

            if ZXDocComm.ZXDoc_RawDataType_GPS == _rawData.type:
                _gpsData = ZXDocComm.PZXDoc_GPSData(_rawData.data).contents
                gpsData = ZGPSData(
                    time=(
                        ZGPSData.Time(
                            year=_gpsData.year,
                            mon=_gpsData.mon,
                            day=_gpsData.day,
                            hour=_gpsData.hour,
                            min=_gpsData.min,
                            sec=_gpsData.sec,
                            milsec=_gpsData.milsec,
                        )
                        if _gpsData.timeValid()
                        else None
                    ),
                    latitude=_gpsData.latitude if _gpsData.latlongValid() else None,
                    longitude=_gpsData.longitude if _gpsData.latlongValid() else None,
                    altitude=_gpsData.altitude if _gpsData.altitudeValid() else None,
                    speed=_gpsData.speed if _gpsData.speedValid() else None,
                    courseAngle=(
                        _gpsData.courseAngle if _gpsData.courseAngleValid() else None
                    ),
                )
                rawData.data = gpsData
                l.append(rawData)
                continue

            if ZXDocComm.ZXDoc_RawDataType_CANFDError == _rawData.type:
                _canfdErrData = ZXDocComm.PZXDoc_CANFDErrorData(_rawData.data).contents
                canfdErrData = ZCANFDErrorData(
                    errType=_canfdErrData.errType,
                    errSubType=_canfdErrData.errSubType,
                    nodeState=_canfdErrData.nodeState,
                    rxErrCount=_canfdErrData.rxErrCount,
                    txErrCount=_canfdErrData.txErrCount,
                    errData=_canfdErrData.errData,
                )
                rawData.data = canfdErrData
                l.append(rawData)
                continue

            if ZXDocComm.ZXDoc_RawDataType_LINError == _rawData.type:
                _linErrData = ZXDocComm.PZXDoc_LINErrorData(_rawData.data).contents
                linErrData = ZLINErrorData(
                    ID=_linErrData.ID,
                    dataLen=_linErrData.dataLen,
                    direction=_linErrData.direction,
                    errorState=_linErrData.errorState,
                    errorReason=_linErrData.errorReason,
                    chksum=_linErrData.chksum,
                    data=bytes(_linErrData.data),
                )
                rawData.data = gpsData
                l.append(rawData)
                continue

            if ZXDocComm.ZXDoc_RawDataType_LINEvent == _rawData.type:
                _linEvtData = ZXDocComm.PZXDoc_LINEventData(_rawData.data).contents
                linEvtData = ZLINEventData(
                    type=ZLINEventType(_linEvtData.type),
                )
                rawData.data = linEvtData
                l.append(rawData)
                continue

            if ZXDocComm.ZXDoc_RawDataType_BusUsage == _rawData.type:
                _buData = ZXDocComm.PZXDoc_BusUsageData(_rawData.data).contents
                buData = ZBusUsageData(
                    duration=_linEvtData.duration,
                    busUsage=_linEvtData.busUsage,
                    frameCount=_linEvtData.frameCount,
                )
                rawData.data = buData
                l.append(rawData)
                continue

        if 0 == len(l):
            return
        self.__on_data_sink_callback(l)

    def on_data_sink(self, callback, filter: ZDataSinkFilter = None) -> ZErrorCode:
        """
        设置数据收发回调

        callback 参数的定义如下:

            def on_data_sink(rawDatas: List[ZRawData]):
                pass
        """
        self.__on_data_sink_callback = callback

        self.__on_data_sink_callback_obj = (
            ZXDocComm.ZXDoc_Chnl_SetDataSinkCallbackType()
            if callback is None
            else ZXDocComm.ZXDoc_Chnl_SetDataSinkCallbackType(
                self.__on_data_sink_handler
            )
        )

        filterMode = ZXDocComm.ZXDoc_FilterMode_NoFilter
        filters = None
        filterCnt = 0

        if filter:
            filterMode = (
                ZXDocComm.ZXDoc_FilterMode_NoFilter
                if 0 == len(filter.rules)
                else filter.filterMode.value
            )

            if ZXDocComm.ZXDoc_FilterMode_NoFilter != filterMode:
                filterCnt = len(filter.rules)
                filters = (ZXDocComm.ZXDoc_DataSinkFilter * filterCnt)()

                for i in range(filterCnt):
                    rule = filter.rules[i]
                    _rule = filters[i]

                    if type(rule) == ZCANIDRangeFilter:
                        _rule.type = ZXDocComm.ZXDoc_CANIDRangeFilterType
                        o = _rule.rule.canIDRangeFilter
                        o.channelIndex = rule.channelIndex
                        o.direction = rule.direction
                        o.frameType = rule.frameType
                        o.idMin = rule.idMin
                        o.idMax = rule.idMax
                        continue

                    if type(rule) == ZLINIDRangeFilter:
                        _rule.type = ZXDocComm.ZXDoc_LINIDRangeFilterType
                        o = _rule.rule.linIDRangeFilter
                        o.channelIndex = rule.channelIndex
                        o.direction = rule.direction
                        o.idMin = rule.idMin
                        o.idMax = rule.idMax
                        continue

                    if type(rule) == ZCANErrorFilter:
                        _rule.type = ZXDocComm.ZXDoc_CANErrorFilterType
                        o = _rule.rule.canErrorFilter
                        o.channelIndex = rule.channelIndex
                        continue

                    if type(rule) == ZLINErrorFilter:
                        _rule.type = ZXDocComm.ZXDoc_LINErrorFilterType
                        o = _rule.rule.linErrorFilter
                        o.channelIndex = rule.channelIndex
                        continue

                    if type(rule) == ZLINWakeUpEventFilter:
                        _rule.type = ZXDocComm.ZXDoc_LINWakeUpEventFilterType
                        o = _rule.rule.linWakeUpEventFilter
                        o.channelIndex = rule.channelIndex
                        continue

        return ZErrorCode(
            ZXDocComm.ZXDoc_Chnl_SetDataSinkCallback(
                self.__handle,
                self.__on_data_sink_callback_obj,
                ctypes.c_void_p(0),
                filterMode,
                ctypes.cast(filters, ZXDocComm.PZXDoc_DataSinkFilter),
                filterCnt,
            )
        )

    # Calibration

    def cali_connect(self) -> ZErrorCode:
        """
        连接标定设备
        """
        return ZErrorCode(ZXDocComm.ZXDoc_Cali_Connect(self.__handle))

    def cali_disconnect(self) -> ZErrorCode:
        """
        断开标定设备的连接
        """
        return ZErrorCode(ZXDocComm.ZXDoc_Cali_Disonnect(self.__handle))

    def cali_start_data_acquisition(self) -> ZErrorCode:
        """
        开始获取标定数据
        """
        return ZErrorCode(ZXDocComm.ZXDoc_Cali_StartDataAcquisition(self.__handle))

    def cali_stop_data_acquisition(self) -> ZErrorCode:
        """
        停止获取标定数据
        """
        return ZErrorCode(ZXDocComm.ZXDoc_Cali_StopDataAcquisition(self.__handle))

    def cali_add_signal_to_meas_list(
        self,
        deviceId: str,
        signalId: str,
        measEvent: ZMeasEvent,
        eventChannel: int,
        pollingPeriod: int,
        daqCyclic: int,
    ) -> ZErrorCode:
        """
        添加标定信号到测量列表
        """
        return ZErrorCode(
            ZXDocComm.ZXDoc_Cali_AddSignalToMeasList(
                self.__handle,
                deviceId.encode(STR_ENCODING),
                signalId.encode(STR_ENCODING),
                measEvent.value,
                eventChannel,
                pollingPeriod,
                daqCyclic,
            )
        )

    def cali_remove_signal_from_meas_list(
        self, deviceId: str, signalId: str
    ) -> ZErrorCode:
        """
        从测量列表移除标定信号
        """
        return ZErrorCode(
            ZXDocComm.ZXDoc_Cali_RemoveSignalFromMeasList(
                self.__handle,
                deviceId.encode(STR_ENCODING),
                signalId.encode(STR_ENCODING),
            )
        )

    def cali_is_need_cache_sync_all(self) -> bool:
        result = ZXDocComm.ZXDoc_Bool(0)
        ZXDocComm.ZXDoc_Cali_RemoveSignalFromMeasList(
            self.__handle, ctypes.pointer(result)
        )
        return 0 != result.value

    def cali_can_sync_download_all(self) -> bool:
        result = ZXDocComm.ZXDoc_Bool(0)
        ZXDocComm.ZXDoc_Cali_CanSyncDownloadAll(self.__handle, ctypes.pointer(result))
        return 0 != result.value

    def cali_sync_upload_all(self, asInit: bool) -> ZErrorCode:
        return ZErrorCode(
            ZXDocComm.ZXDoc_Cali_SyncUploadAll(self.__handle, 1 if asInit else 0)
        )

    def cali_sync_download_all(self) -> ZErrorCode:
        return ZErrorCode(ZXDocComm.ZXDoc_Cali_SyncDownloadAll(self.__handle))

    def cali_select_memory_page_all(self, type: ZEcuMemPageType) -> ZErrorCode:
        return ZErrorCode(
            ZXDocComm.ZXDoc_Cali_SelectMemoryPageAll(self.__handle, type.value)
        )

    def cali_can_select_memory_page(self) -> bool:
        result = ZXDocComm.ZXDoc_Bool(0)
        ZXDocComm.ZXDoc_Cali_CanSelectMemoryPage(self.__handle, ctypes.pointer(result))
        return 0 != result.value

    def cali_can_select_memory_page(self) -> ZEcuMemPageType:
        type = ZXDocComm.ZXDoc_EcuMemPageType(0)
        ZXDocComm.ZXDoc_Cali_GetCurrentMemoryPage(self.__handle, ctypes.pointer(type))
        return ZEcuMemPageType(type)

    # E2E

    def get_e2e_crc_calculator(
        self,
        crcType: ZE2ECrcType,
        parameters: Optional[ZE2ECRCCalculatorParameters] = None,
    ) -> ZCRCCalculator:
        """
        获取一个CRC计算器
        """
        crcHanele = ZXDocComm.ZXDoc_U64(0)
        params = (
            None
            if parameters is None
            else ZXDocComm.ZXDoc_E2ECRCCalculatorParameters(
                width=parameters.width,
                polynomial=parameters.polynomial,
                initialValue=parameters.initialValue,
                xorValue=parameters.xorValue,
                reflectInput=parameters.reflectInput,
                reflectOutput=parameters.reflectOutput,
            )
        )
        if ZErrorCode.OK != ZXDocComm.ZXDoc_E2E_GetCrcCalculator(
            self.__handle,
            ctypes.byref(crcHanele),
            crcType.value,
            params,
        ):
            return None

        return ZCRCCalculator(self.__handle, crcHanele.value)

    # CAN simulation

    def start_can_simulation(self) -> ZErrorCode:
        """
        启动CAN仿真
        """
        return ZErrorCode(ZXDocComm.ZXDoc_Simu_StartCanSimulation(self.__handle))

    def stop_can_simulation(self) -> ZErrorCode:
        """
        停止CAN仿真
        """
        return ZErrorCode(ZXDocComm.ZXDoc_Simu_StopCanSimulation(self.__handle))

    def simu_active_can_send(
        self,
        channelIndex: int,
        databaseId: str,
        node: str,
        message: str,
        isActive: bool,
    ) -> ZErrorCode:
        """
        激活CAN仿真发送
        """
        return ZErrorCode(
            ZXDocComm.ZXDoc_Simu_ActiveCanSend(
                self.__handle,
                channelIndex,
                databaseId.encode(STR_ENCODING),
                node.encode(STR_ENCODING),
                message.encode(STR_ENCODING),
                ZXDocComm.ZXDoc_True if isActive else ZXDocComm.ZXDoc_False,
            )
        )

    def simu_set_can_send_type(
        self,
        channelIndex: int,
        databaseId: str,
        node: str,
        message: str,
        type: ZSimuSendType,
    ) -> ZErrorCode:
        """
        设置CAN仿真发送方式
        """
        return ZErrorCode(
            ZXDocComm.ZXDoc_Simu_SetCanSendType(
                self.__handle,
                channelIndex,
                databaseId.encode(STR_ENCODING),
                node.encode(STR_ENCODING),
                message.encode(STR_ENCODING),
                type,
            )
        )

    def simu_set_can_cycle_time(
        self,
        channelIndex: int,
        databaseId: str,
        node: str,
        message: str,
        cycleTime: int,
    ) -> ZErrorCode:
        """
        设置CAN发送周期
        """
        return ZErrorCode(
            ZXDocComm.ZXDoc_Simu_SetCanCycleTime(
                self.__handle,
                channelIndex,
                databaseId.encode(STR_ENCODING),
                node.encode(STR_ENCODING),
                message.encode(STR_ENCODING),
                cycleTime,
            )
        )

    def simu_set_can_send_repetitions(
        self,
        channelIndex: int,
        databaseId: str,
        node: str,
        message: str,
        repetitions: int,
    ) -> ZErrorCode:
        """
        设置CAN发送重复次数
        """
        return ZErrorCode(
            ZXDocComm.ZXDoc_Simu_SetCanSendRepetitions(
                self.__handle,
                channelIndex,
                databaseId.encode(STR_ENCODING),
                node.encode(STR_ENCODING),
                message.encode(STR_ENCODING),
                repetitions,
            )
        )

    def simu_set_can_signal_value(
        self,
        channelIndex: int,
        databaseId: str,
        node: str,
        message: str,
        signal: str,
        value: float,
    ) -> ZErrorCode:
        """
        设置CAN仿真信号值
        """
        return ZErrorCode(
            ZXDocComm.ZXDoc_Simu_SetCanSignalValue(
                self.__handle,
                channelIndex,
                databaseId.encode(STR_ENCODING),
                node.encode(STR_ENCODING),
                message.encode(STR_ENCODING),
                signal.encode(STR_ENCODING),
                float(value),
            )
        )

    def simu_set_can_signal_values(
        self,
        channelIndex: int,
        databaseId: str,
        node: str,
        message: str,
        signals: Dict[str, float],
    ) -> ZErrorCode:
        """
        同时设置指定CAN消息的多个信号值
        """
        itemCnt = len(signals)
        items = list(signals.items())
        signalArr = (ZXDocComm.ZXDoc_CharP * itemCnt)()
        valueArr = (ZXDocComm.ZXDoc_Double * itemCnt)()

        for i in range(itemCnt):
            (k, v) = items[i]
            signalArr[i] = k.encode(STR_ENCODING)
            valueArr[i] = v

        return ZErrorCode(
            ZXDocComm.ZXDoc_Simu_SetCanSignalValues(
                self.__handle,
                channelIndex,
                databaseId.encode(STR_ENCODING),
                node.encode(STR_ENCODING),
                message.encode(STR_ENCODING),
                signalArr,
                valueArr,
                itemCnt,
            )
        )

    def simu_set_canfd_type(
        self,
        channelIndex: int,
        databaseId: str,
        node: str,
        message: str,
        isCANFD: bool,
    ) -> ZErrorCode:
        """
        设置是否使用CANFD
        """
        return ZErrorCode(
            ZXDocComm.ZXDoc_Simu_SetCanFdType(
                self.__handle,
                channelIndex,
                databaseId.encode(STR_ENCODING),
                node.encode(STR_ENCODING),
                message.encode(STR_ENCODING),
                isCANFD,
            )
        )

    def simu_set_canfd_brs(
        self,
        channelIndex: int,
        databaseId: str,
        node: str,
        message: str,
        isBRS: bool,
    ) -> ZErrorCode:
        """
        设置是否使用CANFD
        """
        return ZErrorCode(
            ZXDocComm.ZXDoc_Simu_SetCanFdBrs(
                self.__handle,
                channelIndex,
                databaseId.encode(STR_ENCODING),
                node.encode(STR_ENCODING),
                message.encode(STR_ENCODING),
                isBRS,
            )
        )

    def simu_set_can_high_precision_cheduled(self, enabled: bool) -> ZErrorCode:
        """ """
        return ZErrorCode(
            ZXDocComm.ZXDoc_Simu_SetCanHighPrecisionScheduled(
                self.__handle,
                ZXDocComm.ZXDoc_True if enabled else ZXDocComm.ZXDoc_False,
            )
        )

    def simu_is_can_high_precision_cheduled(self) -> bool:
        """ """
        re = ZXDocComm.ZXDoc_Bool(0)
        if ZErrorCode.OK != ZXDocComm.ZXDoc_Simu_IsCanHighPrecisionScheduled(
            self.__handle, ctypes.byref(re)
        ):
            return False
        return 0 != re.value

    # Device

    def get_devices(self, maxCount: int = 16) -> List[ZDeviceInfo]:
        """
        获取设备列表
        """
        deviceInfos = (ZXDocComm.ZXDoc_DeviceInfo * maxCount)()
        count = ZXDocComm.ZXDoc_U32(maxCount)
        ZXDocComm.ZXDoc_Dev_GetDevices(
            self.__handle, 0xFF, deviceInfos, ctypes.byref(count)
        )

        devices = []
        for i in range(count.value):
            d = deviceInfos[i]
            devices.append(
                ZDeviceInfo(
                    id=d.id.decode(STR_ENCODING),
                    type=ZDeviceType(d.type),
                    name=d.name.decode(STR_ENCODING),
                    busType=ZBusType(d.busType),
                    logicalChannel=d.logicalChannel,
                    enabled=(ZXDocComm.ZXDoc_True == d.enabled),
                    databaseId=d.databaseId.decode(STR_ENCODING),
                    databaseName=d.databaseName.decode(STR_ENCODING),
                )
            )

        return devices

    def create_uds_interface(
        self, cfg: Union[ZDoCANCfg, ZDoLINCfg, ZDoIPCfg]
    ) -> ZUdsInterface:
        if isinstance(cfg, ZDoCANCfg):
            _udsHandle = ZXDocComm.ZXDoc_UdsInterfaceHandle(0)
            _cfg = ZXDocComm.ZXDoc_DoCANCfg()
            _cfg.udsPort = cfg.udsPort
            _cfg.channelIndex = cfg.channelIndex
            _cfg.frameType = cfg.frameType
            _cfg.protocolVersion = cfg.protocolVersion
            _cfg.fillByte = cfg.fillByte
            _cfg.isfillByte = cfg.isfillByte
            _cfg.p2Timeout = cfg.p2Timeout
            _cfg.p2xTimeout = cfg.p2xTimeout
            _cfg.isModifyEcuSTmin = cfg.isModifyEcuSTmin
            _cfg.remoteSTmin = cfg.remoteSTmin
            _cfg.localSTmin = cfg.localSTmin
            _cfg.blockSize = cfg.blockSize
            _cfg.fcTimeout = cfg.fcTimeout
            if ZXDocComm.ZXDOC_E_OK != ZXDocComm.ZXDoc_Diag_CreateDoCANInterface(
                self.__handle, ctypes.byref(_udsHandle), ctypes.byref(_cfg)
            ):
                return None
            return ZUdsInterface(self.__handle, _udsHandle.value)

        if isinstance(cfg, ZDoLINCfg):
            _udsHandle = ZXDocComm.ZXDoc_UdsInterfaceHandle(0)
            _cfg = ZXDocComm.ZXDoc_DoLINCfg()
            _cfg.udsPort = cfg.udsPort
            _cfg.channelIndex = cfg.channelIndex
            _cfg.fillByte = cfg.fillByte
            _cfg.isfillByte = cfg.isfillByte
            _cfg.p2Timeout = cfg.p2Timeout
            _cfg.p2xTimeout = cfg.p2xTimeout
            _cfg.isModifyEcuSTmin = cfg.isModifyEcuSTmin
            _cfg.remoteSTmin = cfg.remoteSTmin
            if ZXDocComm.ZXDOC_E_OK != ZXDocComm.ZXDoc_Diag_CreateDoLINInterface(
                self.__handle, ctypes.byref(_udsHandle), ctypes.byref(_cfg)
            ):
                return None
            return ZUdsInterface(self.__handle, _udsHandle.value)

        if isinstance(cfg, ZDoIPCfg):
            _udsHandle = ZXDocComm.ZXDoc_UdsInterfaceHandle(0)
            _cfg = ZXDocComm.ZXDoc_DoIPCfg()
            _cfg.udsPort = cfg.udsPort
            _cfg.vehicleIp = cfg.vehicleIp.encode(STR_ENCODING)
            _cfg.localIp = cfg.localIp.encode(STR_ENCODING)
            _cfg.localPort = cfg.localPort
            _cfg.protocolVersion = cfg.protocolVersion
            _cfg.testerAddress = cfg.testerAddress
            _cfg.routingActivationType = cfg.routingActivationType
            _cfg.withOEMSpecificData = cfg.withOEMSpecificData
            l = min(len(_cfg.oemSpecificData), len(cfg.oemSpecificData))
            for i in range(l):
                _cfg.oemSpecificData[i] = cfg.oemSpecificData[i]
            _cfg.aliveCheckCycle = cfg.aliveCheckCycle
            _cfg.isResponseAliveCheck = cfg.isResponseAliveCheck
            _cfg.p2Timeout = cfg.p2Timeout
            _cfg.p2xTimeout = cfg.p2xTimeout
            _cfg.waitForACK = cfg.waitForACK
            _cfg.ackTimeoutMs = cfg.ackTimeoutMs
            _cfg.connectTimeout = cfg.connectTimeout

            if ZXDocComm.ZXDOC_E_OK != ZXDocComm.ZXDoc_Diag_CreateDoIPInterface(
                self.__handle, ctypes.byref(_udsHandle), ctypes.byref(_cfg)
            ):
                return None
            return ZUdsInterface(self.__handle, _udsHandle.value)

        return None

    __DBS = {
        ZDBType.DBC: ZDbcDatabase,
        ZDBType.ODX: ZOdxDatabase,
        ZDBType.A2L: ZA2lDatabase,
        ZDBType.LDF: ZLdfDatabase,
        ZDBType.SysVar: ZSysVarDatabase,
        ZDBType.FuncVar: ZFuncVarDatabase,
        ZDBType.UserVar: ZUserVarDatabase,
        ZDBType.AcqPlugin: ZPluginDatabase,
    }

    def __wrap_database(self, db: ZXDocComm.ZXDoc_Database) -> ZDatabase:
        if db is None:
            return None

        dbType = ZDBType.UNKNOWN
        try:
            dbType = ZDBType(db.type)
        except:
            dbType = db.type
        dbId = db.id.decode(STR_ENCODING)
        dbName = db.name.decode(STR_ENCODING)
        dbFilePath = db.filePath.decode(STR_ENCODING)

        DB = self.__DBS.get(dbType)
        if DB is None:
            DB = ZDatabase

        return DB(dbType, dbId, dbName, dbFilePath, self.__handle)

    def add_database(self, filePath: str) -> ZDatabase:
        pDb = ZXDocComm.ZXDoc_Database()
        if ZXDocComm.ZXDOC_E_OK != ZXDocComm.ZXDoc_DB_AddDatabase(
            self.__handle, filePath.encode(STR_ENCODING), ctypes.byref(pDb)
        ):
            return None

        return self.__wrap_database(pDb)

    def remove_database(self, dbId: str) -> bool:
        return ZXDocComm.ZXDOC_E_OK == ZXDocComm.ZXDoc_DB_RemoveDatabase(
            self.__handle, dbId.encode(STR_ENCODING)
        )

    def get_databases(self) -> List[ZDatabase]:
        count = ZXDocComm.ZXDoc_U32(0)

        if ZXDocComm.ZXDOC_E_OK != ZXDocComm.ZXDoc_DB_GetDatabases(
            self.__handle, None, ctypes.byref(count)
        ):
            return None

        if 0 == count:
            return []

        dbs = (ZXDocComm.ZXDoc_Database * count.value)()
        if ZXDocComm.ZXDOC_E_OK != ZXDocComm.ZXDoc_DB_GetDatabases(
            self.__handle, dbs, ctypes.byref(count)
        ):
            return None

        _dbs = []
        for i in range(count.value):
            _dbs.append(self.__wrap_database(dbs[i]))

        return _dbs

    def get_database_by_name(self, name: str) -> ZDatabase:
        db = ZXDocComm.ZXDoc_Database()
        if ZXDocComm.ZXDOC_E_OK != ZXDocComm.ZXDoc_DB_GetDatabaseByName(
            self.__handle, name.encode(STR_ENCODING), ctypes.byref(db)
        ):
            return None

        return self.__wrap_database(db)

    def get_database_by_id(self, id: str) -> ZDatabase:
        db = ZXDocComm.ZXDoc_Database()
        if ZXDocComm.ZXDOC_E_OK != ZXDocComm.ZXDoc_DB_GetDatabaseById(
            self.__handle, id.encode(STR_ENCODING), ctypes.byref(db)
        ):
            return None

        return self.__wrap_database(db)

    def get_chnl_databases(
        self, busType: ZBusType, channelIndex: int, maxSize: int = 16
    ):
        ArrT = ZXDocComm.ZXDoc_Char * 64
        pDbIds = (ArrT * maxSize)()

        count = ZXDocComm.ZXDoc_U32(maxSize)
        if ZXDocComm.ZXDOC_E_OK != ZXDocComm.ZXDoc_Chnl_GetDatabases(
            self.__handle,
            busType,
            channelIndex,
            ctypes.cast(pDbIds, ZXDocComm.ZXDoc_CharPP),
            ctypes.byref(count),
        ):
            return None

        dbs = []
        for i in range(count.value):
            dbs.append(pDbIds[i].value.decode(STR_ENCODING))
        return dbs

    def set_chnl_databases(
        self, busType: ZBusType, channelIndex: int, dbIds: List[str]
    ):
        ArrT = ZXDocComm.ZXDoc_CharP * len(dbIds)
        count = ZXDocComm.ZXDoc_U32(len(dbIds))
        pDbIds = ArrT()

        for i in range(count.value):
            pDbIds[i] = dbIds[i].encode(STR_ENCODING)

        return ZXDocComm.ZXDOC_E_OK == ZXDocComm.ZXDoc_Chnl_SetDatabases(
            self.__handle,
            busType,
            channelIndex,
            ctypes.cast(pDbIds, ZXDocComm.ZXDoc_CharPP),
            count,
        )

    def add_data_recorder(
        self, cfg: Union[ZMeasureDataRecorderCfg, ZMessageRecorderCfg]
    ):
        if type(cfg) == ZMeasureDataRecorderCfg:
            _cfg = ZXDocComm.ZXDoc_MeasureDataRecorderCfg_New()

            ZXDocComm.ZXDoc_MeasureDataRecorderCfg_SetRecorderName(
                _cfg, cfg.recorderName.encode(STR_ENCODING)
            )

            ZXDocComm.ZXDoc_MeasureDataRecorderCfg_SetFilePath(
                _cfg, cfg.filePath.encode(STR_ENCODING)
            )

            ZXDocComm.ZXDoc_MeasureDataRecorderCfg_SetMaxFileSize(_cfg, cfg.maxFileSize)

            ZXDocComm.ZXDoc_MeasureDataRecorderCfg_SetComment(
                _cfg, cfg.comment.encode(STR_ENCODING)
            )

            ZXDocComm.ZXDoc_MeasureDataRecorderCfg_SetFileNameAutoAddTimeSuffix(
                _cfg, 1 if cfg.fileNameAutoAddTimeSuffix else 0
            )

            ZXDocComm.ZXDoc_MeasureDataRecorderCfg_SetMf4Compression(
                _cfg, 1 if cfg.mf4Compression else 0
            )

            for sglId in cfg.signals:
                _sglid = ZXDocComm.ZXDoc_SignalIdentifier()
                _sglid.sourceId = sglId.sourceId.encode(STR_ENCODING)
                _sglid.signalId = sglId.signalId.encode(STR_ENCODING)
                ZXDocComm.ZXDoc_MeasureDataRecorderCfg_AddSignal(
                    _cfg, ctypes.pointer(_sglid)
                )

            if ZXDocComm.ZXDOC_E_OK != ZXDocComm.ZXDoc_RC_AddMeasureDataRecorder(
                self.__handle, _cfg
            ):
                return False

            ZXDocComm.ZXDoc_MeasureDataRecorderCfg_Free(_cfg)
            return True

        if type(cfg) == ZMessageRecorderCfg:
            _cfg = ZXDocComm.ZXDoc_MessageRecorderCfg_New()

            ZXDocComm.ZXDoc_MessageRecorderCfg_SetRecorderName(
                _cfg, cfg.recorderName.encode(STR_ENCODING)
            )

            ZXDocComm.ZXDoc_MessageRecorderCfg_SetFilePath(
                _cfg, cfg.filePath.encode(STR_ENCODING)
            )

            ZXDocComm.ZXDoc_MessageRecorderCfg_SetMaxFileSize(_cfg, cfg.maxFileSize)

            ZXDocComm.ZXDoc_MessageRecorderCfg_SetComment(
                _cfg, cfg.comment.encode(STR_ENCODING)
            )

            ZXDocComm.ZXDoc_MessageRecorderCfg_SetFileNameAutoAddTimeSuffix(
                _cfg, 1 if cfg.fileNameAutoAddTimeSuffix else 0
            )

            ZXDocComm.ZXDoc_MessageRecorderCfg_SetMf4Compression(
                _cfg, 1 if cfg.mf4Compression else 0
            )

            for chl in cfg.channels:
                ZXDocComm.ZXDoc_MessageRecorderCfg_AddChannel(
                    _cfg, chl.busType.value, chl.logicalIndex
                )

            if ZXDocComm.ZXDOC_E_OK != ZXDocComm.ZXDoc_RC_AddMessageRecorder(
                self.__handle, _cfg
            ):
                return False

            ZXDocComm.ZXDoc_MessageRecorderCfg_Free(_cfg)
            return True

        return False

    def remove_data_recorder(self, recorderName: str):
        return ZErrorCode(
            ZXDocComm.ZXDoc_RC_RemoveDataRecorder(
                self.__handle, recorderName.encode(STR_ENCODING)
            )
        )

    def start_data_recorder(self, recorderName: str):
        return ZErrorCode(
            ZXDocComm.ZXDoc_RC_StartDataRecorder(
                self.__handle, recorderName.encode(STR_ENCODING)
            )
        )

    def stop_data_recorder(self, recorderName: str):
        return ZErrorCode(
            ZXDocComm.ZXDoc_RC_StopDataRecorder(
                self.__handle, recorderName.encode(STR_ENCODING)
            )
        )
