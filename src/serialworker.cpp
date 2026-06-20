#include "serialworker.h"
#include <QSerialPortInfo>
#include <QDebug>

SerialWorker::SerialWorker(QObject *parent)
    : QObject(parent)
{
    m_serial = new QSerialPort(this);
    connect(m_serial, &QSerialPort::readyRead,
            this, &SerialWorker::onReadyRead);
    connect(m_serial, &QSerialPort::errorOccurred,
            this, &SerialWorker::onErrorOccurred);
}

SerialWorker::~SerialWorker()
{
    if (m_serial->isOpen())
        m_serial->close();
}

void SerialWorker::openPort(const QString &portName, int baudRate,
                             QSerialPort::DataBits dataBits, QSerialPort::StopBits stopBits,
                             QSerialPort::Parity parity, QSerialPort::FlowControl flowControl)
{
    m_serial->setPortName(portName);
    m_serial->setBaudRate(baudRate);
    m_serial->setDataBits(dataBits);
    m_serial->setStopBits(stopBits);
    m_serial->setParity(parity);
    m_serial->setFlowControl(flowControl);

    if (!m_serial->open(QIODevice::ReadWrite)) {
        emit portOpened(false, m_serial->errorString());
        return;
    }

    qDebug() << "Serial port opened:" << portName << baudRate;
    emit portOpened(true, "串口已打开");
}

void SerialWorker::closePort()
{
    if (m_serial->isOpen()) {
        m_serial->close();
        qDebug() << "Serial port closed";
        emit portClosed();
    }
}

void SerialWorker::writeData(const QByteArray &data)
{
    if (m_serial->isOpen())
        m_serial->write(data);
}

void SerialWorker::onReadyRead()
{
    QByteArray data = m_serial->readAll();
    if (!data.isEmpty())
        emit dataReceived(data);
}

void SerialWorker::onErrorOccurred(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError)
        return;
    qWarning() << "Serial error:" << error << m_serial->errorString();
    emit errorOccurred(m_serial->errorString());
}
