#ifndef SESSION_H
#define SESSION_H

#include <QGst/Bin>
#include <QGst/Caps>

class Session
{
public:
    Session(int id, QGst::CapsPtr caps, QGst::BinPtr bin);
    int getId() const;
    QGst::CapsPtr getCaps() const;
    QGst::BinPtr getBin() const;

private:
    int id;
    QGst::CapsPtr caps;
    QGst::BinPtr bin;
};

#endif // SESSION_H
