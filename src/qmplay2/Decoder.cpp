/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2016  Błażej Szczygieł

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

#include <Decoder.hpp>

#include <QMPlay2Core.hpp>
#include <StreamInfo.hpp>
#include <Module.hpp>

class QMPlay2DummyDecoder : public Decoder
{
	QString name() const
	{
		return QString();
	}

	bool open(StreamInfo &, Writer *)
	{
		return true;
	}
};

Decoder *Decoder::create(StreamInfo &streamInfo, Writer *writer, const QStringList &modNames)
{
	if (!streamInfo.must_decode)
	{
		Decoder *decoder = new QMPlay2DummyDecoder;
		decoder->open(streamInfo);
		return decoder;
	}

	typedef QVector<QPair<Module *, Module::Info> > pluginsList_t;
	pluginsList_t pluginsInstances(modNames.count());
	foreach (Module *pluginInstance, QMPlay2Core.getPluginsInstance())
		foreach (const Module::Info &mod, pluginInstance->getModulesInfo())
			if (mod.type == Module::DECODER)
			{
				if (modNames.isEmpty())
					pluginsInstances += qMakePair(pluginInstance, mod);
				else
				{
					const int idx = modNames.indexOf(mod.name);
					if (idx > -1)
						pluginsInstances[idx] = qMakePair(pluginInstance, mod);
				}
			}
	for(pluginsList_t::const_iterator it = pluginsInstances.constBegin(), end = pluginsInstances.constEnd();
		it != end; ++it)
	{
		Module *module = it->first;
		const Module::Info &moduleInfo = it->second;
		if (!module || moduleInfo.name.isEmpty())
			continue;
		Decoder *decoder = (Decoder *)module->createInstance(moduleInfo.name);
		if (!decoder)
			continue;
		if (decoder->open(streamInfo, writer))
			return decoder;
		delete decoder;
	}
	return NULL;
}

Writer *Decoder::HWAccel() const
{
	return NULL;
}

void Decoder::setSupportedPixelFormats(const QMPlay2PixelFormats &pixelFormats)
{
	Q_UNUSED(pixelFormats)
}

int Decoder::decodeVideo(Packet &encodedPacket, VideoFrame &decoded, QByteArray &newPixFmt, bool flush, unsigned hurry_up)
{
	Q_UNUSED(encodedPacket)
	Q_UNUSED(decoded)
	Q_UNUSED(newPixFmt)
	Q_UNUSED(flush)
	Q_UNUSED(hurry_up)
	return 0;
}
int Decoder::decodeAudio(Packet &encodedPacket, Buffer &decoded, quint8 &channels, quint32 &sampleRate, bool flush)
{
	Q_UNUSED(channels)
	Q_UNUSED(sampleRate)
	Q_UNUSED(flush)
	return (decoded = encodedPacket).size();
}
bool Decoder::decodeSubtitle(const Packet &encodedPacket, double pos, QMPlay2_OSD *&osd, int w, int h)
{
	Q_UNUSED(encodedPacket)
	Q_UNUSED(pos)
	Q_UNUSED(osd)
	Q_UNUSED(w)
	Q_UNUSED(h)
	return false;
}

Decoder::~Decoder()
{}
