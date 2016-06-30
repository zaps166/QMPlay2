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

#include <LastFM.hpp>

static const char api_key[] = "b1165c9688c2fcf29fc74c2ab62ffd90";
static const char secret[]  = "e36ce24a59f45514daad8fccec294c34";
static const int scrobbleSec = 5;

Q_DECLARE_METATYPE(LastFM::Scrobble)

/**/

#include <QCryptographicHash>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QStringList>
#include <QImage>

LastFM::LastFM(Module &module) :
	coverReply(NULL),
	loginReply(NULL),
	scrobbleReply(NULL),
	dontShowLoginError(false),
	firstTime(true)
{
	SetModule(module);
	updateTim.setSingleShot(true);
	loginTimer.setSingleShot(true);
	connect(&updateTim, SIGNAL(timeout()), this, SLOT(processScrobbleQueue()));
	connect(&loginTimer, SIGNAL(timeout()), this, SLOT(login()));
	connect(&QMPlay2Core, SIGNAL(updatePlaying(bool, QString, QString, QString, int, bool, QString)), this, SLOT(updatePlaying(bool, QString, QString, QString, int, bool, QString)));
}

bool LastFM::set()
{
	downloadCovers = sets().getBool("LastFM/DownloadCovers");

	imageSizes.clear();
	if (sets().getBool("LastFM/AllowBigCovers"))
		imageSizes += "mega";
	imageSizes += QStringList() << "extralarge" << "large" << "medium" << "small";

	const QString _user = sets().getString("LastFM/Login");
	const QString _md5pass = sets().getString("LastFM/Password");

	if (!sets().getBool("LastFM/UpdateNowPlayingAndScrobble"))
		logout();
	else if (_user != user || _md5pass != md5pass)
	{
		user = _user;
		md5pass = _md5pass;
		if (!firstTime)
		{
			logout(false);
			login();
		}
	}

	firstTime = false;

	return true;
}

void LastFM::getAlbumCover(const QString &title, const QString &artist, const QString &album)
{
	static const QString infoURL = "http://ws.audioscrobbler.com/2.0/?method=%1.getInfo&api_key=%4&artist=%2&%1=%3";
	if (!artist.isEmpty() && (!title.isEmpty() || !album.isEmpty()))
	{
		const QString url = infoURL.arg(album.isEmpty() ? "track" : "album").arg(artist).arg(album.isEmpty() ? title : album).arg(api_key);
		if (coverReply)
		{
			disconnect(coverReply, SIGNAL(finished()), this, SLOT(albumFinished()));
			coverReply->deleteLater();
		}
		coverReply = net.get(QNetworkRequest(url));
		coverReply->setProperty("taa", QStringList() << title << artist << album);
		connect(coverReply, SIGNAL(finished()), this, SLOT(albumFinished()));
	}
}

void LastFM::login()
{
	static const QString getSessionURL = "http://ws.audioscrobbler.com/2.0/?method=auth.getmobilesession&username=%1&authToken=%2&api_key=%3&api_sig=%4";
	if (!loginReply && !user.isEmpty() && md5pass.length() == 32)
	{
		const QString auth_token = QCryptographicHash::hash(user.toUtf8() + md5pass.toUtf8(), QCryptographicHash::Md5).toHex();
		const QString api_sig = QCryptographicHash::hash(QString("api_key%1authToken%2methodauth.getmobilesessionusername%3%4").arg(api_key).arg(auth_token).arg(user).arg(secret).toUtf8(), QCryptographicHash::Md5).toHex();
		loginReply = net.get(QNetworkRequest(getSessionURL.arg(user).arg(auth_token).arg(api_key).arg(api_sig)));
		connect(loginReply, SIGNAL(finished()), this, SLOT(loginFinished()));
	}
}
void LastFM::logout(bool canClear)
{
	updateTim.stop();
	loginTimer.stop();
	if (loginReply)
	{
		loginReply->deleteLater();
		loginReply = NULL;
	}
	if (scrobbleReply)
	{
		scrobbleReply->deleteLater();
		scrobbleReply = NULL;
	}
	if (canClear)
		clear();
	else
		session_key.clear();
}

void LastFM::updateNowPlayingAndScrobble(const Scrobble &scrobble)
{
	static const QString audioScrobbler2URL = "http://ws.audioscrobbler.com/2.0";
	if (!session_key.isEmpty())
	{
		QNetworkRequest request(audioScrobbler2URL);
		QString api_sig;

		request.setRawHeader("content-type", "application/x-www-form-urlencoded");

		int duration = scrobble.duration - (time(NULL) - scrobble.startTime);
		if (duration < 0)
			duration = 0;

		//updateNowPlaying
		api_sig = QCryptographicHash::hash(QString("album%1api_key%2artist%3duration%4methodtrack.updatenowplayingsk%5track%6%7").arg(scrobble.album).arg(api_key).arg(scrobble.artist).arg(duration).arg(session_key).arg(scrobble.title).arg(secret).toUtf8(), QCryptographicHash::Md5).toHex();
		QNetworkReply *reply = net.post(request, QString("album=%1&api_key=%2&api_sig=%3&artist=%4&duration=%5&method=track.updatenowplaying&sk=%6&track=%7").arg(scrobble.album).arg(api_key).arg(api_sig).arg(scrobble.artist).arg(duration).arg(session_key).arg(scrobble.title).toUtf8());
		connect(reply, SIGNAL(finished()), reply, SLOT(deleteLater()));

		//scrobble
		const QString ts = QString::number(scrobble.startTime);
		api_sig = QCryptographicHash::hash(QString("album%1api_key%2artist%3methodtrack.scrobblesk%4timestamp%5track%6%7").arg(scrobble.album).arg(api_key).arg(scrobble.artist).arg(session_key).arg(ts).arg(scrobble.title).arg(secret).toUtf8(), QCryptographicHash::Md5).toHex();
		scrobbleReply = net.post(request, QString("album=%1&api_key=%2&api_sig=%3&artist=%4&method=track.scrobble&sk=%5&timestamp=%6&track=%7").arg(scrobble.album).arg(api_key).arg(api_sig).arg(scrobble.artist).arg(session_key).arg(ts).arg(scrobble.title).toUtf8());
		scrobbleReply->setProperty("scrobble", QVariant::fromValue(scrobble));
		connect(scrobbleReply, SIGNAL(finished()), this, SLOT(scrobbleFinished()));
	}
}

void LastFM::clear()
{
	user.clear();
	md5pass.clear();
	updateTim.stop();
	loginTimer.stop();
	session_key.clear();
	scrobbleQueue.clear();
	dontShowLoginError = false;
}

void LastFM::updatePlaying(bool play, const QString &title, const QString &artist, const QString &album, int length, bool needCover, const QString &fileName)
{
	Q_UNUSED(fileName)
	if (!artist.isEmpty() && (!title.isEmpty() || !album.isEmpty()))
	{
		if (!user.isEmpty() && !md5pass.isEmpty())
		{
			const time_t currTime = time(NULL);
			const Scrobble scrobble = {title, artist, album, currTime, length};
			if (play)
			{
				if (!scrobbleQueue.isEmpty() && currTime - scrobbleQueue.last().startTime < scrobbleSec)
					scrobbleQueue.removeLast();
				if (length < 0)
					length = 0;
				scrobbleQueue.enqueue(scrobble);
			}
			else for (int i = 0; i < scrobbleQueue.count(); ++i)
				if (scrobbleQueue[i] == scrobble && currTime - scrobbleQueue[i].startTime < scrobbleSec)
					scrobbleQueue.removeAt(i);
			if (scrobbleQueue.isEmpty())
				updateTim.stop();
			else if (!session_key.isEmpty())
				updateTim.start(scrobbleSec * 1000);
			else if (play && !loginTimer.isActive())
				login();
		}
		if (downloadCovers && needCover)
			getAlbumCover(title, artist, album);
	}
}

void LastFM::albumFinished()
{
	if (!coverReply->error())
	{
		const QByteArray reply = coverReply->readAll();
		if (!coverReply->url().toString().contains("api_key"))
		{
			const QStringList taa = coverReply->property("taa").toStringList();
			emit QMPlay2Core.updateCover(taa[0], taa[1], taa[2], reply);
		}
		else foreach (const QString &size, imageSizes)
		{
			int idx = reply.indexOf(size);
			if (idx > -1)
			{
				idx = reply.indexOf("http:", idx += size.length());
				if (idx > -1)
				{
					int end_idx = reply.indexOf("<", idx);
					if (end_idx > -1)
					{
						const QUrl imgUrl = QUrl(reply.mid(idx, end_idx - idx));
						if (imgUrl.toString().contains("noimage"))
							continue;
						const QVariant taa = coverReply->property("taa");
						coverReply->deleteLater();
						coverReply = net.get(QNetworkRequest(imgUrl));
						coverReply->setProperty("taa", taa);
						connect(coverReply, SIGNAL(finished()), this, SLOT(albumFinished()));
						return;
					}
				}
			}
		}
	}
	coverReply->deleteLater();
	coverReply = NULL;
}
void LastFM::loginFinished()
{
	const QByteArray reply = loginReply->readAll();
	if (loginReply->error())
	{
		const bool wrongLoginOrPassword = reply.contains("error code=\"4\"");
		if (!dontShowLoginError || wrongLoginOrPassword)
			QMPlay2Core.logError(tr("LastFM login error.") + (wrongLoginOrPassword ? (" " + tr("Check login and password!")) : QString()));
		if (wrongLoginOrPassword)
			clear();
		else
		{
			dontShowLoginError = true;
			loginTimer.start(30000);
		}
	}
	else
	{
		int idx = reply.indexOf("<key>");
		if (idx > -1)
		{
			int end_idx = reply.indexOf("</key>", idx += 5);
			if (end_idx > -1)
			{
				session_key = reply.mid(idx, end_idx - idx);
				QMPlay2Core.log(tr("Logged in to LastFM!"), InfoLog);
				if (!scrobbleQueue.isEmpty() && !updateTim.isActive())
					updateTim.start(scrobbleSec * 1000);
				dontShowLoginError = false;
			}
		}
	}
	loginReply->deleteLater();
	loginReply = NULL;
}
void LastFM::scrobbleFinished()
{
	if (scrobbleReply->error())
	{
		scrobbleQueue.enqueue(scrobbleReply->property("scrobble").value<Scrobble>());
		logout(false);
		login();
	}
	else
	{
		scrobbleReply->deleteLater();
		scrobbleReply = NULL;
	}
}

void LastFM::processScrobbleQueue()
{
	while (!scrobbleQueue.isEmpty())
		updateNowPlayingAndScrobble(scrobbleQueue.dequeue());
}
