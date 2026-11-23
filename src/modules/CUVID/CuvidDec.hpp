// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <CUVIDHeaders.hpp>

#include <Decoder.hpp>

class CuvidHWInterop;
class VideoWriter;

class CuvidDec final : public Decoder
{
    Q_DECLARE_TR_FUNCTIONS(CuvidDec)

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

    bool hasHWDecContext() const override;
    std::shared_ptr<VideoFilter> hwAccelFilter() const override;

    void setSupportedPixelFormats(const AVPixelFormats &pixelFormats) override;

    int decodeVideo(const Packet &encodedPacket, Frame &decoded, AVPixelFormat &newPixFmt, bool flush, unsigned hurry_up) override;

    bool hasCriticalError() const override;

    bool open(StreamInfo &streamInfo) override;

    /**/

    bool testDecoder(const int depth);

    bool createCuvidVideoParser();
    void destroyCuvid(bool all);

    inline void resetTimeStampHelpers();

    bool loadLibrariesAndInit();

private:
    std::shared_ptr<VideoFilter> m_filter;
    std::shared_ptr<CuvidHWInterop> m_cuvidHwInterop;

    bool m_limited;
    AVColorPrimaries colorPrimaries = AVCOL_PRI_UNSPECIFIED;
    AVColorTransferCharacteristic colorTrc = AVCOL_TRC_UNSPECIFIED;
    AVColorSpace m_colorSpace = AVCOL_SPC_UNSPECIFIED;
    int m_depth = 0;

    bool m_hasP016 = false;

    int m_width, m_height, m_codedHeight;
    CUvideotimestamp m_lastCuvidTS;
    QQueue<double> m_timestamps;
    double m_lastTS[2];

    cudaVideoDeinterlaceMode m_deintMethod;
    bool m_forceFlush;
    bool m_tsWorkaround;

    QQueue<CUVIDPARSERDISPINFO> m_cuvidSurfaces;

    AVBSFContext *m_bsfCtx;
    SwsContext *m_swsCtx;
    AVPacket *m_pkt;

    std::shared_ptr<CUcontext> m_cuCtx;

    CUVIDEOFORMATEX m_cuvidFmt;
    CUVIDPARSERPARAMS m_cuvidParserParams;
    CUvideoparser m_cuvidParser;
    CUvideodecoder m_cuvidDec;

    bool m_decodeMPEG4;
    bool m_hasCriticalError;
    bool m_skipFrames;
};

#define CuvidName "CUVID decoder"
