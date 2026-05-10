from ZXDoc import *
import time


# 测量状态变更回调
def on_meas_stat_changed(stat):
    print(
        f'measurement { "started" if MEASUREMENT_STATUS_STARTED == stat else "stoped"}'
    )


# 数据接收回调
def on_data(dataSet):
    for rawData in dataSet:
        print(rawData.data)


# 执行一个UDS请求
def doUdsRequest(uds: ZUdsInterface, phyAddr: int, funcAddr: int, testerAddr: int):
    rsp = uds.request(
        ZUdsRequest(
            src_addr=phyAddr,
            dst_addr=testerAddr,
            is_extend=False,
            suppress_response=False,
            sid=0x10,
            data=[0x03],
        ),
    )
    if rsp:
        if UDS_RSP_STATUS_OK != rsp.status or UDS_RSP_TYPE_UNKNOWN == rsp.responseType:
            app.log_e(uds.get_error_message(rsp))
    else:
        app.log_e("uds_request failed")

# 执行一个UDS请求
def doUdsRequest1(uds: ZUdsInterface, phyAddr: int, funcAddr: int, testerAddr: int):
    rsp = uds.request(
        ZUdsRequest(
            src_addr=phyAddr,
            dst_addr=testerAddr,
            is_extend=False,
            suppress_response=False,
            sid=0x10,
            data=[0x02],
        ),
    )
    if rsp:
        if UDS_RSP_STATUS_OK != rsp.status or UDS_RSP_TYPE_UNKNOWN == rsp.responseType:
            app.log_e(uds.get_error_message(rsp))
    else:
        app.log_e("uds_request failed")
# 安全访问
def doSecurityAccess(uds: ZUdsInterface, phyAddr: int, funcAddr: int, testerAddr: int):
    saReq = ZSecurityAccessReq(
        keyDllPath=r"E:\visualStudioCode\ZcanProDll\Debug\ZcanProDll.dll",
        srcAddr=phyAddr,
        dstAddr=testerAddr,
        securityLevel=1,
        isExtend=False,
    )
    if not uds.security_access(saReq):
        app.log_e("Security access failed")
    else:
        app.log_i("Security access success")


# 文件下载
def doFileDownload(uds: ZUdsInterface, phyAddr: int, funcAddr: int, testerAddr: int):
    dlReq = ZFileDownloadReq(
        filePath=r"D:\flash_file.bin",  # 刷写文件路径
        memEraseType=ZMemEraseType.WithParam,  # 内存擦除方式
        srcAddr=phyAddr,  # 请求地址
        dstAddr=testerAddr,  # 测试仪地址/响应地址
        crcAlgorithm=ZCrcAlgorithm(
            type=ZCrcType.CRC32,
            polynomial=0x4C11DB7,  # 多项式
            initValue=0xFFFFFFFF,  # 初始值
            xorOutput=0xFFFFFFFF,  # 输出异或值
            reflectInput=True,  # 输入反转
            reflectOutput=True,  # 输出反转
        ),
        totalCheckCmd=bytes.fromhex("31 01 02 02"),  # 总校验指令
    )
    if not uds.file_download(dlReq):
        app.log_e("File download failed")
    else:
        app.log_i("File download success")


# CAN诊断示例
def DoCAN_demo():
    uds = ZDoCanInterface(
        channelIndex=1,  # 通道号
        cfg=ZUdsCANCfg(
            frameType=CAN_FRAME_TYPE_CAN,
            protocolVersion=CAN_TP_ISO15765_2_2016,
            fillByte=0xAA,
            isFillByte=True,
            p2Timeout=2000,
            p2xTimeout=5000,
            isReplaceEcuSTmin=False,
            remoteSTmin=0,
            localSTmin=0,
            blockSize=0,
            fcTimeout=1000,
        ),
    )

    if uds.handle() < 0:
        app.log_e("failed to create uds interface")
        return

    # 开启会话保持
    uds.start_session_keep(0x7DF, 3000, True)

    doUdsRequest(uds, 0x74C , 0x7DF, 0x75C )
    doSecurityAccess(uds, 0x74C , 0x7DF, 0x75C )
    # doFileDownload(uds, 0x74C , 0x7DF, 0x75C )
    doUdsRequest1(uds, 0x74C , 0x7DF, 0x75C )
    # 停止会话保持
    uds.stop_session_keep()


# DoIP诊断示例
def DoIP_demo():
    uds = ZDoIpInterface(
        channelIndex=1,  # 通道号
        cfg=ZUdsDoIPCfg(
            vehicleIp="127.0.0.1",
            localIp="127.0.0.1",
            localPort=0,
            srcLogicalAddr=0xE81,
            protocolVersion=DOIP_TP_ISO_13400_2_2012,
            routingActivationType=ZDOIP_ACTIVATION_DEFAULT,
            oemSpecificData=[0x01, 0x02, 0x03, 0x04],
            connectTimeout=2000,
            p2Timeout=1000,
            p2xTimeout=2000,
            waitForACK=True,
            ackTimeout=500,
        ),
    )

    if uds.handle() < 0:
        app.log_e("failed to create uds interface")
        return

    # 开启会话保持
    uds.start_session_keep(0xE400, 3000, True)

    doUdsRequest(uds, 0x0002, 0xE400, 0xE80)
    doSecurityAccess(uds, 0x0002, 0xE400, 0xE80)
    doFileDownload(uds, 0x0002, 0xE400, 0xE80)

    # 停止会话保持
    uds.stop_session_keep()


def __zxdoc_main__():
    # 设置测量状态回调
    measurement.on_status_changed(on_meas_stat_changed)

    # 启动测量
    if not measurement.is_started():
        measurement.start()

    # 设置数据回调函数
    # channel.add_data_sink(on_data)

    DoCAN_demo()
    DoIP_demo()

    time.sleep(1)


def __zxdoc_on_exit__():
    # 停止测量
    measurement.stop()
