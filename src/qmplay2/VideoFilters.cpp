/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2024  Błażej Szczygieł

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <VideoFilters.hpp>

#include <Frame.hpp>
#include <Module.hpp>

class VideoFiltersThr final : public QThread
{
public:
    VideoFiltersThr(VideoFilters &videoFilters) :
        videoFilters(videoFilters)
    {
        setObjectName("VideoFiltersThr");
    }
    ~VideoFiltersThr()
    {
        stop();
    }

    void start()
    {
        br = filtering = false;
        QThread::start();
    }
    void stop()
    {
        {
            QMutexLocker locker(&mutex);
            br = true;
            cond.wakeOne();
        }
        wait();
    }

    void filterFrame(const Frame &frame)
    {
        QMutexLocker locker(&mutex);
        frameToFilter = frame;
        filtering = true;
        cond.wakeOne();
    }

    void waitForFinished(bool waitForAllFrames)
    {
        bufferMutex.lock();
        while (filtering && !br)
        {
            if (!waitForAllFrames && !videoFilters.outputQueue.isEmpty())
                break;
            cond.wait(&bufferMutex);
        }
        if (waitForAllFrames)
            bufferMutex.unlock();
    }

    QMutex bufferMutex;
private:
    void run() override
    {
        while (!br)
        {
            QMutexLocker locker(&mutex);

            if (frameToFilter.isEmpty() && !br)
                cond.wait(&mutex);
            if (frameToFilter.isEmpty() || br)
                continue;

            QQueue<Frame> queue;
            queue.enqueue(frameToFilter);
            frameToFilter.clear();

            bool pending = false;
            do
            {
                for (const std::shared_ptr<VideoFilter> &vFilter : qAsConst(videoFilters.filters))
                    pending |= vFilter->filter(queue);

                if (queue.isEmpty())
                    pending = false;

                {
                    QMutexLocker locker(&bufferMutex);
                    if (!queue.isEmpty())
                    {
                        videoFilters.outputQueue.append(queue);
                        videoFilters.outputNotEmpty = true;
                        queue.clear();
                    }
                    if (!pending)
                        filtering = false;
                }

                cond.wakeOne();
            } while (pending && !br);
        }
        if (br)
        {
            QMutexLocker locker(&bufferMutex);
            filtering = false;
            cond.wakeOne();
        }
    }

    VideoFilters &videoFilters;

    bool br = false, filtering = false;

    QWaitCondition cond;
    QMutex mutex;

    Frame frameToFilter;
};

/**/

void VideoFilters::averageTwoLines(quint8 *__restrict__ dest, const quint8 *__restrict__ src1, const quint8 *__restrict__ src2, int linesize)
{
    for (int i = 0; i < linesize; ++i)
        dest[i] = (src1[i] + src2[i] + 1) >> 1; // This generates "pavgb" instruction on x86
}

VideoFilters::VideoFilters() :
    filtersThr(*(new VideoFiltersThr(*this))),
    outputNotEmpty(false)
{}
VideoFilters::~VideoFilters()
{
    clear();
    delete &filtersThr;
}

void VideoFilters::start()
{
    if (!filters.isEmpty())
        filtersThr.start();
}
void VideoFilters::clear()
{
    if (!filters.isEmpty())
    {
        filtersThr.stop();
        filters.clear();
    }
    clearBuffers();
}

std::shared_ptr<VideoFilter> VideoFilters::on(const QString &filterName, bool isHw)
{
    if (filterName.isEmpty())
        return nullptr;
    std::shared_ptr<VideoFilter> filter;
    for (Module *module : QMPlay2Core.getPluginsInstance())
    {
        for (const Module::Info &mod : module->getModulesInfo())
        {
            if ((mod.type & 0xF) == Module::VIDEOFILTER && (!isHw || (mod.type & Module::DATAPRESERVE)) && mod.name == filterName)
            {
                filter.reset(reinterpret_cast<VideoFilter *>(module->createInstance(mod.name)));
                break;
            }
        }
    }
    on(filter);
    return filter;
}
void VideoFilters::on(const std::shared_ptr<VideoFilter> &videoFilter)
{
    if (videoFilter)
        filters.append(videoFilter);
}
void VideoFilters::off(std::shared_ptr<VideoFilter> &videoFilter)
{
    const int idx = filters.indexOf(videoFilter);
    if (idx > -1)
    {
        filters.remove(idx);
        videoFilter.reset();
    }
}

void VideoFilters::clearBuffers()
{
    if (!filters.isEmpty())
    {
        filtersThr.waitForFinished(true);
        for (auto &&vFilter : qAsConst(filters))
            vFilter->clearBuffer();
    }
    outputQueue.clear();
    outputNotEmpty = false;
}
void VideoFilters::removeLastFromInputBuffer()
{
    if (!filters.isEmpty())
    {
        filtersThr.waitForFinished(true);
        for (int i = filters.count() - 1; i >= 0; --i)
            if (filters[i]->removeLastFromInternalBuffer())
                break;
    }
}

void VideoFilters::addFrame(const Frame &videoFrame)
{
    if (!filters.isEmpty())
    {
        filtersThr.filterFrame(videoFrame);
    }
    else
    {
        outputQueue.enqueue(videoFrame);
        outputNotEmpty = true;
    }
}
bool VideoFilters::getFrame(Frame &videoFrame)
{
    bool locked, ret;
    if ((locked = !filters.isEmpty()))
        filtersThr.waitForFinished(false);
    if ((ret = !outputQueue.isEmpty()))
    {
        videoFrame = outputQueue.at(0);
        outputQueue.removeFirst();
        outputNotEmpty = !outputQueue.isEmpty();
    }
    if (locked)
        filtersThr.bufferMutex.unlock();
    return ret;
}

bool VideoFilters::readyRead()
{
    filtersThr.waitForFinished(false);
    const bool ret = outputNotEmpty;
    filtersThr.bufferMutex.unlock();
    return ret;
}
