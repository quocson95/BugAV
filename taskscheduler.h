#ifndef TASKSCHEDULER_H
#define TASKSCHEDULER_H

#include <QMutex>
#include <QPointer>
#include <QRunnable>
#include <QThread>
#include <QVector>

namespace BugAV {

using TaskType = QRunnable *;
class Worker;

class TaskScheduler{
public:

    static TaskScheduler& instance() {
        static TaskScheduler ins;
        return ins;
    }

    TaskScheduler(TaskScheduler const&)          = delete;
    void operator=(TaskScheduler const&)  = delete;

    ~TaskScheduler();

    void addTask(TaskType task);

    void removeTask(TaskType task);

//    void removeTask(QVector<TaskType> *queue,  QVector<QMutex *> *mutexs, QVector<Worker *> *worker, TaskType task);

    void start();
    void stop();

//    void updateIndexWorker(QVector<Worker *> *queue);

private:
    TaskScheduler();
    Worker * getWorkerMinQueue(QVector<Worker *> *queueWorker);

    void removeTask(QVector<Worker *> *queueWorker, TaskType task);
//    QThread *thread;
//    QVector<TaskType> queueRunableForRender;
//    QVector<TaskType> queueRunableForvDecoder;

//    QVector<QMutex *> mutexsForRender;
//    QVector<QMutex *> mutexsForvDecoder;

    QVector<Worker *> workerForRender;
    QVector<Worker *> workerForvDecoder;

    int maxTaskPerWorker;
};

class Worker:public QObject {
    Q_OBJECT
public:
    Worker(QObject *parent = nullptr)
        :QObject{parent}
        ,thread{nullptr}
    {        
    }

    void setStop() {
        requestStop = true;
    }


    void start() {
        requestStop = false;
        if (thread == nullptr) {
            thread = new QThread{this};
            moveToThread(thread);
            connect(thread, &QThread::started, this, &Worker::process);
        }
        if (!thread->isRunning()) {
            thread->start();
        }
    }

    inline int queueTaskSize() const {
        return queueTask.size();
    }

    void addTask(TaskType task) {
        queueTask.append(task);
    }

    int indexTask(TaskType task) const {
        return queueTask.indexOf(task);
    }

    void removeTask(TaskType task) {
        mutex.lock();
        queueTask.removeOne(task);
        mutex.unlock();
    }

    void stop() {
        requestStop = true;
        thread->quit();
        do {
            thread->wait(2000);
        } while(thread->isRunning());
    }    

    ~Worker() {}

private slots:
    void process() {
            forever {
                if (requestStop) {
                    thread->quit();
                    break;
                }
                mutex.lock();
                if (queueTask.size() == 0) {
                    thread->msleep(500);
                    mutex.unlock();
                    continue;
                }
                for(auto it = queueTask.cbegin(); it != queueTask.cend(); it++) {
                    if (requestStop) {
                        mutex.unlock();
                        return;
                    }
                    (*it)->run();
                }
                mutex.unlock();
                thread->usleep(2*1000);
            }
    }
private:
    bool requestStop;
    QThread *thread;
    QVector<TaskType> queueTask;
    QMutex mutex;
};

}
#endif // TASKSCHEDULER_H
