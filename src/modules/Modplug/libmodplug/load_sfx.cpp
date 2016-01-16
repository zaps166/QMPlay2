#include "stdafx.h"
#include "sndfile.h"

#include <ByteArray.hpp>

BOOL CSoundFile::ReadSFX(const BYTE *lpStream, DWORD dwMemLength)
{
	ByteArray data(lpStream, dwMemLength, true);
	m_nSamples = 15;

	data = m_nSamples * sizeof(DWORD);
	if (FourCC("SONG", true) != data.getDWORD())
	{
		m_nSamples = 31;
		data = m_nSamples * sizeof(DWORD);
		if (FourCC("SONG", true) != data.getDWORD())
			return false;
	}
	data = 0;

	DWORD sample_size[ m_nSamples ];
	for (BYTE i = 0; i < m_nSamples; ++i)
		sample_size[ i ] = data.getDWORD();

	data += 4; //"SONG"

	m_nDefaultTempo = 14565 * 122 / data.getWORD();
	data += 0xE;

	for (BYTE i = 1; i <= m_nSamples; ++i)
	{
		MODINSTRUMENT &sample = Ins[ i ];

		memcpy(m_szNames[ i ], data, 22);
		data += 22;

		sample.nGlobalVol = 64;
		sample.nLength = 2 * data.getWORD();
		sample.nFineTune = MOD2XMFineTune(data.getBYTE()); //is it OK?
		sample.nVolume = data.getBYTE() * 4;
		sample.nLoopStart = data.getWORD();
		DWORD loop_length = data.getWORD();
		sample.nLoopEnd = sample.nLoopStart + 2 * loop_length;
		if (loop_length > 1)
			sample.uFlags |= CHN_LOOP;
	}

	BYTE len = data.getBYTE();
	if (len > 0x7F || data.remaining() < len)
		return false;

	++data; //?

	memcpy(Order, data, len);
	data += 0x80;

	m_nPattern = 0;
	for (BYTE i = 0; i < len; ++i)
		if (Order[ i ] > m_nPattern)
			m_nPattern = Order[ i ];
	++m_nPattern;

	m_dwSongFlags |= SONG_AMIGALIMITS;
	m_nType = MOD_TYPE_SFX;
	m_nDefaultSpeed = 6;
	m_nChannels = 4;
	ChnSettings[ 0 ].nPan = ChnSettings[ 3 ].nPan = 0x00;
	ChnSettings[ 1 ].nPan = ChnSettings[ 2 ].nPan = 0xFF;

	const BYTE rows = 64;
	for (BYTE i = 0; i < m_nPattern; ++i) //Reading patterns
	{
		PatternSize[ i ] = rows;
		MODCOMMAND *&pattern = Patterns[ i ];
		pattern = AllocatePattern(rows, m_nChannels);
		for (WORD j = 0; j < rows * m_nChannels; ++j)
		{
			if (data.remaining() < 4)
				break;
			const BYTE channel = j % m_nChannels;
			const BYTE row = j / m_nChannels;
			MODCOMMAND &m = pattern[ m_nChannels*row+channel ];
			BYTE entry[ 4 ];
			memcpy(entry, data, 4);
			data += 4;

			m.note  = GetNoteFromPeriod(((entry[ 0 ] & 0x0F) << 8 | entry[ 1 ]) << 2);
			m.instr = ((entry[ 0 ] & 0xF0) | (entry[ 2 ] & 0xF0) >> 4);
			m.param = entry[ 3 ];
			switch (entry[ 2 ] & 0x0F) //is it OK?
			{
				case 0x1:
					m.command = CMD_ARPEGGIO;
					break;
				case 0x2:
					if (m.param & 0xF0)
					{
						m.command = CMD_PORTAMENTODOWN;
						m.param >>= 4;
					}
					else if (m.param & 0x0F)
					{
						m.command = CMD_PORTAMENTOUP;
						m.param &= 0x0F;
					}
					break;
				case 0x5:
					m.command = CMD_VOLUMESLIDE;
					break;
				case 0x06:
					m.command = CMD_VOLUME;
					m.param = 0x40 - m.param;
					break;
			}
		}
	}

	for (BYTE i = 0; i < m_nSamples; ++i) //Reading samples data
	{
		if (data.atEnd())
			break;
		DWORD size = sample_size[ i ];
		if (size)
		{
			if (data.remaining() < size)
				size = data.remaining();
			ReadSample(&Ins[ i+1 ], RS_PCM8S, data, size);
			data += size;
		}
	}

	return true;
}
