#pragma once

#include <QMainWindow>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPair>
#include <QThread>
#include <QSettings>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QInputDialog>
#include <QMenu>

#include "waveformwidget.h"

class SerialWorker;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onOpenClose();
    void onSendData();
    void onDataReceived(const QByteArray &data);
    void onExportCSV();
    void onClearTerminal();
    void onClearWaveform();
    void onDisplayModeChanged(int index);

private:
    void setupUI();
    void setupConnections();
    void appendToTerminal(const QByteArray &data, bool isTx);
    void updateWaveform(const QList<qint16> &values);
    void loadMacros();
    void saveMacros();
    void refreshMacroButtons();
    void onAddMacro();
    void onMacroClicked();
    void onMacroContextMenu(const QPoint &pos);
    void sendMacroData(const QString &hexData);

    // 串口线程
    QThread *m_serialThread;
    SerialWorker *m_serialWorker;

    // 串口状态
    bool m_portOpen = false;

    // 数据缓冲（用于粘包/半包处理）
    QByteArray m_rxBuffer;

    // 终端
    QTextEdit *m_terminal;
    QCheckBox *m_protocolMode;  // 协议解析开关
    QComboBox *m_displayMode;  // HEX / ASCII
    QCheckBox *m_sendHex;      // 发送 HEX 模式
    QLineEdit *m_sendEdit;
    QPushButton *m_sendBtn;
    QPushButton *m_openBtn;
    QPushButton *m_exportBtn;
    QPushButton *m_clearBtn;

    // 波形
    WaveformWidget *m_waveform;
    QPushButton *m_clearWaveBtn;
    double m_waveX = 0;
    static constexpr int MAX_WAVE_POINTS = 500;
    QList<QColor> m_channelColors;

    // 宏指令
    QGroupBox *m_macroGroup = nullptr;
    QHBoxLayout *m_macroBtnLayout = nullptr;
    QPushButton *m_addMacroBtn = nullptr;
    QList<QPair<QString, QString>> m_macros;  // name, hexData

    // 统计
    QLabel *m_rxCountLabel;
    QLabel *m_txCountLabel;
    int m_rxBytes = 0;
    int m_txBytes = 0;
};
