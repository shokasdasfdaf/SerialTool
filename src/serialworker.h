#pragma once

#include <QObject>
#include <QSerialPort>
#include <QTimer>

class SerialWorker : public QObject
{
    Q_OBJECT
public:
    explicit SerialWorker(QObject *parent = nullptr);
    ~SerialWorker();

public slots:
    void openPort(const QString &portName, int baudRate,
                  QSerialPort::DataBits dataBits, QSerialPort::StopBits stopBits,
                  QSerialPort::Parity parity, QSerialPort::FlowControl flowControl);
    void closePort();
    void writeData(const QByteArray &data);

signals:
    void portOpened(bool success, const QString &msg);
    void portClosed();
    void dataReceived(const QByteArray &data);
    void errorOccurred(const QString &error);

private slots:
    void onReadyRead();
    void onErrorOccurred(QSerialPort::SerialPortError error);

private:
    QSerialPort *m_serial = nullptr;
};
