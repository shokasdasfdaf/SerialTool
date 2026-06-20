#pragma once

#include <QDialog>
#include <QComboBox>
#include <QSerialPort>

class SerialConfigDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SerialConfigDialog(QWidget *parent = nullptr);

    QString portName() const;
    int baudRate() const;
    QSerialPort::DataBits dataBits() const;
    QSerialPort::StopBits stopBits() const;
    QSerialPort::Parity parity() const;
    QSerialPort::FlowControl flowControl() const;

private slots:
    void onRefreshPorts();

private:
    QComboBox *m_portCombo;
    QComboBox *m_baudCombo;
    QComboBox *m_dataBitsCombo;
    QComboBox *m_stopBitsCombo;
    QComboBox *m_parityCombo;
    QComboBox *m_flowCombo;
};
