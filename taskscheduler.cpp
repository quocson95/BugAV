#include "taskscheduler.h"
#include "QThread"
#include <QDebug>

#include <Render/render.h>

#include <Decoder/videodecoder.h>

namespace BugAV {

TaskScheduler::TaskScheduler()
{

//    thread = new QThread{};
//    workerForRender = new Worker(&queueRunableForRender);
//    workerForvDecoder = new Worker(&queueRunableForvDecoder);
    maxTaskPerWorker = 25;
//    worker->start(thread);
    for(auto i = 0; i < 4; i++) {
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
//    queueRunableForRender.clear();
//    queueRunableForvDecoder.clear();
}

void TaskScheduler::addTask(TaskType task)
{
    auto renderTask = dynamic_cast<Render *>(task);
    if (renderTask != nullptr) {
        auto w = getWorkerMinQueue(&workerForRender);
        w->addTask(task);
//        queueRunableForRender.append(task);
//        updateIndexWorker(&workerForRender);
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
    for(auto it = queueWorker->cbegin(); it != queueWorker->cend(); it++) {
        if ((*it)->indexTask(task) >= 0) {
            (*it)->removeTask(task);
            return;
        }
    }
}

}
 // namespace BugAV
