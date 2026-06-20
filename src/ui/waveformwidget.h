#pragma once

#include <QWidget>
#include <QVector>
#include <QColor>

class WaveformWidget : public QWidget
{
    Q_OBJECT
public:
    explicit WaveformWidget(QWidget *parent = nullptr);

    void addData(int channel, double x, qint16 y);
    void clear();
    void setChannelColor(int channel, const QColor &color);
    void setChannelName(int channel, const QString &name);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    struct ChannelData {
        QString name;
        QColor color;
        QVector<QPointF> points;
    };

    static constexpr int MAX_POINTS = 500;
    ChannelData m_channels[4];
    double m_xMin = 0;
    double m_xMax = 500;
    double m_yMin = -500;
    double m_yMax = 3500;

    void drawGrid(QPainter &p, const QRect &rect);
    void drawLegend(QPainter &p, const QRect &rect);
};
