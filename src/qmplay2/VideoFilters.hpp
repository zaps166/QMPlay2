// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <VideoFilter.hpp>

#include <QWaitCondition>
#include <QThread>
#include <QVector>
#include <QMutex>
#include <QQueue>

#include <memory>

class VideoFiltersThr;

class QMPLAY2SHAREDLIB_EXPORT VideoFilters
{
    Q_DISABLE_COPY(VideoFilters)
    friend class VideoFiltersThr;
public:
    static void averageTwoLines(quint8 *dest, const quint8 *src1, const quint8 *src2, int linesize);

    VideoFilters();
    ~VideoFilters();

    void start();
    void clear();

    std::shared_ptr<VideoFilter> on(const QString &filterName, bool isHw);
    void on(const std::shared_ptr<VideoFilter> &videoFilter);
    void off(std::shared_ptr<VideoFilter> &videoFilter);

    void clearBuffers();
    void removeLastFromInputBuffer();

    void addFrame(const Frame &videoFrame);
    bool getFrame(Frame &videoFrame);

    bool readyRead();
private:
    QQueue<Frame> outputQueue;
    QVector<std::shared_ptr<VideoFilter>> filters;
    VideoFiltersThr &filtersThr;
    bool outputNotEmpty = false;
};
