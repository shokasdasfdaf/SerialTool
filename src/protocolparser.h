#pragma once

#include <QObject>
#include <QSerialPort>
#include <QByteArray>

class ProtocolParser
{
public:
    // 帧格式: 0xAA 0x55 [DataLen(1B)] [Data(N)] [Checksum(1B)]
    static constexpr quint8 FRAME_HEADER0 = 0xAA;
    static constexpr quint8 FRAME_HEADER1 = 0x55;
    static constexpr int MIN_FRAME_LEN = 5;  // header(2) + len(1) + data(1+) + checksum(1)

    // 接收缓冲区最大长度，超过则丢弃旧数据保留尾部
    // 防止串口持续输出乱码导致内存无限增长
    static constexpr int MAX_BUFFER_SIZE = 64 * 1024;

    struct Frame {
        bool valid = false;
        QByteArray data;
    };

    // 从原始数据流中提取完整帧，返回解析出的帧列表
    // 这个函数会修改内部缓冲区（处理粘包/半包）
    static QList<Frame> parseFrames(QByteArray &buffer);

    // 构造一帧数据
    static QByteArray buildFrame(const QByteArray &data);

    // 从帧数据中提取 4 通道波形值（每个通道 int16）
    // data 格式: [Ch0_Hi Ch0_Lo Ch1_Hi Ch1_Lo Ch2_Hi Ch2_Lo Ch3_Hi Ch3_Lo]
    static QList<qint16> extractChannelValues(const QByteArray &data);

private:
    static quint8 calcChecksum(const QByteArray &data);
};
