#ifndef APPSINK_H
#define APPSINK_H

#include <QGst/Utils/ApplicationSink>
#include <QGst/Utils/ApplicationSource>
#include <QDebug>
#include <QByteArray>
#include <QRect>

class AppSink :  public QObject, public QGst::Utils::ApplicationSink
{
    Q_OBJECT

public:
    AppSink(QObject *parent = 0) : QObject(parent) { }

signals:
    void setRoi(QRect roi);

protected:
    virtual void eos();

    virtual QGst::FlowReturn newSample();

    virtual QGst::FlowReturn newPreroll();
};

#endif // APPSINK_H
