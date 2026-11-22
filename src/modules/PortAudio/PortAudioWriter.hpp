// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <Writer.hpp>

#include <PortAudioCommon.hpp>

#include <portaudio.h>
#ifdef Q_OS_WIN
#   include <mmdeviceapi.h>
#   include <pa_win_wasapi.h>

#   include <QMutex>
#endif

#include <QCoreApplication>

#ifdef Q_OS_WIN
class PortAudioWriter;

class WASAPINotifications final : public IMMNotificationClient
{
public:
    WASAPINotifications(PortAudioWriter *writer);
    ~WASAPINotifications();;

private:
    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState) override;
    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId) override;
    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId) override;
    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId) override;
    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key) override;

private:
    HRESULT STDMETHODCALLTYPE QueryInterface(const IID &iid, void **ppUnk) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;

private:
    PortAudioWriter *const m_writer;
    IMMDeviceEnumerator *m_deviceEnumerator = nullptr;
};
#endif

#ifdef Q_OS_MACOS
class AudioDevice;
#endif

class PortAudioWriter final : public Writer
{
    Q_DECLARE_TR_FUNCTIONS(PortAudioWriter)

public:
    PortAudioWriter(Module &);
private:
    ~PortAudioWriter();

    bool set() override;

    bool readyWrite() const override;

    bool processParams(bool *paramsCorrected) override;
    qint64 write(const QByteArray &) override;
    void pause() override;

    QString name() const override;

    bool open() override;

private:
    bool deviceNeedsChangeParams(int *newChn = nullptr, int *newRate = nullptr);

    bool openStream();
    bool startStream();
    bool writeStream(const QByteArray &arr);
    void playbackError();

#ifdef Q_OS_WIN
    bool isDeviceInvalidated() const;
    bool reopenStream();
#endif

    void drain();

    void close();

#ifdef Q_OS_WIN
public:
    void wasapiDefaultDeviceId(const QString &id);
#endif

private:
    QString m_outputDevice;
    PaStreamParameters m_outputParameters = {};
    PaStream *m_stream = nullptr;
    int m_sampleRate = 0;
    double m_outputLatency = 0.0;
    bool m_initialized = false;
    bool m_err = false;
    bool m_streamOpen = false;
    bool m_dontShowError = false;
#ifdef Q_OS_WIN
    bool m_exclusive = false;
    PaWasapiStreamInfo m_wasapiStreamInfo = {};
    WASAPINotifications *m_wasapiNotifications = nullptr;
    QMutex m_defaultDeviceIdMutex;
    QString m_paDefaultDeviceId;
    QString m_defaultDeviceId;
#endif
#ifdef Q_OS_MACOS
    bool m_bitPerfect = false;
    AudioDevice *m_coreAudioDevice = nullptr;
#endif
};

#define PortAudioWriterName "PortAudio"
