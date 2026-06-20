#include "mainwindow.h"
#include "serialconfig.h"
#include "../serialworker.h"
#include "../protocolparser.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QFileInfo>
#include <QMessageBox>
#include <QScrollBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_channelColors = {Qt::red, Qt::blue, Qt::green, QColor(200, 120, 0)};

    m_serialThread = new QThread(this);
    m_serialWorker = new SerialWorker();
    m_serialWorker->moveToThread(m_serialThread);
    // 线程结束时自动删除 worker，避免在 UI 线程跨线程 delete 含 QSerialPort 的对象
    connect(m_serialThread, &QThread::finished,
            m_serialWorker, &QObject::deleteLater);
    m_serialThread->start();

    setupUI();
    setupConnections();
}

MainWindow::~MainWindow()
{
    // 先在 worker 线程里关串口（BlockingQueuedConnection 同步等待返回），再退出线程
    if (m_serialWorker) {
        QMetaObject::invokeMethod(m_serialWorker, "closePort",
                                  Qt::BlockingQueuedConnection);
    }
    m_serialThread->quit();
    m_serialThread->wait();
    // m_serialWorker 由 finished 信号触发 deleteLater 自动释放，不要手动 delete
}

void MainWindow::setupUI()
{
    setWindowTitle("串口调试与数据监控工具");
    resize(1000, 700);

    auto *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    auto *mainLayout = new QVBoxLayout(centralWidget);

    // ======== 工具栏 ========
    auto *toolBar = new QHBoxLayout;

    m_openBtn = new QPushButton("打开串口");
    m_openBtn->setFixedWidth(90);
    toolBar->addWidget(m_openBtn);

    toolBar->addSpacing(10);

    m_rxCountLabel = new QLabel("RX: 0 B");
    m_txCountLabel = new QLabel("TX: 0 B");
    toolBar->addWidget(m_rxCountLabel);
    toolBar->addWidget(m_txCountLabel);

    toolBar->addStretch();

    m_exportBtn = new QPushButton("导出 CSV");
    m_clearBtn = new QPushButton("清空终端");
    toolBar->addWidget(m_exportBtn);
    toolBar->addWidget(m_clearBtn);

    mainLayout->addLayout(toolBar);

    // ======== 主体：终端 + 波形 ========
    auto *splitter = new QSplitter(Qt::Vertical);

    // --- 终端区 ---
    auto *terminalGroup = new QGroupBox("数据终端");
    auto *terminalLayout = new QVBoxLayout(terminalGroup);

    auto *terminalBar = new QHBoxLayout;
    m_displayMode = new QComboBox;
    m_displayMode->addItems({"HEX 显示", "ASCII 显示"});
    terminalBar->addWidget(new QLabel("显示模式:"));
    terminalBar->addWidget(m_displayMode);
    m_protocolMode = new QCheckBox("协议解析 (AA 55)");
    terminalBar->addWidget(m_protocolMode);
    terminalBar->addStretch();
    terminalLayout->addLayout(terminalBar);

    m_terminal = new QTextEdit;
    m_terminal->setReadOnly(true);
    m_terminal->setFont(QFont("Consolas", 10));
    m_terminal->document()->setMaximumBlockCount(5000);
    terminalLayout->addWidget(m_terminal);

    // 发送区
    auto *sendLayout = new QHBoxLayout;
    m_sendHex = new QCheckBox("HEX");
    m_sendEdit = new QLineEdit;
    m_sendEdit->setPlaceholderText("输入要发送的数据...");
    m_sendBtn = new QPushButton("发送");
    m_sendBtn->setFixedWidth(60);
    sendLayout->addWidget(m_sendHex);
    sendLayout->addWidget(m_sendEdit, 1);
    sendLayout->addWidget(m_sendBtn);
    terminalLayout->addLayout(sendLayout);

    // 宏指令面板
    m_macroGroup = new QGroupBox("宏指令");
    auto *macroMainLayout = new QVBoxLayout(m_macroGroup);
    m_macroBtnLayout = new QHBoxLayout;
    m_macroBtnLayout->setContentsMargins(0, 0, 0, 0);
    m_macroBtnLayout->setSpacing(4);
    m_addMacroBtn = new QPushButton("+");
    m_addMacroBtn->setFixedWidth(30);
    m_macroBtnLayout->addWidget(m_addMacroBtn);
    m_macroBtnLayout->addStretch();
    macroMainLayout->addLayout(m_macroBtnLayout);
    terminalLayout->addWidget(m_macroGroup);
    loadMacros();

    splitter->addWidget(terminalGroup);

    // --- 波形区 ---
    auto *waveGroup = new QGroupBox("实时波形");
    auto *waveLayout = new QVBoxLayout(waveGroup);

    auto *waveBar = new QHBoxLayout;
    m_clearWaveBtn = new QPushButton("清空波形");
    waveBar->addStretch();
    waveBar->addWidget(m_clearWaveBtn);
    waveLayout->addLayout(waveBar);

    m_waveform = new WaveformWidget;
    const QStringList chNames = {"通道 0", "通道 1", "通道 2", "通道 3"};
    for (int i = 0; i < 4; ++i) {
        m_waveform->setChannelColor(i, m_channelColors[i]);
        m_waveform->setChannelName(i, chNames[i]);
    }

    waveLayout->addWidget(m_waveform);
    splitter->addWidget(waveGroup);

    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 3);
    mainLayout->addWidget(splitter);
}

void MainWindow::setupConnections()
{
    connect(m_serialWorker, &SerialWorker::portOpened,
            this, [this](bool success, const QString &msg) {
        if (success) {
            m_portOpen = true;
            m_openBtn->setText("关闭串口");
            m_terminal->append(QString("[%1] %2")
                .arg(QDateTime::currentDateTime().toString("hh:mm:ss"), msg));
        } else {
            QMessageBox::warning(this, "串口错误", "无法打开串口: " + msg);
        }
    });

    connect(m_serialWorker, &SerialWorker::portClosed,
            this, [this] {
        m_portOpen = false;
        m_openBtn->setText("打开串口");
        m_terminal->append(QString("[%1] 串口已关闭")
            .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
    });

    connect(m_serialWorker, &SerialWorker::dataReceived,
            this, &MainWindow::onDataReceived);

    connect(m_serialWorker, &SerialWorker::errorOccurred,
            this, [this](const QString &err) {
        m_terminal->append(QString("[%1] 错误: %2")
            .arg(QDateTime::currentDateTime().toString("hh:mm:ss"), err));
    });

    connect(m_openBtn, &QPushButton::clicked, this, &MainWindow::onOpenClose);
    connect(m_sendBtn, &QPushButton::clicked, this, &MainWindow::onSendData);
    connect(m_sendEdit, &QLineEdit::returnPressed, this, &MainWindow::onSendData);
    connect(m_exportBtn, &QPushButton::clicked, this, &MainWindow::onExportCSV);
    connect(m_clearBtn, &QPushButton::clicked, this, &MainWindow::onClearTerminal);
    connect(m_clearWaveBtn, &QPushButton::clicked, this, &MainWindow::onClearWaveform);
    connect(m_displayMode, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onDisplayModeChanged);
    connect(m_addMacroBtn, &QPushButton::clicked, this, &MainWindow::onAddMacro);
}

void MainWindow::onOpenClose()
{
    if (m_portOpen) {
        QMetaObject::invokeMethod(m_serialWorker, "closePort", Qt::QueuedConnection);
        return;
    }

    SerialConfigDialog dlg(this);
    if (dlg.exec() != QDialog::Accepted)
        return;

    QString port = dlg.portName();
    if (port.isEmpty()) {
        QMessageBox::information(this, "提示", "没有可用的串口");
        return;
    }

    QMetaObject::invokeMethod(m_serialWorker, "openPort", Qt::QueuedConnection,
                              Q_ARG(QString, port),
                              Q_ARG(int, dlg.baudRate()),
                              Q_ARG(QSerialPort::DataBits, dlg.dataBits()),
                              Q_ARG(QSerialPort::StopBits, dlg.stopBits()),
                              Q_ARG(QSerialPort::Parity, dlg.parity()),
                              Q_ARG(QSerialPort::FlowControl, dlg.flowControl()));
}

void MainWindow::onSendData()
{
    QString text = m_sendEdit->text().trimmed();
    if (text.isEmpty() || !m_portOpen)
        return;

    QByteArray toSend;
    if (m_sendHex->isChecked()) {
        QStringList hexParts = text.split(' ', Qt::SkipEmptyParts);
        for (const QString &h : hexParts) {
            bool ok;
            quint8 byte = h.toUInt(&ok, 16);
            if (!ok || h.size() > 2) {
                QMessageBox::warning(this, "格式错误", "HEX 格式无效: " + h);
                return;
            }
            toSend.append(static_cast<char>(byte));
        }
    } else {
        toSend = text.toUtf8();
    }

    m_txBytes += toSend.size();
    m_txCountLabel->setText(QString("TX: %1 B").arg(m_txBytes));

    appendToTerminal(toSend, true);

    QMetaObject::invokeMethod(m_serialWorker, "writeData", Qt::QueuedConnection,
                              Q_ARG(QByteArray, toSend));

    m_sendEdit->clear();
}

void MainWindow::onDataReceived(const QByteArray &data)
{
    m_rxBytes += data.size();
    m_rxCountLabel->setText(QString("RX: %1 B").arg(m_rxBytes));

    if (!m_protocolMode->isChecked()) {
        appendToTerminal(data, false);
        return;
    }

    m_rxBuffer.append(data);

    QList<ProtocolParser::Frame> frames = ProtocolParser::parseFrames(m_rxBuffer);

    for (const auto &f : frames) {
        if (!f.valid)
            continue;

        appendToTerminal(ProtocolParser::buildFrame(f.data), false);

        QList<qint16> values = ProtocolParser::extractChannelValues(f.data);
        if (values.size() == 4)
            updateWaveform(values);
    }
}

void MainWindow::appendToTerminal(const QByteArray &data, bool isTx)
{
    QString prefix = isTx ? "TX> " : "RX< ";
    QString text;

    if (m_displayMode->currentIndex() == 0) {
        QString hex;
        for (int i = 0; i < data.size(); ++i) {
            hex += QString("%1 ").arg(static_cast<quint8>(data[i]), 2, 16, QChar('0')).toUpper();
        }
        text = prefix + hex.trimmed();
    } else {
        text = prefix + QString::fromUtf8(data);
    }

    m_terminal->append(text);

    QScrollBar *sb = m_terminal->verticalScrollBar();
    sb->setValue(sb->maximum());
}

void MainWindow::updateWaveform(const QList<qint16> &values)
{
    m_waveX++;

    for (int i = 0; i < values.size() && i < 4; ++i)
        m_waveform->addData(i, m_waveX, values[i]);
}

void MainWindow::onExportCSV()
{
    QString path = QFileDialog::getSaveFileName(this, "导出 CSV",
        QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".csv",
        "CSV 文件 (*.csv)");
    if (path.isEmpty())
        return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "导出失败", "无法写入文件: " + path);
        return;
    }

    QTextStream out(&file);
    out << "时间,RX/TX,数据\n";

    QString text = m_terminal->toPlainText();
    QStringList lines = text.split('\n', Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << ",,";
        out << "\"" << line << "\"\n";
    }

    file.close();
    m_terminal->append(QString("[%1] 已导出至: %2")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"), path));
}

void MainWindow::onClearTerminal()
{
    m_terminal->clear();
    m_rxBytes = 0;
    m_txBytes = 0;
    m_rxCountLabel->setText("RX: 0 B");
    m_txCountLabel->setText("TX: 0 B");
}

void MainWindow::onClearWaveform()
{
    m_waveX = 0;
    m_waveform->clear();
}

void MainWindow::onDisplayModeChanged(int index)
{
    Q_UNUSED(index);
    onClearTerminal();
}

// ======== 宏指令 ========

void MainWindow::loadMacros()
{
    QSettings settings("SerialTool", "SerialTool");
    QJsonArray arr = QJsonDocument::fromJson(settings.value("macros", "[]").toByteArray()).array();
    m_macros.clear();
    for (const auto &v : arr) {
        QJsonObject obj = v.toObject();
        m_macros.append({obj["name"].toString(), obj["data"].toString()});
    }
    refreshMacroButtons();
}

void MainWindow::saveMacros()
{
    QJsonArray arr;
    for (const auto &m : m_macros) {
        QJsonObject obj;
        obj["name"] = m.first;
        obj["data"] = m.second;
        arr.append(obj);
    }
    QSettings settings("SerialTool", "SerialTool");
    settings.setValue("macros", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

void MainWindow::refreshMacroButtons()
{
    while (m_macroBtnLayout->count() > 2) {
        auto *item = m_macroBtnLayout->takeAt(1);
        delete item->widget();
        delete item;
    }
    for (int i = 0; i < m_macros.size(); ++i) {
        auto *btn = new QPushButton(m_macros[i].first);
        btn->setFixedHeight(26);
        btn->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(btn, &QPushButton::clicked, this, &MainWindow::onMacroClicked);
        connect(btn, &QPushButton::customContextMenuRequested, this, &MainWindow::onMacroContextMenu);
        m_macroBtnLayout->insertWidget(m_macroBtnLayout->count() - 1, btn);
    }
}

void MainWindow::onAddMacro()
{
    QString name = QInputDialog::getText(this, "新建宏指令", "名称:");
    if (name.isEmpty()) return;
    QString data = QInputDialog::getText(this, "新建宏指令",
        QString("指令数据 (%1):").arg(m_sendHex->isChecked() ? "HEX" : "文本"),
        QLineEdit::Normal, "", nullptr, Qt::WindowFlags(), m_sendHex->isChecked() ? Qt::ImhLatinOnly : Qt::ImhNone);
    if (data.isEmpty()) return;
    m_macros.append({name, data});
    saveMacros();
    refreshMacroButtons();
}

void MainWindow::onMacroClicked()
{
    auto *btn = qobject_cast<QPushButton*>(sender());
    if (!btn || !m_portOpen) return;
    for (const auto &m : m_macros) {
        if (m.first == btn->text()) {
            m_terminal->append(QString("[%1] 发送宏: %2")
                .arg(QDateTime::currentDateTime().toString("hh:mm:ss"), m.first));
            sendMacroData(m.second);
            return;
        }
    }
}

void MainWindow::sendMacroData(const QString &rawData)
{
    QByteArray toSend;
    if (m_sendHex->isChecked()) {
        QStringList hexParts = rawData.split(' ', Qt::SkipEmptyParts);
        for (const QString &h : hexParts) {
            bool ok;
            quint8 byte = h.toUInt(&ok, 16);
            if (!ok || h.size() > 2) {
                QMessageBox::warning(this, "格式错误", "宏 HEX 格式无效: " + h);
                return;
            }
            toSend.append(static_cast<char>(byte));
        }
    } else {
        toSend = rawData.toUtf8();
    }
    m_txBytes += toSend.size();
    m_txCountLabel->setText(QString("TX: %1 B").arg(m_txBytes));
    appendToTerminal(toSend, true);
    QMetaObject::invokeMethod(m_serialWorker, "writeData", Qt::QueuedConnection,
                              Q_ARG(QByteArray, toSend));
}

void MainWindow::onMacroContextMenu(const QPoint &pos)
{
    auto *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    QMenu menu;
    QAction *editAct = menu.addAction("编辑");
    QAction *delAct = menu.addAction("删除");
    QAction *chosen = menu.exec(btn->mapToGlobal(pos));
    if (!chosen) return;
    for (int i = 0; i < m_macros.size(); ++i) {
        if (m_macros[i].first != btn->text()) continue;
        if (chosen == delAct) {
            m_macros.removeAt(i);
            saveMacros();
            refreshMacroButtons();
        } else if (chosen == editAct) {
            QString name = QInputDialog::getText(this, "编辑宏指令", "名称:",
                QLineEdit::Normal, m_macros[i].first);
            if (name.isEmpty()) return;
            QString data = QInputDialog::getText(this, "编辑宏指令", "指令数据:",
                QLineEdit::Normal, m_macros[i].second);
            if (data.isEmpty()) return;
            m_macros[i] = {name, data};
            saveMacros();
            refreshMacroButtons();
        }
        return;
    }
}
