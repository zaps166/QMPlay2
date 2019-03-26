/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2018  Błażej Szczygieł

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

#include <Decoder.hpp>

#include <cuda/CUDAQMPlayTypes.hpp>
#include <cuda/CUVIDQMPlayTypes.hpp>

#include <QCoreApplication>
#include <QQueue>

class CuvidHWAccel;
class VideoWriter;

struct AVBSFContext;
struct SwsContext;
struct AVPacket;

class CuvidDec final : public Decoder
{
    Q_DECLARE_TR_FUNCTIONS(CuvidDec)

    static QMutex loadMutex;
    static int loadState;

public:
    static bool canCreateInstance();

    CuvidDec(Module &module);
    ~CuvidDec();

    bool set() override;

    int videoSequence(CUVIDEOFORMAT *format);
    int pictureDecode(CUVIDPICPARAMS *picParams);
    int pictureDisplay(CUVIDPARSERDISPINFO *dispInfo);

private:
    QString name() const override;

    VideoWriter *HWAccel() const override;

    int decodeVideo(Packet &encodedPacket, VideoFrame &decoded, QByteArray &newPixFmt, bool flush, unsigned hurry_up) override;

    bool hasCriticalError() const override;

    bool open(StreamInfo &streamInfo, VideoWriter *writer = nullptr) override;

    /**/

    bool testDecoder(const int depth);

    bool createCuvidVideoParser();
    void destroyCuvid(bool all);

    inline void resetTimeStampHelpers();

    bool loadLibrariesAndInit();

private:
    VideoWriter *m_writer;
    CuvidHWAccel *m_cuvidHWAccel;

    int m_width, m_height, m_codedHeight;
    CUvideotimestamp m_lastCuvidTS;
    QQueue<double> m_timestamps;
    double m_lastTS[2];

    cudaVideoDeinterlaceMode m_deintMethod;
    Qt::CheckState m_copyVideo;
    bool m_forceFlush;
    bool m_tsWorkaround;

    QQueue<CUVIDPARSERDISPINFO> m_cuvidSurfaces;

    AVBufferRef *m_nv12Chroma;
    AVBufferRef *m_frameBuffer[3];

    AVBSFContext *m_bsfCtx;
    SwsContext *m_swsCtx;
    AVPacket *m_pkt;

    CUcontext m_cuCtx;

    CUVIDEOFORMATEX m_cuvidFmt;
    CUVIDPARSERPARAMS m_cuvidParserParams;
    CUvideoparser m_cuvidParser;
    CUvideodecoder m_cuvidDec;

    bool m_decodeMPEG4;
    bool m_hasCriticalError;
    bool m_skipFrames;
};

#define CuvidName "CUVID decoder"
