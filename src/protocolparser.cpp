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

    while (buffer.size() >= MIN_FRAME_LEN) {
        // 找帧头
        int headerPos = -1;
        for (int i = 0; i <= buffer.size() - 2; ++i) {
            if (static_cast<quint8>(buffer[i]) == FRAME_HEADER0
                && static_cast<quint8>(buffer[i + 1]) == FRAME_HEADER1) {
                headerPos = i;
                break;
            }
        }

        if (headerPos == -1)
            break;  // 没找到帧头

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
        buffer.remove(0, totalLen);

        // 校验
        QByteArray toCheck = frameData.mid(2, 1 + dataLen);  // len byte + data
        quint8 expected = calcChecksum(toCheck);
        quint8 actual = static_cast<quint8>(frameData[totalLen - 1]);

        Frame f;
        if (expected == actual) {
            f.valid = true;
            f.data = frameData.mid(3, dataLen);  // 提取数据部分
        }
        frames.append(f);
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
