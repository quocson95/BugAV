#ifndef CLOCK_H
#define CLOCK_H

namespace BugAV {

class Clock {
public:
    Clock();
    ~Clock();
    void init(int *queueSerial);
    void set(double pts, int serial);
    void setAt(double pts, int serial, double time);
    double get();

    void syncToSlave(Clock *slave);
public:
    double pts;           /* clock base */
    double pts_drift;     /* clock base minus time at which we updated the clock */
    double last_updated;
    double speed;
    int serial;           /* clock is based on a packet with this serial */
    int paused;
    int *queue_serial;
};
}
#endif // CLOCK_H
