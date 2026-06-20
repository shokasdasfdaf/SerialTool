#include "serialconfig.h"
#include <QSerialPortInfo>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QPushButton>

SerialConfigDialog::SerialConfigDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("串口配置");
    setMinimumWidth(320);

    auto *form = new QFormLayout;

    m_portCombo = new QComboBox;
    auto *refreshBtn = new QPushButton("刷新");
    auto *portLayout = new QHBoxLayout;
    portLayout->addWidget(m_portCombo, 1);
    portLayout->addWidget(refreshBtn);
    form->addRow("串口:", portLayout);

    m_baudCombo = new QComboBox;
    m_baudCombo->addItems({"9600", "19200", "38400", "57600", "115200", "230400", "460800"});
    m_baudCombo->setCurrentText("115200");
    form->addRow("波特率:", m_baudCombo);

    m_dataBitsCombo = new QComboBox;
    m_dataBitsCombo->addItems({"8", "7", "6", "5"});
    m_dataBitsCombo->setCurrentText("8");
    form->addRow("数据位:", m_dataBitsCombo);

    m_stopBitsCombo = new QComboBox;
    m_stopBitsCombo->addItems({"1", "1.5", "2"});
    m_stopBitsCombo->setCurrentText("1");
    form->addRow("停止位:", m_stopBitsCombo);

    m_parityCombo = new QComboBox;
    m_parityCombo->addItems({"None", "Even", "Odd", "Mark", "Space"});
    form->addRow("校验位:", m_parityCombo);

    m_flowCombo = new QComboBox;
    m_flowCombo->addItems({"None", "RTS/CTS", "XON/XOFF"});
    form->addRow("流控:", m_flowCombo);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form->addRow(buttons);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(form);

    connect(refreshBtn, &QPushButton::clicked, this, &SerialConfigDialog::onRefreshPorts);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    onRefreshPorts();
}

void SerialConfigDialog::onRefreshPorts()
{
    m_portCombo->clear();
    const auto ports = QSerialPortInfo::availablePorts();
    for (const auto &info : ports)
        m_portCombo->addItem(info.portName() + " - " + info.description(), info.portName());
}

QString SerialConfigDialog::portName() const
{
    return m_portCombo->currentData().toString();
}

int SerialConfigDialog::baudRate() const
{
    return m_baudCombo->currentText().toInt();
}

QSerialPort::DataBits SerialConfigDialog::dataBits() const
{
    return static_cast<QSerialPort::DataBits>(m_dataBitsCombo->currentText().toInt());
}

QSerialPort::StopBits SerialConfigDialog::stopBits() const
{
    float s = m_stopBitsCombo->currentText().toFloat();
    if (s == 1.5f) return QSerialPort::OneAndHalfStop;
    if (s == 2.0f) return QSerialPort::TwoStop;
    return QSerialPort::OneStop;
}

QSerialPort::Parity SerialConfigDialog::parity() const
{
    QString p = m_parityCombo->currentText();
    if (p == "Even") return QSerialPort::EvenParity;
    if (p == "Odd") return QSerialPort::OddParity;
    if (p == "Mark") return QSerialPort::MarkParity;
    if (p == "Space") return QSerialPort::SpaceParity;
    return QSerialPort::NoParity;
}

QSerialPort::FlowControl SerialConfigDialog::flowControl() const
{
    QString f = m_flowCombo->currentText();
    if (f == "RTS/CTS") return QSerialPort::HardwareControl;
    if (f == "XON/XOFF") return QSerialPort::SoftwareControl;
    return QSerialPort::NoFlowControl;
}
