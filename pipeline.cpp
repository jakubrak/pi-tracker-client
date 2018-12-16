#include "pipeline.h"

#include <regex>

#include <QString>

#include <QGlib/Connect>
#include <QGst/Bus>
#include <QGst/ElementFactory>
#include <QGst/GhostPad>
#include <QGst/Message>
#include <QGst/Pad>
#include <QGst/Buffer>

#include "appsink.h"

Pipeline::Pipeline(const QString &host, int videoPort, int metadataPort, QObject *parent) :
    QObject{parent},
    host{host} {
    pipeline = QGst::Pipeline::create();

    auto rtpBin = QGst::ElementFactory::make("rtpbin");
    if (!rtpBin) {
        qFatal("Failed to create element. Aborting...");
    }

    // watch the bus
    if (QGlib::connect(pipeline->bus(), "message", this, &Pipeline::onBusMessage, QGlib::PassSender) == false) {
         qDebug() << "Failed to connect message::error signal from " << pipeline->name();
    }

    pipeline->bus()->addSignalWatch();

    pipeline->add(rtpBin);
    rtpBin->setProperty("latency", 50);
    rtpBin->setProperty("do-retransmission", false);

    QGlib::connect(rtpBin, "pad-added", this, &Pipeline::onPadAdded);

    appSink = new AppSink(this);
    videoSession = makeVideoSession(videoPort);
    metadataSession = makeMetadataSession(metadataPort);

    qDebug() << "Join sessions";

    joinSession(rtpBin, *videoSession);
    joinSession(rtpBin, *metadataSession);

    connect(appSink, SIGNAL(updateRoi(QRect)), this, SIGNAL(updateRoi(QRect)));
}

Pipeline::~Pipeline() {
    if (pipeline.isNull() == false) {
        pipeline->setState(QGst::StateNull);
        pipeline.clear();
    }
}

void Pipeline::play() {
    pipeline->setState(QGst::StatePlaying);
}

QString Pipeline::toString(QGst::State state) {
    switch(state) {
    case QGst::StateNull:
        return "Null";
    case QGst::StatePaused:
        return "Paused";
    case QGst::StatePlaying:
        return "Playing";
    case QGst::StateReady:
        return "Ready";
    case QGst::StateVoidPending:
        return "VoidPending";
    }
}

void Pipeline::onBusMessage(const QGst::ObjectPtr object, const QGst::MessagePtr &message) {
    //qDebug() << message->typeName();
    switch (message->type()) {
    case QGst::MessageEos:
        exit(0);
    case QGst::MessageError:
        qCritical() << message.staticCast<QGst::ErrorMessage>()->error();
        break;
    case QGst::MessageWarning:
        qDebug() << message.staticCast<QGst::WarningMessage>()->error();
        break;
    case QGst::MessageStateChanged:
        qDebug() << "[GSTREAMER] Pipeline state changed to"
                 << toString(message.staticCast<QGst::StateChangedMessage>()->newState());
        if (message.staticCast<QGst::StateChangedMessage>()->newState() == QGst::StatePaused)
                pipeline->setState(QGst::StatePlaying);
        break;
    case QGst::MessageElement:
        qDebug() << "[GSTREAMER] Element message: " << object.staticCast<QGst::Element>()->name();
        for (uint index=0; index < pipeline->childrenCount(); ++index) {
            QGst::ElementPtr elem = pipeline->childByIndex(index).staticCast<QGst::Element>();
            QGst::State state, statePending;
            elem->getState(&state, &statePending, 0);
            qDebug() << elem->name() << ":" << toString(state);
        }
        break;
    default:
        break;
    }

    // workaround to get frame size from pipeline
    checkCaps();
}

std::unique_ptr<Pipeline::Session> Pipeline::makeVideoSession(const int port) {
    auto bin = QGst::Bin::create("video");


    auto depayloader = QGst::ElementFactory::make("rtph264depay");
    if (!depayloader) {
        qFatal("Failed to create element. Aborting...");
    }


    auto decoder = QGst::ElementFactory::make("avdec_h264");
    if (!decoder) {
        qFatal("Failed to create element. Aborting...");
    }

    auto converter = QGst::ElementFactory::make("videoconvert");
    if (!converter) {
        qFatal("Failed to create element. Aborting...");
    }

    videoSink = QGst::ElementFactory::make("qt5videosink");
    if (!videoSink) {
        qFatal("Failed to create element. Aborting...");
    }

    videoSink->setProperty("sync", true);
    videoSink->setProperty("async", true);
    videoSink->setProperty("force-aspect-ratio", true);
    QGlib::connect(videoSink, "update", this, &Pipeline::onUpdate, QGlib::PassSender);

    bin->add(depayloader, decoder, converter, videoSink);
    bin->linkMany(depayloader, decoder, converter, videoSink);

    auto caps = QGst::Caps::fromString("application/x-rtp,"
                                       "media=video,"
                                       "encoding-name=H264,"
                                       "payload=96,"
                                       "clock-rate=90000");

    bin->addPad(QGst::GhostPad::create(depayloader->getStaticPad("sink"), "sink"));

    return std::make_unique<Session>(1, caps, bin, port);
}

std::unique_ptr<Pipeline::Session> Pipeline::makeMetadataSession(const int port)
{
    auto bin = QGst::Bin::create("klv");

    auto depayloader = QGst::ElementFactory::make("rtpklvdepay");
    if (!depayloader) {
        qFatal("Failed to create element. Aborting...");
    }

    auto capsFilter = QGst::ElementFactory::make("capsfilter");
    if (!capsFilter) {
        qFatal("Failed to create element. Aborting...");
    }
    capsFilter->setProperty("caps", QGst::Caps::fromString("meta/x-klv"));

    appSink->setCaps(QGst::Caps::fromString("meta/x-klv"));
    appSink->element()->setProperty("sync", true);
    appSink->element()->setProperty("async", true);

    bin->add(depayloader, capsFilter, appSink->element());
    bin->linkMany(depayloader, capsFilter, appSink->element());

    auto caps = QGst::Caps::fromString("application/x-rtp,"
                                       "media=application,"
                                       "encoding-name=SMPTE336M,"
                                       "payload=96,"
                                       "clock-rate=90000");

    bin->addPad(QGst::GhostPad::create(depayloader->getStaticPad("sink"), "sink"));

    return std::make_unique<Session>(2, caps, bin, port);
}

void Pipeline::onPadAdded(const QGst::PadPtr &pad) {
    qDebug() << "Pad added";
    std::regex re("recv_rtp_src_([0-9]+)");
    std::cmatch m;
    pad->name().sprintf("recv_rtp_src_");

    if (std::regex_search(pad->name().toStdString().c_str(), m, re)) {
        auto sessionId = std::stoi(m[1]);
        if (sessionId == videoSession->getId()) {
            qDebug() << "Video session";
            pipeline->add(videoSession->getBin());
            videoSession->getBin()->syncStateWithParent();
            auto videoPad =  videoSession->getBin()->getStaticPad("sink");
            if (!videoPad) {
                qDebug() << "Something is wrong with static pad";
            }
            pad->link(videoPad);

        } else if (sessionId == metadataSession->getId()) {
            qDebug() << "Metadata session";
            pipeline->add(metadataSession->getBin());
            metadataSession->getBin()->syncStateWithParent();
            pad->link(metadataSession->getBin()->getStaticPad("sink"));
        }
    }
}

void Pipeline::joinSession(QGst::ElementPtr rtpBin, const Session& session) {
    auto rtpSrc = QGst::ElementFactory::make("udpsrc");
    if (!rtpSrc) {
        qFatal("Failed to create udpsrc. Aborting...");
    }
    rtpSrc->setProperty("port", session.getPort());
    rtpSrc->setProperty("caps", session.getCaps());
    pipeline->add(rtpSrc);
    rtpSrc->link(rtpBin, std::string("recv_rtp_sink_" + std::to_string(session.getId())).c_str());

    // recv sink will be added when the pad becomes available

    // video rtcp connection
    auto rtcpSrc = QGst::ElementFactory::make("udpsrc");
    rtcpSrc->setProperty("port", session.getPort() + 1);
    pipeline->add(rtcpSrc);
    rtcpSrc->link(rtpBin, std::string("recv_rtcp_sink_" + std::to_string(session.getId())).c_str());

    auto rtcpSink = QGst::ElementFactory::make("udpsink");
    rtcpSink->setProperty("port", session.getPort() + 2);
    rtcpSink->setProperty("host", host);
    rtcpSink->setProperty("sync", false);
    rtcpSink->setProperty("async", false);
    pipeline->add(rtcpSrc);
    rtcpSrc->link(rtpBin, std::string("recv_rtcp_src_" + std::to_string(session.getId())).c_str());
}

void Pipeline::onUpdate(const QGst::ElementPtr &element) {
    //qDebug() << "UPDATE";
}

void Pipeline::checkCaps() {
    auto pad = videoSink->getStaticPad("sink");
    if (pad) {
        auto caps = pad->currentCaps();
        if (caps) {
            if (!videoCaps || !videoCaps->equals(caps)) {
                videoCaps = caps;
                emit frameSizeChanged(QSize(caps->internalStructure(0)->value("width").toInt(),
                                            caps->internalStructure(0)->value("height").toInt()));
            }
        }
    }
}

Pipeline::Session::Session(int id, QGst::CapsPtr caps, QGst::BinPtr bin, int port)
    : id{id}, caps{caps}, bin{bin}, port{port} {

}

int Pipeline::Session::getId() const {
    return id;
}

QGst::CapsPtr Pipeline::Session::getCaps() const {
    return caps;
}

QGst::BinPtr Pipeline::Session::getBin() const {
    return bin;
}

int Pipeline::Session::getPort() const {
    return port;
}
