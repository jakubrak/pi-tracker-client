#ifndef PIPELINE_H
#define PIPELINE_H

#include <memory>

#include <QSize>
#include <QRect>

#include <QGst/Pipeline>

class AppSink;

class Pipeline : public QObject {
     Q_OBJECT

public:
    Pipeline(const QString &host, int videoPort, int metadataPort, QObject *parent = nullptr);

    ~Pipeline();

    void play();

    const QGst::ElementPtr getVideoSink() {
        return videoSink;
    }

signals:
    void frameSizeChanged(QSize rect);
    void updateRoi(QRect roi);

private:
    class Session {
    public:
        Session(int id, QGst::CapsPtr caps, QGst::BinPtr bin, int port);
        int getId() const;
        QGst::CapsPtr getCaps() const;
        QGst::BinPtr getBin() const;
        int getPort() const;

    private:
        int id;
        QGst::CapsPtr caps;
        QGst::BinPtr bin;
        int port;
    };

    std::unique_ptr<Session> makeVideoSession(const int port);

    std::unique_ptr<Session> makeMetadataSession(const int port);

    void joinSession(QGst::ElementPtr rtpBin, const Session& session);

    QString toString(QGst::State state);

    void onBusMessage(const QGst::ObjectPtr object, const QGst::MessagePtr &message);

    void onPadAdded(const QGst::PadPtr &pad);

    void onUpdate(const QGst::ElementPtr &element);

    void checkCaps();

    QGst::PipelinePtr pipeline;
    QGst::ElementPtr videoSink;
    QGst::CapsPtr videoCaps;

    AppSink *appSink;

    std::unique_ptr<Session> videoSession;
    std::unique_ptr<Session> metadataSession;

    QString host;
};

#endif // PIPELINE_H
