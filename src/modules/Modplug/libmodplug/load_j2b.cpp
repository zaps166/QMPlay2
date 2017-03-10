/*
 * This source code is public domain.
 *
 * Authors: Olivier Lapicque <olivierl@jps.net>
 *          Błażej Szczygieł <spaz16@wp.pl> (All below code)
*/


///////////////////////////////////////////////////
//
// J2B module loader (AM and AMFF)
//
///////////////////////////////////////////////////
#include "stdafx.hpp"
#include "sndfile.hpp"

/* ZLIB */
#include <zlib.h>

namespace QMPlay2ModPlug {

static bool Inflate(const BYTE *data, DWORD size, BYTE *&out_arr, const DWORD out_size)
{
	const DWORD CHUNK = 16384;
	DWORD bytes_done = 0;
	Bytef out[CHUNK];
	int err;

	out_arr = NULL;

	/* Obsługa biblioteki zlib do dekompresji danych */

	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	if (!out_size || inflateInit(&strm) != Z_OK)
		return false;

	do
	{
		strm.avail_in = size > CHUNK ? CHUNK : size;
		strm.next_in = (Bytef *)data;
		data += strm.avail_in;
		size -= strm.avail_in;

		do
		{
			strm.avail_out = CHUNK;
			strm.next_out = out;

			err = inflate(&strm, Z_NO_FLUSH);
			if (err != Z_STREAM_END && err != Z_OK) //Jakikolwiek błąd
				break;

			DWORD to_append = CHUNK-strm.avail_out;
			if (to_append + bytes_done > out_size)
			{
				err = Z_BUF_ERROR;
				break;
			}
			BYTE *out_arr_2 = (BYTE *)realloc(out_arr, to_append + bytes_done);
			if (!out_arr_2)
			{
				err = Z_MEM_ERROR;
				break;
			}
			out_arr = out_arr_2;
			memcpy(out_arr + bytes_done, out, to_append);
			bytes_done += to_append;
		} while (!strm.avail_out);
	} while (err == Z_OK);

	inflateEnd(&strm);

	if ((err != Z_STREAM_END && err != Z_OK) || out_size != bytes_done)
	{
		free(out_arr);
		out_arr = NULL;
		return false;
	}

	return true;
}
/**/

#include <ByteArray.hpp>

struct CHUNK
{
	DWORD type;
	DWORD size;
	const BYTE *data;
};
struct RIFF
{
	DWORD type;
	DWORD chunk_count;
	CHUNK *chunk_arr;
};

class RIFFList
{
public:
	RIFFList(const BYTE *lpStream, const DWORD dwMemLength, const bool skip1B) :
		riff_arr(NULL), riff_count(0)
	{
		ByteArray data(lpStream, dwMemLength);
		while (!data.atEnd())
		{
			if (data.getDWORD() != FourCC("RIFF"))
				break;

			DWORD size = data.getDWORD() - 12;

			riff_arr = (RIFF *)realloc(riff_arr, ++riff_count * sizeof(RIFF));

			RIFF &riff = riff_arr[riff_count-1];
			riff.type = data.getDWORD();
			riff.chunk_count = 0;
			riff.chunk_arr = NULL;

			while (!data.atEnd())
			{
				CHUNK chunk;
				chunk.type = data.getDWORD();
				if (chunk.type == FourCC("RIFF"))
				{
					data -= 4;
					break;
				}
				chunk.size = data.getDWORD();
				if (!chunk.size)
					break;
				if (size < chunk.size)
					continue;
				size -= chunk.size;
				chunk.data = data;

				data += chunk.size;
				if (skip1B && (chunk.size & 1))
					++data;

				riff.chunk_arr = (CHUNK *)realloc(riff.chunk_arr, ++riff.chunk_count * sizeof(CHUNK));
				riff.chunk_arr[riff.chunk_count-1] = chunk;
			}
		}
	}
	~RIFFList()
	{
		for (DWORD i = 0; i < riff_count; ++i)
			free(riff_arr[i].chunk_arr);
		free(riff_arr);
	}

	DWORD count() const
	{
		return riff_count;
	}
	RIFF &operator [](DWORD idx)
	{
		return riff_arr[idx];
	}
private:
	RIFF *riff_arr;
	DWORD riff_count;
};

static inline BYTE ClipBYTE(const BYTE &src, const BYTE &max)
{
	return src < max ? src : max;
}

static void LoadSample(CSoundFile &c, const BYTE *const chunk_data, const DWORD chunk_size, const bool isAM)
{
	ByteArray data(chunk_data, chunk_size);
	DWORD header_length = isAM ? data.getDWORD() : 56;
	if (data.remaining() < header_length)
		return;
	MODINSTRUMENT &sample = c.Ins[c.m_nSamples];

	if (isAM)
	{
		memcpy(c.m_szNames[c.m_nSamples], data, 31);
		data += 32;
	}
	else
	{
		memcpy(c.m_szNames[c.m_nSamples], data, 28);
		data += 28;
	}

	sample.nPan = isAM ? (data.getWORD() * 64 / 8191) : data.getBYTE() * 4;
	sample.nVolume = isAM ? (data.getWORD() * 64 / 8191) : data.getBYTE() * 4;
	DWORD flags = isAM ? data.getDWORD() : data.getWORD();
	sample.nLength = data.getDWORD();
	sample.nLoopStart = data.getDWORD();
	sample.nLoopEnd = data.getDWORD();
	sample.nC4Speed = data.getDWORD(); //Samplerate
	sample.nGlobalVol = 64;

	if ((flags & 0x08) && (sample.nLoopEnd <= sample.nLength && sample.nLoopEnd > sample.nLoopStart))
	{
		sample.uFlags |= CHN_LOOP;
		if (flags & 0x10)
			sample.uFlags |= CHN_PINGPONGLOOP;
	}
	if (flags & 0x20)
		sample.uFlags |= CHN_PANNING;

	data = isAM ? (header_length + 4) : header_length;
	c.ReadSample(&sample, (flags & RSF_16BIT) ? RS_PCM16S : RS_PCM8S, data, data.remaining());
}
static void LoadInstrument(CSoundFile &c, const BYTE inst_idx, const BYTE *const chunk_data, const DWORD chunk_size, const bool isAM)
{
	INSTRUMENTHEADER *&instrument = c.Headers[inst_idx];

	instrument = new INSTRUMENTHEADER;
	memset(instrument, 0, sizeof(INSTRUMENTHEADER));

	instrument->nGlobalVol = 64;
	instrument->nPan = 32 << 2; //*4

	ByteArray data(chunk_data, chunk_size);

	memcpy(instrument->name, data, isAM ? 31 : 28);
	data += isAM ? 32 : 29;

	for (BYTE i = 0; i < NOTE_MAX; i++)
	{
		WORD sample_map = c.m_nSamples + data.getBYTE();
		instrument->Keyboard[i] = sample_map >= MAX_SAMPLES ? 0 : sample_map;
		instrument->NoteMap[i] = i + 1;
	}

	if (isAM)
		data += 0x8E - NOTE_MAX;
	else
		data += 0x07;

	constexpr BYTE J2B_MAX_ENVPOINTS = 10;

	if (isAM)
	{
		enum { J2B_AM_ENV_VOL, J2B_AM_ENV_PITCH, J2B_AM_ENV_PAN };
		for (BYTE t = J2B_AM_ENV_VOL; t <= J2B_AM_ENV_PAN; ++t)
		{
			WORD *Points = NULL;
			BYTE *Env = NULL;
			switch (t)
			{
				case J2B_AM_ENV_VOL:
					instrument->dwFlags |= data.getWORD();
					instrument->nVolEnv = ClipBYTE(data.getBYTE() + 1, J2B_MAX_ENVPOINTS);
					instrument->nVolSustainBegin = instrument->nVolSustainEnd = data.getBYTE();
					instrument->nVolLoopStart = data.getBYTE();
					instrument->nVolLoopEnd = data.getBYTE();
					Points = instrument->VolPoints;
					Env = instrument->VolEnv;
					break;
				case J2B_AM_ENV_PITCH:
					instrument->dwFlags |= data.getWORD() << 6;
					instrument->nPitchEnv = ClipBYTE(data.getBYTE() + 1, J2B_MAX_ENVPOINTS);
					instrument->nPitchSustainBegin = instrument->nPitchSustainEnd = data.getBYTE();
					instrument->nPitchLoopStart = data.getBYTE();
					instrument->nPitchLoopEnd = data.getBYTE();
					Points = instrument->PitchPoints;
					Env = instrument->PitchEnv;
					break;
				case J2B_AM_ENV_PAN:
					instrument->dwFlags |= data.getWORD() << 3;
					instrument->nPanEnv = ClipBYTE(data.getBYTE() + 1, J2B_MAX_ENVPOINTS);
					instrument->nPanSustainBegin = instrument->nPanSustainEnd = data.getBYTE();
					instrument->nPanLoopStart = data.getBYTE();
					instrument->nPanLoopEnd = data.getBYTE();
					Points = instrument->PanPoints;
					Env = instrument->PanEnv;
					break;
			}
			for (BYTE i = 0; i < J2B_MAX_ENVPOINTS; ++i)
			{
				Points[i] = data.getWORD() >> 4; //X, 16bit
				const WORD Y = data.getWORD(); //Y, 8bit
				switch (t)
				{
					case J2B_AM_ENV_VOL:
						Env[i] = Y / 511;
						break;
					case J2B_AM_ENV_PITCH:
						if (Y <= 0xFFF) //Dodatnie
							Env[i] = Y / 127 + 0x20;
						else if (Y >= 0x6FFF && Y <= 0x7FFE) //Ujemne
							Env[i] = (Y - 0x6FFF) / 128;
						else if (Y >= 0x0FFF && Y <= 0x6FFE)
							Env[i] = 0x40;
						else if (Y >= 0x7FFF)
							Env[i] = 0x20;
						break;
					case J2B_AM_ENV_PAN:
						Env[i] = (Y ^ 0x8000) / 1023; //zamiana signed na unsigned (prawie tak jak w 8bit PCM)
						break;
				}
			}
			switch (t)
			{
				case J2B_AM_ENV_VOL:
					instrument->nFadeOut = data.getWORD() << 5; //*32
					break;
				default:
					data += 2;
			}
		}
	}
	else
	{
		BYTE flags = data.getBYTE() & 0x77;
		BYTE points = data.getBYTE();
		BYTE sustain = data.getBYTE();
		BYTE loop_start = data.getBYTE();
		BYTE loop_end = data.getBYTE();

		instrument->dwFlags = (flags & 0x07) | ((flags & 0x70) >> 1);

		instrument->nVolEnv = ClipBYTE(points & 0x0F, J2B_MAX_ENVPOINTS);
		instrument->nVolSustainBegin = instrument->nVolSustainEnd = sustain & 0x0F;
		instrument->nVolLoopStart = loop_start & 0x0F;
		instrument->nVolLoopEnd = loop_end & 0x0F;

		instrument->nPanEnv = ClipBYTE(points >> 4, J2B_MAX_ENVPOINTS);
		instrument->nPanSustainBegin = instrument->nPanSustainEnd = sustain >> 4;
		instrument->nPanLoopStart = loop_start >> 4;
		instrument->nPanLoopEnd = loop_end >> 4;

		enum { J2B_AMFF_ENV_VOL, J2B_AMFF_ENV_PAN };
		for (BYTE t = J2B_AMFF_ENV_VOL; t <= J2B_AMFF_ENV_PAN; ++t)
		{
			for (BYTE i = 0; i < J2B_MAX_ENVPOINTS; ++i)
			{
				const WORD X = data.getWORD() >> 4;
				const BYTE Y = ClipBYTE(data.getBYTE(), 0x40);
				switch (t)
				{
					case J2B_AMFF_ENV_VOL:
						instrument->VolPoints[i] = X;
						instrument->VolEnv[i] = Y;
						break;
					case J2B_AMFF_ENV_PAN:
						instrument->PanPoints[i] = X;
						instrument->PanEnv[i] = Y;
						break;
				}
			}
		}

		instrument->nFadeOut = data.getWORD() << 5; //*32
	}
}

BOOL CSoundFile::ReadJ2B(const BYTE *lpStream, DWORD dwMemLength)
{
	if (dwMemLength <= 24)
		return false;

	bool isAM = false;

	DWORD uncompressed_size = 0;
	BYTE *uncompressed_arr = NULL;

	if (!strncmp((const char *)lpStream, "MUSE", 4))
	{
		ByteArray data(lpStream, dwMemLength);
		data += 20;
		uncompressed_size = data.getDWORD();
		if (uncompressed_size < 12)
			return false;
		if (!Inflate(data, data.remaining(), uncompressed_arr, uncompressed_size))
			return false;
		isAM = !strncmp((const char *)uncompressed_arr + 8, "AM  ", 4);
		if (!isAM && strncmp((const char *)uncompressed_arr + 8, "AMFF", 4))
		{
			delete[] uncompressed_arr;
			return false;
		}
	}
	else if (!strncmp((const char *)lpStream, "RIFF", 4))
	{
		isAM = !strncmp((const char *)lpStream + 8, "AM  ", 4);
		if (!isAM && strncmp((const char *)lpStream + 8, "AMFF", 4))
			return false;
	}
	else
		return false;

	RIFFList riff_list(uncompressed_arr ? uncompressed_arr : lpStream, uncompressed_arr ? uncompressed_size : dwMemLength, isAM);
	for (DWORD i = 0; i < riff_list.count(); ++i)
	{
		for (DWORD j = 0; j < riff_list[i].chunk_count; ++j)
		{
			CHUNK &chunk = riff_list[i].chunk_arr[j];
			ByteArray data(chunk.data, chunk.size);

			if ((isAM && chunk.type == FourCC("INIT")) || (!isAM && chunk.type == FourCC("MAIN")))
			{
				memcpy(m_szNames[0], data, 31); //Name
				data += 64; //Name length in J2B

				BYTE flags = data.getBYTE();

				m_dwSongFlags |= SONG_ITCOMPATMODE | SONG_ITOLDEFFECTS;
				if (!(flags & 1))
					m_dwSongFlags |= SONG_LINEARSLIDES;

				m_nType = MOD_TYPE_J2B;
				m_nChannels = data.getBYTE();
				if (m_nChannels > MAX_CHANNELS)
					m_nChannels = MAX_CHANNELS;
				m_nDefaultSpeed = data.getBYTE();
				m_nDefaultTempo = data.getBYTE();
				data += 4;
				m_nDefaultGlobalVolume = data.getBYTE() * 2;
				for (DWORD i = 0; i < m_nChannels; ++i)
				{
					BYTE c = data.getBYTE();
					if (isAM)
					{
						if (c <= 0x80)
							ChnSettings[i].nPan = c * 2;
						else
							ChnSettings[i].dwFlags |= CHN_MUTE;
					}
					else
					{
						if (c >= 0x80)
							ChnSettings[i].dwFlags |= CHN_MUTE;
						else
							ChnSettings[i].nPan = c * 4;
					}
				}
			}
			else if (chunk.type == FourCC("ORDR"))
			{
				BYTE order_count = data.getBYTE() + 1;
				if (order_count == data.remaining())
					memcpy(Order, data, order_count);
			}
			else if (chunk.type == FourCC("PATT"))
			{
				BYTE pattern_idx = data.getBYTE();
				DWORD size = data.getDWORD();
				if (data.remaining() >= size) //LoadPattern
				{
					if (pattern_idx >= m_nPattern)
						m_nPattern = pattern_idx + 1;

					DWORD rows = data.getBYTE() + 1;
					PatternSize[pattern_idx] = rows;
					MODCOMMAND *&pattern = Patterns[pattern_idx];
					pattern = AllocatePattern(rows, m_nChannels);

					for (DWORD row = 0; row < rows && !data.atEnd();)
					{
						BYTE channel = *data & 0x1F;
						BYTE command = (*data & 0xE0) >> 4;
						++data;

						if (!command || m_nChannels < channel)
						{
							++row;
							continue;
						}

						MODCOMMAND &m = pattern[m_nChannels*row+channel];
						if (command & 0x8)
						{
							m.param   = data.getBYTE(); //Effect parameter
							m.command = data.getBYTE(); //Effect
							switch (m.command)
							{
								case 0x0F:
									m.command = (m.param <= 0x1F) ? CMD_SPEED : CMD_TEMPO;
									break;
								default:
									ConvertModCommand(&m);
							}
						}
						if (command & 0x4)
						{
							m.instr = data.getBYTE();
							m.note = data.getBYTE();
							if (m.note == 0x80)
								m.note = 0xFF;
							else if (m.note > 0x80)
								m.note = 0xFD;
						}
						if (command & 0x2)
						{
							m.volcmd = VOLCMD_VOLUME;
							m.vol = isAM ? (data.getBYTE() * 64 / 127) : data.getBYTE();
						}
					}
				}
			}
			else if (chunk.type == FourCC("INST"))
			{
				if (isAM)
				{
					DWORD size = data.getDWORD();
					if (size == 322)
					{
						RIFFList riff_samp(data+size, data.remaining()-size, isAM);
						++data;
						BYTE inst_idx = data.getBYTE() + 1;
						if (inst_idx < MAX_INSTRUMENTS && riff_samp.count() == 1 && riff_samp[0].chunk_count == 1)
						{
							CHUNK &chunk = riff_samp[0].chunk_arr[0];
							if (chunk.type == FourCC("SAMP"))
							{
								if (inst_idx > m_nInstruments)
									m_nInstruments = inst_idx;
								++m_nSamples;
								LoadInstrument(*this, inst_idx, data, size, isAM);
								LoadSample(*this, chunk.data, chunk.size, isAM);
							}
						}
					}
				}
				else
				{
					constexpr DWORD instr_header = 223;
					++data;
					BYTE inst_idx = data.getBYTE() + 1;
					data += instr_header;
					if (inst_idx < MAX_INSTRUMENTS && FourCC("SAMP") == data.getDWORD())
					{
						DWORD size = data.getDWORD();
						if (size == data.remaining())
						{
							if (inst_idx > m_nInstruments)
								m_nInstruments = inst_idx;
							++m_nSamples;
							LoadInstrument(*this, inst_idx, data-(instr_header+8), instr_header, isAM);
							LoadSample(*this, data, size, isAM);
						}
					}
				}
			}
		}
	}

	delete[] uncompressed_arr;
	return m_nPattern && m_nChannels && m_nInstruments && m_nSamples;
}

} //namespace
