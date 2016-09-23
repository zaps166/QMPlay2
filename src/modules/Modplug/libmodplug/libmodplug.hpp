/*
 * This source code is public domain.
 *
 * Authors: Kenton Varda <temporal@gauge3d.org> (C interface wrapper)
 */

#ifndef MODPLUG_H__INCLUDED
#define MODPLUG_H__INCLUDED

namespace QMPlay2ModPlug {

struct File;

struct ModPlugNote {
	unsigned char Note;
	unsigned char Instrument;
	unsigned char VolumeEffect;
	unsigned char Effect;
	unsigned char Volume;
	unsigned char Parameter;
};

typedef void (*ModPlugMixerProc)(int*, unsigned long, unsigned long);

/* Load a mod file.  [data] should point to a block of memory containing the complete
 * file, and [size] should be the size of that block.
 * Return the loaded mod file on success, or NULL on failure. */
File* Load(const void* data, int size);
/* Unload a mod file. */
void Unload(File* file);

/* Read sample data into the buffer.  Returns the number of bytes read.  If the end
 * of the mod has been reached, zero is returned. */
int  Read(File* file, void* buffer, int size);

/* Get the name of the mod.  The returned buffer is stored within the File
 * structure and will remain valid until you unload the file. */
const char* GetName(File* file);

/* Get the length of the mod, in milliseconds.  Note that this result is not always
 * accurate, especially in the case of mods with loops. */
int GetLength(File* file);

/* Seek to a particular position in the song.  Note that seeking and MODs don't mix very
 * well.  Some mods will be missing instruments for a short time after a seek, as ModPlug
 * does not scan the sequence backwards to find out which instruments were supposed to be
 * playing at that time.  (Doing so would be difficult and not very reliable.)  Also,
 * note that seeking is not very exact in some mods -- especially those for which
 * GetLength() does not report the full length. */
void Seek(File* file, int millisecond);

enum Flags
{
    ENABLE_OVERSAMPLING     = 1 << 0,  /* Enable oversampling (*highly* recommended) */
    ENABLE_NOISE_REDUCTION  = 1 << 1,  /* Enable noise reduction */
    ENABLE_REVERB           = 1 << 2,  /* Enable reverb */
    ENABLE_MEGABASS         = 1 << 3,  /* Enable megabass */
    ENABLE_SURROUND         = 1 << 4   /* Enable surround sound. */
};

enum ResamplingMode
{
    RESAMPLE_NEAREST = 0,  /* No interpolation (very fast, extremely bad sound quality) */
    RESAMPLE_LINEAR  = 1,  /* Linear interpolation (fast, good quality) */
    RESAMPLE_SPLINE  = 2,  /* Cubic spline interpolation (high quality) */
    RESAMPLE_FIR     = 3   /* 8-tap fir filter (extremely high quality) */
};

struct Settings
{
	int mFlags;  /* One or more of the MODPLUG_ENABLE_* flags above, bitwise-OR'ed */
	
	/* Note that ModPlug always decodes sound at 44100kHz, 32 bit, stereo and then
	 * down-mixes to the settings you choose. */
	int mChannels;       /* Number of channels - 1 for mono or 2 for stereo */
	int mBits;           /* Bits per sample - 8, 16, or 32 */
	int mFrequency;      /* Sampling rate - 11025, 22050, or 44100 */
	int mResamplingMode; /* One of MODPLUG_RESAMPLE_*, above */

	int mStereoSeparation; /* Stereo separation, 1 - 256 */
	int mMaxMixChannels; /* Maximum number of mixing channels (polyphony), 32 - 256 */
	
	int mReverbDepth;    /* Reverb level 0(quiet)-100(loud)      */
	int mReverbDelay;    /* Reverb delay in ms, usually 40-200ms */
	int mBassAmount;     /* XBass level 0(quiet)-100(loud)       */
	int mBassRange;      /* XBass cutoff in Hz 10-100            */
	int mSurroundDepth;  /* Surround level 0(quiet)-100(heavy)   */
	int mSurroundDelay;  /* Surround delay in ms, usually 5-40ms */
	int mLoopCount;      /* Number of times to loop.  Zero prevents looping.
	                        -1 loops forever. */
};

/* Get and set the mod decoder settings.  All options, except for channels, bits-per-sample,
 * sampling rate, and loop count, will take effect immediately.  Those options which don't
 * take effect immediately will take effect the next time you load a mod. */
void GetSettings(Settings* settings);
void SetSettings(const Settings* settings);

/* New ModPlug API Functions */
/* NOTE: Master Volume (1-512) */
unsigned int GetMasterVolume(File* file) ;
void SetMasterVolume(File* file,unsigned int cvol) ;

int GetCurrentSpeed(File* file);
int GetCurrentTempo(File* file);
int GetCurrentOrder(File* file);
int GetCurrentPattern(File* file);
int GetCurrentRow(File* file);
int GetPlayingChannels(File* file);

void SeekOrder(File* file,int order);
int GetModuleType(File* file);
char* GetMessage(File* file);


#ifndef MODPLUG_NO_FILESAVE
/*
 * EXPERIMENTAL Export Functions
 */
/*Export to a Scream Tracker 3 S3M module. EXPERIMENTAL (only works on Little-Endian platforms)*/
char ExportS3M(File* file, const char* filepath);

/*Export to a Extended Module (XM). EXPERIMENTAL (only works on Little-Endian platforms)*/
char ExportXM(File* file, const char* filepath);

/*Export to a Amiga MOD file. EXPERIMENTAL.*/
char ExportMOD(File* file, const char* filepath);

/*Export to a Impulse Tracker IT file. Should work OK in Little-Endian & Big-Endian platforms :-) */
char ExportIT(File* file, const char* filepath);
#endif // MODPLUG_NO_FILESAVE

unsigned int NumInstruments(File* file);
unsigned int NumSamples(File* file);
unsigned int NumPatterns(File* file);
unsigned int NumChannels(File* file);
unsigned int SampleName(File* file, unsigned int qual, char* buff);
unsigned int InstrumentName(File* file, unsigned int qual, char* buff);

/*
 * Retrieve pattern note-data
 */
ModPlugNote* GetPattern(File* file, int pattern, unsigned int* numrows);

/*
 * =================
 * Mixer callback
 * =================
 *
 * Use this callback if you want to 'modify' the mixed data of LibModPlug.
 * 
 * void proc(int* buffer,unsigned long channels,unsigned long nsamples) ;
 *
 * 'buffer': A buffer of mixed samples
 * 'channels': N. of channels in the buffer
 * 'nsamples': N. of samples in the buffeer (without taking care of n.channels)
 *
 * (Samples are signed 32-bit integers)
 */
void InitMixerCallback(File* file,ModPlugMixerProc proc) ;
void UnloadMixerCallback(File* file) ;

}

#endif
