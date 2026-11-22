// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Lib.hpp>
#include <VideoWriter.hpp>

#include <QCoreApplication>
#include <QSet>

class OpenGLHWInterop;
class OpenGLCommon;

class OpenGLWriter final : public VideoWriter
{
    Q_DECLARE_TR_FUNCTIONS(OpenGLWriter)

public:
    OpenGLWriter();
    ~OpenGLWriter();

    bool set() override;

    bool readyWrite() const override;

    bool processParams(bool *paramsCorrected) override;

    AVPixelFormats supportedPixelFormats() const override;

    void writeVideo(const Frame &videoFrame, QMPlay2OSDList &&osdList) override;

    void pause() override;

    QString name() const override;

    bool open() override;

    bool setHWDecContext(const std::shared_ptr<HWDecContext> &hwDecContext) override;
    std::shared_ptr<HWDecContext> hwDecContext() const override;

private:
    void addAdditionalParam(const QString &key);

    void initialize(const std::shared_ptr<OpenGLHWInterop> &hwInterop);

private:
    OpenGLCommon *m_drawable = nullptr;
    bool m_useRtt = false;
    bool m_bypassCompositor = false;
    QSet<QString> m_additionalParams;
};

