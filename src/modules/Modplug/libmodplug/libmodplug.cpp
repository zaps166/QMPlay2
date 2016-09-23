/*
 * This source code is public domain.
 *
 * Authors: Kenton Varda <temporal@gauge3d.org> (C interface wrapper)
 */

#include "libmodplug.hpp"
#include "stdafx.hpp"
#include "sndfile.hpp"

namespace QMPlay2ModPlug {

	struct File
	{
		CSoundFile mSoundFile;
	};

	Settings gSettings =
	{
		ENABLE_OVERSAMPLING | ENABLE_NOISE_REDUCTION,

		2, // mChannels
		16, // mBits
		44100, // mFrequency
		RESAMPLE_LINEAR, //mResamplingMode

		128, // mStereoSeparation
		32, // mMaxMixChannels
		0,
		0,
		0,
		0,
		0,
		0,
		0
	};

	int gSampleSize;

	static void UpdateSettings(bool updateBasicConfig)
	{
		if(gSettings.mFlags & ENABLE_REVERB)
		{
			CSoundFile::SetReverbParameters(gSettings.mReverbDepth,
			                                gSettings.mReverbDelay);
		}

		if(gSettings.mFlags & ENABLE_MEGABASS)
		{
			CSoundFile::SetXBassParameters(gSettings.mBassAmount,
			                               gSettings.mBassRange);
		}
		else // modplug seems to ignore the SetWaveConfigEx() setting for bass boost
			CSoundFile::SetXBassParameters(0, 0);

		if(gSettings.mFlags & ENABLE_SURROUND)
		{
			CSoundFile::SetSurroundParameters(gSettings.mSurroundDepth,
			                                  gSettings.mSurroundDelay);
		}

		if(updateBasicConfig)
		{
			CSoundFile::SetWaveConfig(gSettings.mFrequency,
                                      gSettings.mBits,
                                      gSettings.mChannels);
			CSoundFile::SetMixConfig(gSettings.mStereoSeparation,
                                     gSettings.mMaxMixChannels);

			gSampleSize = gSettings.mBits / 8 * gSettings.mChannels;
		}

		CSoundFile::SetWaveConfigEx(gSettings.mFlags & ENABLE_SURROUND,
                                    !(gSettings.mFlags & ENABLE_OVERSAMPLING),
                                    gSettings.mFlags & ENABLE_REVERB,
                                    true,
                                    gSettings.mFlags & ENABLE_MEGABASS,
                                    gSettings.mFlags & ENABLE_NOISE_REDUCTION,
                                    false);
		CSoundFile::SetResamplingMode(gSettings.mResamplingMode);
	}


File* Load(const void* data, int size)
{
	File* result = new File;
	UpdateSettings(true);
	if(result->mSoundFile.Create((const BYTE*)data, size))
	{
		result->mSoundFile.SetRepeatCount(gSettings.mLoopCount);
		return result;
	}
	else
	{
		delete result;
		return NULL;
	}
}

void Unload(File* file)
{
	file->mSoundFile.Destroy();
	delete file;
}

int Read(File* file, void* buffer, int size)
{
	return file->mSoundFile.Read(buffer, size) * gSampleSize;
}

const char* GetName(File* file)
{
	return file->mSoundFile.GetTitle();
}

int GetLength(File* file)
{
	return file->mSoundFile.GetSongTime() * 1000;
}

void InitMixerCallback(File* file,ModPlugMixerProc proc)
{
	file->mSoundFile.gpSndMixHook = (LPSNDMIXHOOKPROC)proc ;
	return;
}

void UnloadMixerCallback(File* file)
{
	file->mSoundFile.gpSndMixHook = NULL;
	return ;
}

unsigned int GetMasterVolume(File* file)
{
	return (unsigned int)file->mSoundFile.m_nMasterVolume;
}

void SetMasterVolume(File* file,unsigned int cvol)
{
	(void)file->mSoundFile.SetMasterVolume((UINT)cvol,
						FALSE);
	return ;
}

int GetCurrentSpeed(File* file)
{
	return file->mSoundFile.m_nMusicSpeed;
}

int GetCurrentTempo(File* file)
{
	return file->mSoundFile.m_nMusicTempo;
}

int GetCurrentOrder(File* file)
{
	return file->mSoundFile.GetCurrentOrder();
}

int GetCurrentPattern(File* file)
{
	return file->mSoundFile.GetCurrentPattern();
}

int GetCurrentRow(File* file)
{
	return file->mSoundFile.m_nRow;
}

int GetPlayingChannels(File* file)
{
	return (file->mSoundFile.m_nMixChannels < file->mSoundFile.m_nMaxMixChannels ? file->mSoundFile.m_nMixChannels : file->mSoundFile.m_nMaxMixChannels);
}

void SeekOrder(File* file,int order)
{
	file->mSoundFile.SetCurrentOrder(order);
}

int GetModuleType(File* file)
{
	return file->mSoundFile.m_nType;
}

char* GetMessage(File* file)
{
	return file->mSoundFile.m_lpszSongComments;
}

#ifndef MODPLUG_NO_FILESAVE
char ExportS3M(File* file,const char* filepath)
{
	return (char)file->mSoundFile.SaveS3M(filepath,0);
}

char ExportXM(File* file,const char* filepath)
{
	return (char)file->mSoundFile.SaveXM(filepath,0);
}

char ExportMOD(File* file,const char* filepath)
{
	return (char)file->mSoundFile.SaveMod(filepath,0);
}

char ExportIT(File* file,const char* filepath)
{
	return (char)file->mSoundFile.SaveIT(filepath,0);
}
#endif // MODPLUG_NO_FILESAVE

unsigned int NumInstruments(File* file)
{
	return file->mSoundFile.m_nInstruments;
}

unsigned int NumSamples(File* file)
{
	return file->mSoundFile.m_nSamples;
}

unsigned int NumPatterns(File* file)
{
	return file->mSoundFile.GetNumPatterns();
}

unsigned int NumChannels(File* file)
{
	return file->mSoundFile.GetNumChannels();
}

unsigned int SampleName(File* file,unsigned int qual,char* buff)
{
	return file->mSoundFile.GetSampleName(qual,buff);
}

unsigned int InstrumentName(File* file,unsigned int qual,char* buff)
{
	return file->mSoundFile.GetInstrumentName(qual,buff);
}

ModPlugNote* GetPattern(File* file,int pattern,unsigned int* numrows) {
	if (pattern<MAX_PATTERNS && pattern >= 0) {
		if (file->mSoundFile.Patterns[pattern]) {
			if (numrows) *numrows=(unsigned int)file->mSoundFile.PatternSize[pattern];
			return (ModPlugNote*)file->mSoundFile.Patterns[pattern];
		}
	}
	return NULL;
}

void Seek(File* file, int millisecond)
{
	int maxpos;
	int maxtime = file->mSoundFile.GetSongTime() * 1000;
	float postime;

	if(millisecond > maxtime)
		millisecond = maxtime;
	maxpos = file->mSoundFile.GetMaxPosition();
	postime = 0.0f;
	if (maxtime != 0.0f)
		postime = (float)maxpos / (float)maxtime;

	file->mSoundFile.SetCurrentPos((int)(millisecond * postime));
}

void GetSettings(Settings* settings)
{
	memcpy(settings, &gSettings, sizeof(Settings));
}

void SetSettings(const Settings* settings)
{
	memcpy(&gSettings, settings, sizeof(Settings));
	UpdateSettings(false); // do not update basic config.
}

} //namespace ModPlug
