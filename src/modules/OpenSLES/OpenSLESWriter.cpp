/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

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

#include <OpenSLESWriter.hpp>

#ifdef Q_OS_ANDROID
    #include <QAndroidJniEnvironment>
    static int getNativeOutputSampleRate()
    {
        enum AudioManager
        {
            STREAM_MUSIC = 3
        };
        QAndroidJniEnvironment jniEnv;
        jclass clazz = jniEnv->FindClass("android/media/AudioTrack");
        jmethodID method = jniEnv->GetStaticMethodID(clazz, "getNativeOutputSampleRate", "(I)I");
        return jniEnv->CallStaticIntMethod(clazz, method, STREAM_MUSIC);
    }
#endif

static inline qint16 toInt16(float sample)
{
    if (sample <= -1.0f)
        return -32767;
    else if (sample >= 1.0f)
        return 32767;
    return sample * 32767;
}

static void bqPlayerCallback(SLBufferQueueItf bq, QSemaphore *sem)
{
    Q_UNUSED(bq)
    sem->release();
}

/**/

OpenSLESWriter::OpenSLESWriter(Module &module) :
    engineObject(nullptr),
    engineEngine(nullptr),
    outputMixObject(nullptr),
    bqPlayerObject(nullptr),
    bqPlayerPlay(nullptr),
    bqPlayerBufferQueue(nullptr),
    sample_rate(0), channels(0)
{
    addParam("delay");
    addParam("chn");
    addParam("rate");

    SetModule(module);
}
OpenSLESWriter::~OpenSLESWriter()
{
    close();
}

bool OpenSLESWriter::set()
{
    return sets().getBool("WriterEnabled");
}

bool OpenSLESWriter::readyWrite() const
{
    return bqPlayerBufferQueue;
}

bool OpenSLESWriter::processParams(bool *paramsCorrected)
{
    int chn = getParam("chn").toInt();
    if (chn > 2)
    {
        if (!paramsCorrected)
            return false;
        else
        {
            modParam("chn", chn = 2);
            *paramsCorrected = true;
        }
    }

    int rate = getParam("rate").toInt();
#ifdef Q_OS_ANDROID
    const int nativeSampleRate = getNativeOutputSampleRate();
    if (rate != nativeSampleRate)
#else
    if (rate != 8000 && rate != 11025 && rate != 12000 && rate != 16000 && rate != 22050 && rate != 24000 && rate != 32000 && rate != 44100 && rate != 48000)
#endif
    {
        if (!paramsCorrected)
            return false;
        else
        {
#ifdef Q_OS_ANDROID
            modParam("rate", rate = nativeSampleRate);
#else
            if (!(44100 % rate))
                rate = 44100;
            else
                rate = 48000;
            modParam("rate", rate);
#endif
            *paramsCorrected = true;
        }
    }

    if (chn != channels || rate != sample_rate)
    {
        close();
        channels = chn;
        sample_rate = rate;
        buffers.resize(10);
        for (int i = 0; i < buffers.size(); ++i)
            buffers[i].resize(10 * channels * sample_rate / 1000);

        /* Engine, output mix */
        if (slCreateEngine(&engineObject, 0, nullptr, 0, nullptr, nullptr) != SL_RESULT_SUCCESS)
            return false;
        if ((*engineObject)->Realize(engineObject, false) != SL_RESULT_SUCCESS)
            return false;
        if ((*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine))
            return false;

        if ((*engineEngine)->CreateOutputMix(engineEngine, &outputMixObject, 0, nullptr, nullptr) != SL_RESULT_SUCCESS)
            return false;

        if ((*outputMixObject)->Realize(outputMixObject, false) != SL_RESULT_SUCCESS)
            return false;

        /* Buffer queue */
        SLDataLocator_BufferQueue loc_bufq = {SL_DATALOCATOR_BUFFERQUEUE, (SLuint32)buffers.size()};
        SLDataFormat_PCM format_pcm =
        {
            SL_DATAFORMAT_PCM, (SLuint32)channels, sample_rate * 1000U,
            SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
            channels == 2 ? (SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT) : SL_SPEAKER_FRONT_CENTER, SL_BYTEORDER_LITTLEENDIAN
        };
        SLDataSource audioSrc = {&loc_bufq, &format_pcm};

        SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
        SLDataSink audioSink = {&loc_outmix, nullptr};

        /* Audio player */
        const SLInterfaceID ids = SL_IID_BUFFERQUEUE;
        const SLboolean req = SL_BOOLEAN_TRUE;
        if ((*engineEngine)->CreateAudioPlayer(engineEngine, &bqPlayerObject, &audioSrc, &audioSink, 1, &ids, &req) != SL_RESULT_SUCCESS) //Get player object
            return false;

        if ((*bqPlayerObject)->Realize(bqPlayerObject, false) != SL_RESULT_SUCCESS)
            return false;
        if ((*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay) != SL_RESULT_SUCCESS) //Get player interface
            return false;
        if ((*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE, &bqPlayerBufferQueue)) //Get the buffer queue interface
            return false;

        if ((*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, (slBufferQueueCallback)bqPlayerCallback, &sem)) //Register callback on the buffer queue
            return false;

        /* Set the audio latency */
        modParam("delay", (buffers.at(0).size() / channels) * buffers.size() / (double)sample_rate);
    }
    else
    {
        (*bqPlayerBufferQueue)->Clear(bqPlayerBufferQueue);
        (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_STOPPED);
    }

    sem.release(buffers.size() - sem.available()); //Ensure that all buffers are available
    currBuffIdx = 0;
    paused = true;

    return readyWrite();
}
qint64 OpenSLESWriter::write(const QByteArray &arr)
{
    if (!readyWrite())
        return 0;
    if (paused)
    {
        (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
        paused = false;
    }

    const float *flt = (const float *)arr.constData();
    int numSamples = arr.size() / sizeof(float);

    const int BUFFER_SIZE = buffers.at(0).size();
    while (tmpBuffer.size() + numSamples >= BUFFER_SIZE)
    {
        /* Wait if no buffers, decrement buffers available */
        sem.acquire();
        /* Get raw pointer to current buffer */
        qint16 *currBuff = (qint16 *)buffers.at(currBuffIdx).constData(); //constData() for don't check if must do a deep-copy
        qint16 *dest = currBuff;
        /* Copy samples from tempBuffer (if available) */
        int samplesCopiedFromTmp = 0;
        if (!tmpBuffer.isEmpty())
        {
            samplesCopiedFromTmp = qMin(BUFFER_SIZE, tmpBuffer.size());
            memcpy(dest, tmpBuffer.constData(), samplesCopiedFromTmp * sizeof(qint16));
            tmpBuffer.remove(0, samplesCopiedFromTmp);
        }
        /* Copy samples from input array (if tempBuffer had less samples than BUFFER_SIZE) */
        if (samplesCopiedFromTmp < BUFFER_SIZE)
        {
            const int toCopy = BUFFER_SIZE - samplesCopiedFromTmp;
            dest += samplesCopiedFromTmp;
            for (int i = 0; i < toCopy; ++i)
                dest[i] = toInt16(flt[i]);
            flt += toCopy;
            numSamples -= toCopy;
        }
        /* Enqueue the buffer */
        (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, currBuff, BUFFER_SIZE * sizeof(qint16));
        /* Switch to next buffer */
        if (++currBuffIdx >= buffers.size())
            currBuffIdx = 0;
    }
    /* If there are any samples left in input buffer, then copy it to tempBuffer */
    if (numSamples > 0)
    {
        const int tmpBufferSamples = tmpBuffer.size();
        tmpBuffer.resize(tmpBufferSamples + numSamples);
        qint16 *dest = (qint16 *)tmpBuffer.constData() + tmpBufferSamples;
        for (int i = 0; i < numSamples; ++i)
            dest[i] = toInt16(flt[i]);
    }

    return arr.size();
}
void OpenSLESWriter::pause()
{
    if (bqPlayerPlay)
    {
        (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PAUSED);
        paused = true;
    }
}

QString OpenSLESWriter::name() const
{
    return OpenSLESWriterName;
}

bool OpenSLESWriter::open()
{
    return true;
}

/**/

void OpenSLESWriter::close()
{
    if (bqPlayerPlay)
        (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_STOPPED);
    if (bqPlayerObject)
    {
        (*bqPlayerObject)->Destroy(bqPlayerObject);
        bqPlayerObject = nullptr;
        bqPlayerPlay = nullptr;
        bqPlayerBufferQueue = nullptr;
    }
    if (outputMixObject)
    {
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = nullptr;
    }
    if (engineObject)
    {
        (*engineObject)->Destroy(engineObject);
        engineObject = nullptr;
        engineEngine = nullptr;
    }
    if (!buffers.isEmpty())
        buffers.clear();
    channels = sample_rate = 0;
}
