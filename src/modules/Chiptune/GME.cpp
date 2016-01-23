#include <GME.hpp>

#include <Functions.hpp>
#include <Reader.hpp>
#include <Packet.hpp>

#include <gme/gme.h>

GME::GME(Module &module) :
	m_srate(Functions::getBestSampleRate()),
	m_aborted(false),
	m_gme(NULL)
{
	SetModule(module);
}
GME::~GME()
{
	gme_delete(m_gme);
}

bool GME::set()
{
	m_length = sets().getInt("DefaultLength");
	return sets().getBool("GME");
}

QString GME::name() const
{
	return gme_type_system(gme_type(m_gme));
}
QString GME::title() const
{
	return m_title;
}
QList<QMPlay2Tag> GME::tags() const
{
	return m_tags;
}
double GME::length() const
{
	return m_length;
}
int GME::bitrate() const
{
	return -1;
}

bool GME::seek(int s, bool)
{
	return !gme_seek(m_gme, s * 1000);
}
bool GME::read(Packet &decoded, int &idx)
{
	if (m_aborted || gme_track_ended(m_gme))
		return false;

	gme_set_fade(m_gme, (m_length - 8) * 1000); //Set every time to avoid problems

	const int chunkSize = 1024 * 2; //Always stereo

	decoded.resize(chunkSize * sizeof(float));
	qint16 *srcData = (qint16 *)decoded.data();
	float *dstData = (float *)decoded.data();

	if (gme_play(m_gme, chunkSize, srcData) != NULL)
		return false;

	for (int i = chunkSize - 1; i >= 0; --i)
		dstData[i] = srcData[i] / 32768.0;

	decoded.ts = gme_tell(m_gme) / 1000.0;
	decoded.duration = chunkSize / 2 / (double)m_srate;

	idx = 0;

	return true;
}
void GME::abort()
{
	m_reader.abort();
	m_aborted = true;
}

bool GME::open(const QString &url)
{
	return open(url, false);
}

Playlist::Entries GME::fetchTracks(const QString &url, bool &ok)
{
	Playlist::Entries entries;
	if (open(url, true))
	{
		const int tracks = gme_track_count(m_gme);
		for (int i = 0; i < tracks; ++i)
		{
			gme_info_t *info = NULL;
			if (!gme_track_info(m_gme, &info, i) && info)
			{
				Playlist::Entry entry;
				entry.url = GMEName + QString("://{%1}%2").arg(m_url).arg(i);
				entry.name = getTitle(info, i);
				entry.length = getLength(info);
				gme_free_info(info);
				entries.append(entry);
			}
		}
		if (entries.length() > 1)
		{
			for (int i = 0; i < entries.length(); ++i)
				entries[i].parent = 1;
			Playlist::Entry entry;
			entry.name = Functions::fileName(m_url, false);
			entry.url = m_url;
			entry.GID = 1;
			entries.prepend(entry);
		}
	}
	ok = !entries.isEmpty();
	return entries;
}


bool GME::open(const QString &_url, bool tracksOnly)
{
	QString prefix, url, param;
	const bool hasPluginPrefix = Functions::splitPrefixAndUrlIfHasPluginPrefix(_url, &prefix, &url, &param);

	if (tracksOnly == hasPluginPrefix)
		return false;

	int track = 0;
	if (!hasPluginPrefix)
	{
		if (url.startsWith(GMEName "://"))
			return false;
		url = _url;
	}
	else
	{
		if (prefix != GMEName)
			return false;
		bool ok;
		track = param.toInt(&ok);
		if (track < 0 || !ok)
			return false;
	}

	if (Reader::create(url, m_reader))
	{
		const QByteArray data = m_reader->read(m_reader->size());
		m_reader.clear();

		gme_open_data(data.data(), data.size(), &m_gme, m_srate);
		if (!m_gme)
			return false;

		if (!hasPluginPrefix)
		{
			m_aborted = true;
			m_url = url;
			return true;
		}

		if (track >= gme_track_count(m_gme))
			return false;

		gme_info_t *info = NULL;
		if (!gme_track_info(m_gme, &info, track) && info)
		{
			m_title = getTitle(info, track);
			m_length = getLength(info);

			if (*info->game)
				m_tags << qMakePair(QString::number(QMPLAY2_TAG_TITLE), QString(info->game));
			if (*info->author)
				m_tags << qMakePair(QString::number(QMPLAY2_TAG_ARTIST), QString(info->author));
			if (*info->system)
				m_tags << qMakePair(tr("System"), QString(info->system));
			if (*info->copyright)
				m_tags << qMakePair(tr("Copyright"), QString(info->copyright));
			if (*info->comment)
				m_tags << qMakePair(tr("Comment"), QString(info->comment));

			gme_free_info(info);
		}

		QString voices;
		const int numVoices = gme_voice_count(m_gme);
		for (int i = 0; i < numVoices; ++i)
		{
			voices += gme_voice_name(m_gme, i);
			voices += ", ";
		}
		voices.chop(2);
		m_tags << qMakePair(tr("Voices"), voices);

		m_tags << qMakePair(tr("Track"), QString::number(track + 1));

		streams_info += new StreamInfo(m_srate, 2);

		gme_set_stereo_depth(m_gme, 0.5);
		return !gme_start_track(m_gme, track);
	}

	return false;
}

QString GME::getTitle(gme_info_t *info, int track) const
{
	const QString title  = info->game;
	const QString author = info->author;
	QString ret;
	if (!author.isEmpty() && !title.isEmpty())
		ret = author + " - " + title;
	else
		ret = title;
	if (gme_track_count(m_gme) > 1)
		return tr("Track") + QString(" %1%2").arg(track + 1).arg(ret.isEmpty() ? QString() : (" - " + ret));
	return ret;
}
int GME::getLength(gme_info_t *info) const
{
	int length = m_length;
	if (info->length > -1)
		length = info->play_length / 1000.0;
	else if (info->intro_length > -1 && info->loop_length > -1)
		length = (info->intro_length * info->loop_length * 2) / 1000.0;
	return length;
}
