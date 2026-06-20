#include "waveformwidget.h"
#include <QPainter>
#include <QPaintEvent>

WaveformWidget::WaveformWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(200);
    setMouseTracking(true);
}

void WaveformWidget::addData(int channel, double x, qint16 y)
{
    if (channel < 0 || channel > 3)
        return;

    auto &pts = m_channels[channel].points;
    pts.append(QPointF(x, y));

    while (pts.size() > MAX_POINTS)
        pts.removeFirst();

    // 滑动窗口
    if (x > m_xMax) {
        m_xMax = x;
        m_xMin = x - MAX_POINTS;
    }

    update();
}

void WaveformWidget::clear()
{
    for (int i = 0; i < 4; ++i)
        m_channels[i].points.clear();
    m_xMin = 0;
    m_xMax = MAX_POINTS;
    update();
}

void WaveformWidget::setChannelColor(int channel, const QColor &color)
{
    if (channel >= 0 && channel < 4)
        m_channels[channel].color = color;
}

void WaveformWidget::setChannelName(int channel, const QString &name)
{
    if (channel >= 0 && channel < 4)
        m_channels[channel].name = name;
}

void WaveformWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    p.fillRect(rect(), QColor(30, 30, 30));

    QRect chartRect = rect().adjusted(50, 10, -20, -30);
    drawGrid(p, chartRect);

    // 数据线
    for (int ch = 0; ch < 4; ++ch) {
        const auto &pts = m_channels[ch].points;
        if (pts.size() < 2)
            continue;

        p.setPen(QPen(m_channels[ch].color, 1.5));
        QPolygonF poly;
        for (const auto &pt : pts) {
            double px = chartRect.left() + (pt.x() - m_xMin) / (m_xMax - m_xMin) * chartRect.width();
            double py = chartRect.bottom() - (pt.y() - m_yMin) / (m_yMax - m_yMin) * chartRect.height();
            poly.append(QPointF(px, py));
        }
        p.drawPolyline(poly);
    }

    drawLegend(p, rect());
}

void WaveformWidget::drawGrid(QPainter &p, const QRect &r)
{
    p.setPen(QPen(QColor(60, 60, 60), 1, Qt::DotLine));

    // Y 轴刻度线
    double yRange = m_yMax - m_yMin;
    double yStep = yRange / 8.0;
    for (int i = 0; i <= 8; ++i) {
        int y = r.bottom() - (i * r.height() / 8);
        p.drawLine(r.left(), y, r.right(), y);

        p.setPen(QColor(180, 180, 180));
        p.drawText(0, y - 6, 45, 12, Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(m_yMin + i * yStep, 'f', 0));
        p.setPen(QPen(QColor(60, 60, 60), 1, Qt::DotLine));
    }

    // X 轴边框
    p.setPen(QPen(QColor(100, 100, 100), 1));
    p.drawRect(r);
}

void WaveformWidget::drawLegend(QPainter &p, const QRect &r)
{
    int x = r.left() + 8;
    int y = r.top() + 4;

    p.setFont(QFont("Microsoft YaHei", 9));

    for (int i = 0; i < 4; ++i) {
        QString label = m_channels[i].name.isEmpty()
                            ? QString("CH%1").arg(i) : m_channels[i].name;

        QFontMetrics fm(p.font());
        int textW = fm.horizontalAdvance(label) + 18;

        p.fillRect(x, y, 12, 3, m_channels[i].color);
        p.setPen(QColor(220, 220, 220));
        p.drawText(x + 14, y - 6, textW, 14, Qt::AlignVCenter, label);

        x += textW + 8;
    }
}
