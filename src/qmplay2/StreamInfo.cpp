#include <StreamInfo.hpp>

QMPlay2Tags StreamInfo::getTag(const QString &tag)
{
	bool ok;
	const int tagID = tag.toInt(&ok);
	if (ok && tagID >= QMPLAY2_TAG_LANGUAGE && tagID <= QMPLAY2_TAG_COMMENT)
		return (QMPlay2Tags)tagID;
	return QMPLAY2_TAG_UNKNOWN;
}
QString StreamInfo::getTagName(const QString &tag)
{
	const QMPlay2Tags tagID = getTag(tag);
	switch (tagID)
	{
		case QMPLAY2_TAG_LANGUAGE:
			return tr("Język");
		case QMPLAY2_TAG_TITLE:
			return tr("Tytuł");
		case QMPLAY2_TAG_ARTIST:
			return tr("Artysta");
		case QMPLAY2_TAG_ALBUM:
			return tr("Album");
		case QMPLAY2_TAG_GENRE:
			return tr("Gatunek");
		case QMPLAY2_TAG_DATE:
			return tr("Data");
		case QMPLAY2_TAG_COMMENT:
			return tr("Komentarz");
		default:
			return tag;
	}
}
