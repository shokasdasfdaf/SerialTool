#include "protocolparser.h"

quint8 ProtocolParser::calcChecksum(const QByteArray &data)
{
    quint8 sum = 0;
    for (int i = 0; i < data.size(); ++i)
        sum ^= static_cast<quint8>(data[i]);
    return sum;
}

QList<ProtocolParser::Frame> ProtocolParser::parseFrames(QByteArray &buffer)
{
    QList<Frame> frames;

    // 缓冲区超限保护：丢弃前半段保留尾部继续找帧头
    // 防止串口持续输出乱码导致内存无限增长
    if (buffer.size() > MAX_BUFFER_SIZE)
        buffer.remove(0, buffer.size() - MAX_BUFFER_SIZE / 2);

    static const QByteArray FRAME_HEADER = QByteArrayLiteral("\xAA\x55");

    while (buffer.size() >= MIN_FRAME_LEN) {
        // 用 indexOf 查找帧头，比逐字节循环高效（Boyer-Moore 算法）
        // 校验失败时也用同一逻辑跳到下一个候选帧头，加速异常恢复
        int headerPos = buffer.indexOf(FRAME_HEADER);

        if (headerPos == -1) {
            // 没找到帧头，但末尾的 0xAA 可能是下一帧的起始字节，留 1 字节
            if (buffer.size() > 1)
                buffer.remove(0, buffer.size() - 1);
            break;
        }

        // 丢弃帧头之前的无效数据
        if (headerPos > 0)
            buffer.remove(0, headerPos);

        if (buffer.size() < MIN_FRAME_LEN)
            break;

        quint8 dataLen = static_cast<quint8>(buffer[2]);
        int totalLen = 2 + 1 + dataLen + 1;  // header + len + data + checksum

        if (buffer.size() < totalLen)
            break;  // 帧未完整到达

        QByteArray frameData = buffer.left(totalLen);

        // 校验
        QByteArray toCheck = frameData.mid(2, 1 + dataLen);  // len byte + data
        quint8 expected = calcChecksum(toCheck);
        quint8 actual = static_cast<quint8>(frameData[totalLen - 1]);

        if (expected == actual) {
            Frame f;
            f.valid = true;
            f.data = frameData.mid(3, dataLen);  // 提取数据部分
            frames.append(f);
            buffer.remove(0, totalLen);
        } else {
            // 校验失败：只跳过当前的 AA 55（2 字节），让下一轮 indexOf
            // 在剩余数据里找下一个候选帧头，而不是丢掉整个 totalLen
            // 避免误把恰好 dataLen 字段大值的位置当成"帧"丢弃真实帧
            buffer.remove(0, 2);
        }
    }

    return frames;
}

QByteArray ProtocolParser::buildFrame(const QByteArray &data)
{
    if (data.size() > 255)
        return {};

    QByteArray frame;
    frame.append(static_cast<char>(FRAME_HEADER0));
    frame.append(static_cast<char>(FRAME_HEADER1));

    quint8 len = static_cast<quint8>(data.size());
    frame.append(static_cast<char>(len));
    frame.append(data);

    QByteArray toCheck;
    toCheck.append(static_cast<char>(len));
    toCheck.append(data);
    frame.append(static_cast<char>(calcChecksum(toCheck)));

    return frame;
}

QList<qint16> ProtocolParser::extractChannelValues(const QByteArray &data)
{
    QList<qint16> values;
    if (data.size() < 8)
        return values;

    for (int i = 0; i < 4; ++i) {
        qint16 val = (static_cast<quint8>(data[i * 2]) << 8)
                   | static_cast<quint8>(data[i * 2 + 1]);
        values.append(val);
    }
    return values;
}
