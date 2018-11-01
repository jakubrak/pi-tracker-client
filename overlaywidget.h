#ifndef OVERLAYWIDGET_H
#define OVERLAYWIDGET_H

#include <QWidget>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>

class OverlayWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OverlayWidget(QWidget *parent = nullptr);

    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void paintEvent(QPaintEvent *event);

public slots:
    void updateRoi(QRect roi);

signals:
    void setROI(QSize windowSize, QRect roi);

private:
    QPainter *painter;
    QPen pen;
    QPen oldPen;
    QPoint lastPoint;
    QPoint currentPoint;
    QRect roi;
    QRect oldRoi;
    bool settingRoi;
};

#endif // OVERLAYWIDGET_H
