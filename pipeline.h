#ifndef PIPELINE_H
#define PIPELINE_H

#include <memory>

#include <QSize>

#include <QGst/Pipeline>

class AppSink;
class Session;

class Pipeline : public QObject
{
     Q_OBJECT

public:
    Pipeline(QObject *parent = nullptr);

    ~Pipeline();

    void play();

    const QGst::ElementPtr getVideoSink() {
        return videoSink;
    }

signals:
    void frameSizeChanged(QSize rect);

private:
    std::unique_ptr<Session> makeVideoSession();

    std::unique_ptr<Session> makeMetadataSession();

    void joinSession(QGst::ElementPtr rtpBin, int rtpPort, const Session& session);

    QString toString(QGst::State state);

    void onBusMessage(const QGst::ObjectPtr object, const QGst::MessagePtr &message);

    void onPadAdded(const QGst::PadPtr &pad);

    void onUpdate(const QGst::ElementPtr &element);

    void checkCaps();

    QGst::PipelinePtr pipeline;
    QGst::ElementPtr videoSink;
    QGst::CapsPtr videoCaps;
    std::unique_ptr<AppSink> appSink;
    std::unique_ptr<Session> videoSession;
    std::unique_ptr<Session> metadataSession;
};

#endif // PIPELINE_H
