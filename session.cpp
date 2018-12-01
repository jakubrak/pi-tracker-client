#include "session.h"

Session::Session(int id, QGst::CapsPtr caps, QGst::BinPtr bin)
    : id{id}, caps{caps}, bin{bin} {

}

int Session::getId() const {
    return id;
}

QGst::CapsPtr Session::getCaps() const {
    return caps;
}

QGst::BinPtr Session::getBin() const {
    return bin;
}
