#include "overlaywidget.h"

#include <QDebug>


OverlayWidget::OverlayWidget(QWidget *parent) : QWidget(parent) {
    //this->setWindowOpacity(0);
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_PaintOnScreen);
    painter = new QPainter(this);
    pen.setColor(Qt::red);
    pen.setWidth(2);
    oldPen.setColor(Qt::blue);
    oldPen.setWidth(2);
    settingRoi = false;
}

void OverlayWidget::mousePressEvent(QMouseEvent *event) {
    settingRoi = true;
    if (event->button() == Qt::LeftButton) {
        roi = QRect(event->pos(), event->pos());
    } else {
        roi.setTopLeft(roi.bottomRight());
    }
}

void OverlayWidget::mouseMoveEvent(QMouseEvent *event) {
    settingRoi = true;
    if (event->buttons() & Qt::LeftButton) {
        roi.setBottomRight(event->pos());
    }
}

void OverlayWidget::mouseReleaseEvent(QMouseEvent *event) {
    settingRoi = false;
    qDebug() << "new ROI:" << this->size() << roi.normalized();
    emit setROI(this->size(), roi.normalized());
}

void OverlayWidget::paintEvent(QPaintEvent *event) {
    if (roi.topLeft() != roi.bottomRight()) {
        painter->begin(this);
        painter->setPen(pen);
        painter->drawRect(roi);
        painter->end();
    }
}

void OverlayWidget::updateRoi(QRect roi) {
    if (settingRoi == false) {
        this->roi = roi;
    }
}
