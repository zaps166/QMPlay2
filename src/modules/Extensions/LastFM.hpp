#ifndef LASTFM_HPP
#define LASTFM_HPP

#include <QMPlay2Extensions.hpp>

#include <QNetworkAccessManager>
#include <QTimer>
#include <QQueue>

#include <time.h>

class QImage;

class LastFM : public QObject, public QMPlay2Extensions
{
	Q_OBJECT
public:
	class Scrobble
	{
	public:
		inline bool operator ==(const Scrobble &other)
		{
			return title == other.title && artist == other.artist && album == other.album && duration == other.duration;
		}

		QString title, artist, album;
		time_t startTime;
		int duration;
	};

	LastFM(Module &module);
private:
	bool set();

	void getAlbumCover(const QString &title, const QString &artist, const QString &album);

	Q_SLOT void login();
	void logout(bool canClear = true);

	void updateNowPlayingAndScrobble(const Scrobble &scrobble);

	void clear();
private slots:
	void updatePlaying(bool play, const QString &title, const QString &artist, const QString &album, int length, bool needCover, const QString &fileName);

	void albumFinished();
	void loginFinished();
	void scrobbleFinished();

	void processScrobbleQueue();
private:
	QNetworkReply *coverReply, *loginReply, *scrobbleReply;
	bool downloadCovers, dontShowLoginError, firstTime;
	QString user, md5pass, session_key;
	QQueue< Scrobble > scrobbleQueue;
	QTimer updateTim, loginTimer;
	QNetworkAccessManager net;
	QStringList imageSizes;
};

#define LastFMName "LastFM"

#endif // LASTFM_HPP
