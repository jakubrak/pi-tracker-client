#include "appsink.h"
#include <QtEndian>
#include <QRect>

void AppSink::eos() {

}

QGst::FlowReturn AppSink::newSample() {
    QGst::SamplePtr sample = pullSample();
    QGst::MapInfo mapInfo;
    sample->buffer()->map(mapInfo, QGst::MapRead);
    quint8 *data = mapInfo.data();
    size_t size = mapInfo.size();
    QString hexString;
    for (int i=0; i<size; ++i) {
         hexString += QString("%1 ").arg(data[i], 2, 16, QLatin1Char( '0' ));
    }
    QTime timestamp = QTime::currentTime();
    QString timeString = QString("[%1:%2:%3:%4]")
            .arg(timestamp.hour(), 2)
            .arg(timestamp.minute(), 2)
            .arg(timestamp.second(), 2)
            .arg(timestamp.msec(), 3);
    //qDebug() << "[GSTREAMER] " << timeString << "new KLV data:" << hexString;
    quint32 roiX = 0;
    quint32 roiY = 0;
    quint32 roiW = 0;
    quint32 roiH = 0;
    memcpy(&roiX, &data[2], 4);
    memcpy(&roiY, &data[6], 4);
    memcpy(&roiW, &data[10], 4);
    memcpy(&roiH, &data[14], 4);
    roiX = qFromLittleEndian(roiX);
    roiY = qFromLittleEndian(roiY);
    roiW = qFromLittleEndian(roiW);
    roiH = qFromLittleEndian(roiH);
    QRect roi(roiX, roiY, roiW, roiH);
    //qDebug() << "new ROI: " <<  roiX << ", " << roiY << ", " << roiW << "x" << roiH;
    emit updateRoi(roi);

    return QGst::FlowOk;
}

QGst::FlowReturn AppSink::newPreroll() {
    qDebug() << "new preroll";
    return QGst::FlowOk;
}

