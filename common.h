#ifndef COMMON_H
#define COMMON_H

#include <QTime>
#include <QDebug>

//#define LOG(...) qDebug("[INFO] " + __VA_ARGS__);
#define LOG(...) \
    { \
        qDebug(__VA_ARGS__); \
    }

#endif // COMMON_H

