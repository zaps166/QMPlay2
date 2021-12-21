/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2021  Błażej Szczygieł

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

#pragma once

#include <IOController.hpp>
#include <StreamInfo.hpp>

#include <QString>
#include <QThread>
#include <QMutex>
#include <QTimer>

class BufferInfo;
class PlayClass;
class AVThread;
class Demuxer;
class BasicIO;

class DemuxerThr final : public QThread
{
    friend class DemuxerTimer;
    friend class PlayClass;
    Q_OBJECT
private:
    DemuxerThr(PlayClass &);
    ~DemuxerThr();

    QByteArray getCoverFromStream() const;

    inline bool isDemuxerReady() const
    {
        return demuxerReady;
    }
    inline bool canSeek() const
    {
        return !unknownLength;
    }

    void loadImage();

    void seek(bool doDemuxerSeek);

    void stop();
    void end();

    void emitInfo();

    bool load(bool canEmitInfo = true);

    void checkReadyWrite(AVThread *avThr);

    void run() override;

    inline void ensureTrueUpdateBuffered();
    inline bool canUpdateBuffered() const;
    void handlePause();
    void emitBufferInfo(bool clearBackwards);

    void updateCoverAndPlaying(bool doCompare);

    void addStreamsMenuString(QStringList &streamsMenu, const QString &idxStr, const QString &link, bool current, const QString &additional);

    void addSubtitleStream(bool, QString &, int, int, const QString &, const QString &, const QString &, QStringList &, const QVector<QMPlay2Tag> &other_info = QVector<QMPlay2Tag>());

    bool mustReloadStreams();
    bool bufferedAllPackets(int vS, int aS, int p);
    bool emptyBuffers(int vS, int aS);
    bool canBreak(const AVThread *avThr1, const AVThread *avThr2);
    double getAVBuffersSize(int &vS, int &aS);
    BufferInfo getBufferInfo(bool clearBackwards);
    void clearBuffers();

    double getFrameDelay() const;

    PlayClass &playC;

    QString name, url, updatePlayingName;

    int minBuffSizeLocal, minBuffSizeNetwork;
    bool err, updateBufferedSeconds, demuxerReady, hasCover, skipBufferSeek, localStream, unknownLength, waitingForFillBufferB, paused, demuxerPaused;
    QMutex stopVAMutex, endMutex, seekMutex;
    IOController<> ioCtrl;
    IOController<Demuxer> demuxer;
    QString title, artist, album;
    double playIfBuffered, time, updateBufferedTime;
private slots:
    void stopVADec();
    void updateCover(const QString &title, const QString &artist, const QString &album, const QByteArray &cover);
signals:
    void load(Demuxer *);
};

/**/

class DemuxerTimer : public QObject
{
    Q_OBJECT
public:
    DemuxerTimer(DemuxerThr &demuxerThr);

    inline void start();
    inline void stop();
private slots:
    void timeout();
private:
    DemuxerThr &demuxerThr;
    QTimer t;
};
