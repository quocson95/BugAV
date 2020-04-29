#include "taskscheduler.h"
#include "QThread"
#include <QDebug>
#include <QApplication>
#include <Render/render.h>

#include <Decoder/videodecoder.h>

namespace BugAV {

TaskScheduler::TaskScheduler()
    :QObject(nullptr)
{
    for(auto i = 0; i < 10; i++) {
//        auto mutex = new QMutex;
        workerForRender.append(new Worker());
//        mutexsForRender.append(mutex);

    }
    for(auto i = 0; i < 10; i++) {
//        auto mutex = new QMutex;
        workerForvDecoder.append(new Worker());
//        mutexsForvDecoder.append(mutex);
    }
    start();
}

BugAV::TaskScheduler::~TaskScheduler()
{
    qDebug() << "TaskScheduler Desktroy";
    stop();
    qDeleteAll(workerForRender);
    qDeleteAll(workerForvDecoder);
}

void TaskScheduler::addTask(TaskType task)
{
    auto renderTask = dynamic_cast<Render *>(task);
    if (renderTask != nullptr) {
        auto w = getWorkerMinQueue(&workerForRender);
        w->addTask(task);
        return;
    }
    auto vDecoderTask = dynamic_cast<VideoDecoder *>(task);
    if (vDecoderTask != nullptr) {
        auto w = getWorkerMinQueue(&workerForvDecoder);
        w->addTask(task);
        return;
    }
}

void TaskScheduler::removeTask(TaskType task)
{
    auto renderTask = dynamic_cast<Render *>(task);
    if (renderTask != nullptr) {
        removeTask(&workerForRender, task);
    }

    auto vDecoderTask = dynamic_cast<VideoDecoder *>(task);
    if (vDecoderTask != nullptr) {
        removeTask(&workerForvDecoder, task);
    }
}

void TaskScheduler::start()
{
    for(auto it = workerForRender.cbegin(); it != workerForRender.cend(); it++) {
        (*it)->start();
    }
    for(auto it = workerForvDecoder.cbegin(); it != workerForvDecoder.cend(); it++) {
        (*it)->start();
    }
}

void TaskScheduler::stop()
{
    for(auto it = workerForRender.cbegin(); it != workerForRender.cend(); it++) {
        (*it)->stop();
    }
    for(auto it = workerForvDecoder.cbegin(); it != workerForvDecoder.cend(); it++) {
        (*it)->stop();
    }
}

Worker *TaskScheduler::getWorkerMinQueue(QVector<Worker *> *queueWorker)
{
    auto index = -1;
    auto minSize = INT_MAX;
    for(auto i = 0; i < queueWorker->size(); i++) {
        auto w = queueWorker->at(i);
        if (minSize > w->queueTaskSize()) {
            minSize = w->queueTaskSize();
            index = i;
        }
    }
    return queueWorker->at(index);
}

void TaskScheduler::removeTask(QVector<Worker *> *queueWorker, TaskType task)
{
    QMutexLocker lock(&mutex);
    Q_UNUSED(lock)
    for(auto it = queueWorker->cbegin(); it != queueWorker->cend(); it++) {
        auto index = (*it)->indexTask(task);
        if ( index >= 0) {
            (*it)->removeTask(index);
            return;
        }
    }
}

}
 // namespace BugAV
