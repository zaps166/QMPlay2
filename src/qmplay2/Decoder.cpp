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
	QVector<QPair<Module *, Module::Info> > pluginsInstances(modNames.count());
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
	for (int i = 0; i < pluginsInstances.count(); i++)
	{
		Module *module = pluginsInstances[i].first;
		Module::Info &moduleInfo = pluginsInstances[i].second;
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

int Decoder::decodeVideo(Packet &encodedPacket, VideoFrame &decoded, bool flush, unsigned hurry_up)
{
	Q_UNUSED(encodedPacket)
	Q_UNUSED(decoded)
	Q_UNUSED(flush)
	Q_UNUSED(hurry_up)
	return 0;
}
int Decoder::decodeAudio(Packet &encodedPacket, Buffer &decoded, bool flush)
{
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
