#include "clock.h"

extern "C" {
    #include "libavutil/time.h"
}

#include "QtMath"
#include "define.h"

namespace BugAV {
Clock::Clock()
    :pts{0.0}
    ,pts_drift{0.0}
    ,last_updated{0.0}
    ,speed{0}
    ,serial{0}
    ,paused{0}
    ,queue_serial{ nullptr }
{
}

Clock::~Clock()
{

}

void Clock::init(int *queueSerial)
{
    this->queue_serial = queueSerial;
    this->speed = 1.0;
    this->paused = 0;
    set(double(NAN), -1);
}

void Clock::set(double pts, int serial)
{
    double time = av_gettime_relative() / 1000000.0;
    setAt(pts, serial, time);
}

void Clock::setSpeed(double speed)
{
    set(get(), serial);
    this->speed = speed;
}

void Clock::setAt(double pts, int serial, double time)
{
    this->pts = pts;
    this->last_updated = time;
    this->pts_drift = this->pts - time;
    this->serial = serial;
}

double Clock::get()
{
    if (queue_serial == nullptr) {
        return double(NAN);
    }
    if (*this->queue_serial != this->serial)
        return double(NAN);
    if (paused) {
        return pts;
    }
    auto time = av_gettime_relative() / 1000000.0;
    return pts_drift + time - (time - last_updated) * (1.0 - speed);
}

void Clock::syncToSlave(Clock *slave)
{
    double clock = this->get();
    double slave_clock = slave->get();
    if (!std::isnan(slave_clock) && (std::isnan(clock) || std::fabs(clock - slave_clock) > AV_NOSYNC_THRESHOLD)) {
        this->set(slave_clock, slave->serial);
    }
}
}
