/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2018  Błażej Szczygieł

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

#include <Downloader.hpp>

#include <Functions.hpp>
#include <CppUtils.hpp>
#include <MkvMuxer.hpp>
#include <Demuxer.hpp>
#include <Packet.hpp>
#include <Reader.hpp>

#include <QMenu>
#include <QTimer>
#include <QLabel>
#include <QAction>
#include <QScreen>
#include <QWindow>
#include <QProcess>
#include <QMimeData>
#include <QClipboard>
#include <QFileDialog>
#include <QTreeWidget>
#include <QToolButton>
#include <QGridLayout>
#include <QTreeWidget>
#include <QMessageBox>
#include <QFormLayout>
#include <QPushButton>
#include <QInputDialog>
#include <QProgressBar>
#include <QApplication>
#include <QElapsedTimer>
#include <QStandardPaths>
#include <QLoggingCategory>
#include <QDialogButtonBox>

#include <functional>

Q_LOGGING_CATEGORY(downloader, "Downloader")

/**/

constexpr const char *g_defaultMp3ConvertCommand = "ffmpeg -i <input/> -vn -sn -c:a libmp3lame -ab 224k -f mp3 -y <output>%f.mp3</output>";

constexpr const char *g_downloadComplete = QT_TRANSLATE_NOOP("DownloadItemW", "Download complete");
constexpr const char *g_downloadAborted = QT_TRANSLATE_NOOP("DownloadItemW", "Download aborted");
constexpr const char *g_downloadError = QT_TRANSLATE_NOOP("DownloadItemW", "Download error");
constexpr const char *g_conversionAborted = QT_TRANSLATE_NOOP("DownloadItemW", "Conversion aborted");
constexpr const char *g_conversionError = QT_TRANSLATE_NOOP("DownloadItemW", "Conversion error");

/**/

static QStringRef getCommandOutput(const QString &command)
{
	const int idx1 = command.indexOf("<output>");
	if (idx1 < 0)
		return {};

	const int idx2 = command.indexOf("</output>");
	if (idx2 < 0)
		return {};

	return command.midRef(idx1 + 8, idx2 - idx1 - 8);
}

/**/

DownloadItemW::DownloadItemW(DownloaderThread *downloaderThr, QString name, const QIcon &icon, QDataStream *stream, QString preset) :
	dontDeleteDownloadThr(false), downloaderThr(downloaderThr), finished(false), readyToPlay(false)
{
	QString sizeLText;

	if (stream)
	{
		quint8 type;
		*stream >> filePath >> type >> name >> preset;
		finished = true;
		switch (type)
		{
			case 1:
				readyToPlay = true;
				sizeLText = tr(g_downloadComplete);
				break;
			case 2:
				sizeLText = tr(g_downloadAborted);
				break;
			case 3:
				sizeLText = tr(g_downloadError);
				break;
			case 4:
				sizeLText = tr(g_conversionAborted);
				m_needsConversion = true;
				break;
			case 5:
				sizeLText = tr(g_conversionError);
				m_needsConversion = true;
				break;
		}
	}
	else
	{
		sizeLText = tr("Waiting for connection");
	}

	titleL = new QLabel(name);

	sizeL = new QLabel(sizeLText);

	iconL = new QLabel;
	iconL->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred));
	iconL->setPixmap(Functions::getPixmapFromIcon(icon.isNull() ? QMPlay2Core.getIconFromTheme("applications-multimedia") : icon, QSize(22, 22), this));

	ssB = new QToolButton;
	if (readyToPlay)
	{
		ssB->setIcon(QMPlay2Core.getIconFromTheme("media-playback-start"));
		ssB->setToolTip(tr("Play"));
	}
	else if (finished)
	{
		ssB->setIcon(QMPlay2Core.getIconFromTheme("view-refresh"));
		ssB->setToolTip(tr("Download again"));
	}
	else
	{
		ssB->setIcon(QMPlay2Core.getIconFromTheme("media-playback-stop"));
		ssB->setToolTip(tr("Stop downloading"));
	}
	connect(ssB, SIGNAL(clicked()), this, SLOT(toggleStartStop()));

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(iconL, 0, 0, 2, 1);
	layout->addWidget(titleL, 0, 1, 1, 2);
	layout->addWidget(sizeL, 1, 1, 1, 2);
	if (!finished)
	{
		QHBoxLayout *hLayout = new QHBoxLayout;

		speedProgressW = new SpeedProgressWidget;
		speedProgressW->setLayout(hLayout);

		speedProgressW->progressB = new QProgressBar;
		speedProgressW->progressB->setRange(0, 0);
		hLayout->addWidget(speedProgressW->progressB);

		speedProgressW->speedL = new QLabel;
		hLayout->addWidget(speedProgressW->speedL);

		layout->addWidget(speedProgressW, 2, 0, 1, 2);
	}
	layout->addWidget(ssB, 2, 2, 1, 1);

	m_convertPreset = preset;
}
DownloadItemW::~DownloadItemW()
{
	deleteConvertProcess();
	if (!dontDeleteDownloadThr)
	{
		finish(false);
		delete downloaderThr;
	}
}

void DownloadItemW::setName(const QString &name)
{
	if (!finished)
		titleL->setText(name);
}
void DownloadItemW::setSizeAndFilePath(qint64 size, const QString &filePath)
{
	if (!finished)
	{
		sizeL->setText(tr("Size") + ": " + (size > -1 ? Functions::sizeString(size) : "?"));
		speedProgressW->progressB->setRange(0, (size != -1) ? 100 : 0);
		this->filePath = filePath;
	}
}
void DownloadItemW::setPos(int pos)
{
	if (!finished)
		speedProgressW->progressB->setValue(pos);
}
void DownloadItemW::setSpeed(int Bps)
{
	if (!finished)
		speedProgressW->speedL->setText(Functions::sizeString(Bps) + "/s");
}
void DownloadItemW::finish(bool f)
{
	if (!finished)
	{
		bool canStop = true;
		delete speedProgressW;
		speedProgressW = nullptr;
		if (f)
		{
			if (m_convertPreset.isEmpty())
			{
				sizeL->setText(tr(g_downloadComplete));
			}
			else
			{
				startConversion();
				canStop = false;
			}
		}
		else
		{
			if (m_needsConversion)
				sizeL->setText(tr(g_conversionAborted));
			else
				sizeL->setText(tr(g_downloadAborted));
		}
		if (canStop)
			downloadStop(f);
	}
}
void DownloadItemW::error()
{
	if (!finished)
	{
		if (speedProgressW->progressB->minimum() == speedProgressW->progressB->maximum())
			speedProgressW->progressB->setRange(-1, 0);
		speedProgressW->setEnabled(false);
		sizeL->setText(tr(g_downloadError));
		downloadStop(false);
	}
}

void DownloadItemW::write(QDataStream &stream)
{
	downloaderThr->serialize(stream);
	quint8 type = readyToPlay;
	if (!type)
	{
		if (sizeL->text() == tr(g_conversionError))
			type = 5;
		else if (m_needsConversion)
			type = 4;
		else if (sizeL->text() == tr(g_downloadError))
			type = 3;
		else
			type = 2;
	}
	stream << filePath << type << titleL->text() << m_convertPreset;
}

void DownloadItemW::toggleStartStop()
{
	if (readyToPlay)
	{
		if (!filePath.isEmpty())
			emit QMPlay2Core.processParam("open", filePath);
	}
	else if (finished)
	{
		if (m_needsConversion)
		{
			startConversion();
		}
		else
		{
			filePath.clear();
			emit start();
		}
	}
	else
	{
		finish(false);
		if (m_convertProcess)
		{
			deleteConvertProcess();
		}
		else
		{
			ssB->setEnabled(false);
			emit stop();
		}
	}
}

void DownloadItemW::downloadStop(bool ok)
{
	if (!ok)
	{
		ssB->setIcon(QMPlay2Core.getIconFromTheme("view-refresh"));
		ssB->setToolTip(tr("Download again"));
	}
	else
	{
		ssB->setIcon(QMPlay2Core.getIconFromTheme("media-playback-start"));
		ssB->setToolTip(tr("Play"));
		readyToPlay = true;
	}
	finished = true;
	if (!dontDeleteDownloadThr)
	{
		if (visibleRegion().isNull())
			emit QMPlay2Core.sendMessage(titleL->text(), sizeL->text());
	}
}

void DownloadItemW::startConversion()
{
	deleteConvertProcess();

	m_convertProcess = new QProcess(this);
	m_convertProcessConn[0] = connect(m_convertProcess, Overload<int>::of(&QProcess::finished), this, [this](int exitCode) {
		if (exitCode == 0)
		{
			sizeL->setText(tr(g_downloadComplete));
			QFile::remove(filePath);
			m_needsConversion = false;
			filePath = m_convertedFilePath;
			downloadStop(true);
		}
		else
		{
			sizeL->setText(tr(g_conversionError));
			qCWarning(downloader) << "Failed to convert:" << m_convertProcess->program() << m_convertProcess->arguments() << m_convertProcess->readAllStandardError().constData();
			downloadStop(false);
		}
	});
	m_convertProcessConn[1] = connect(m_convertProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
		if (error == QProcess::FailedToStart)
		{
			sizeL->setText(tr(g_conversionError));
			downloadStop(false);
			qCWarning(downloader) << "Failed to start process:" << m_convertProcess->program();
		}
	});

	m_needsConversion = true;
	finished = false;

	sizeL->setText(tr("Converting..."));

	ssB->setIcon(QMPlay2Core.getIconFromTheme("media-playback-stop"));
	ssB->setToolTip(tr("Stop conversion"));

	const auto conversionError = [&](const QString &errStr) {
		sizeL->setText(tr(g_conversionError));
		downloadStop(false);
		qCWarning(downloader).noquote() << errStr;
	};

	QString convertCommand;
	for (QAction *a : downloaderThr->convertActions())
	{
		if (a->text() == m_convertPreset)
		{
			convertCommand = a->data().toString();
			break;
		}
	}
	if (convertCommand.isEmpty())
	{
		conversionError(QString("Can't convert, because preset \"%1\" not found or invalid!").arg(m_convertPreset));
		return;
	}

	if (!convertCommand.contains("<input/>"))
	{
		conversionError("Can't convert, because \"<input/>\" tag doesn't exist!");
		return;
	}
	convertCommand.replace("<input/>", "\"" + filePath + "\"");

	const int idx1 = convertCommand.indexOf("<output>");
	const int idx2 = convertCommand.indexOf("</output>", idx1);

	m_convertedFilePath = getCommandOutput(convertCommand).toString();
	m_convertedFilePath.replace("%f", Functions::filePath(filePath) + Functions::fileName(filePath, false));
	if (m_convertedFilePath.isEmpty() || idx1 < 0 || idx2 < idx1)
	{
		conversionError("Can't convert, because output file path is empty!");
		return;
	}
	if (m_convertedFilePath.compare(filePath, Qt::CaseInsensitive) == 0)
	{
		conversionError("Can't convert, because source and destination path is the same: " + m_convertedFilePath);
		return;
	}

	convertCommand.replace(idx1, idx2 - idx1 + 9, "\"" + m_convertedFilePath + "\"");

	qDebug() << "Starting conversion:" << convertCommand.toUtf8().constData();
	m_convertProcess->start(convertCommand);
}
void DownloadItemW::deleteConvertProcess()
{
	if (m_convertProcess)
	{
		disconnect(m_convertProcessConn[0]);
		disconnect(m_convertProcessConn[1]);
		m_convertProcess->close();
		delete m_convertProcess;
		m_convertProcess = nullptr;
	}
}

/**/

DownloaderThread::DownloaderThread(QDataStream *stream, const QString &url, DownloadListW *downloadLW, const QMenu *convertsMenu, const QString &name, const QString &prefix, const QString &param, const QString &preset) :
	url(url), name(name), prefix(prefix), param(param), preset(preset), downloadItemW(nullptr), downloadLW(downloadLW), item(nullptr), m_convertsMenu(convertsMenu)
{
	connect(this, SIGNAL(listSig(int, qint64, const QString &)), this, SLOT(listSlot(int, qint64, const QString &)));
	connect(this, SIGNAL(finished()), this, SLOT(finished()));
	if (stream)
	{
		*stream >> this->url >> this->prefix >> this->param;
		item = new QTreeWidgetItem(downloadLW);
		downloadItemW = new DownloadItemW(this, QString(), getIcon(), stream, preset);
		downloadLW->setItemWidget(item, 0, downloadItemW);
		connect(downloadItemW, SIGNAL(start()), this, SLOT(start()));
		connect(downloadItemW, SIGNAL(stop()), this, SLOT(stop()));
	}
	else
	{
		start();
	}
}
DownloaderThread::~DownloaderThread()
{
	disconnect(this, SIGNAL(finished()), this, SLOT(finished()));
	stop();
	if (!wait(2000))
	{
		terminate();
		wait(500);
	}
}

void DownloaderThread::serialize(QDataStream &stream)
{
	stream << url << prefix << param;
}

const QList<QAction *> DownloaderThread::convertActions()
{
	QList<QAction *> actions = m_convertsMenu->actions();
	actions.removeFirst();
	return actions;
}

void DownloaderThread::listSlot(int param, qint64 val, const QString &filePath)
{
	switch (param)
	{
		case ADD_ENTRY:
			if (!item)
				item = new QTreeWidgetItem(downloadLW);
			if (downloadItemW)
			{
				downloadItemW->dontDeleteDownloadThr = true;
				downloadItemW->deleteLater();
			}
			downloadItemW = new DownloadItemW(this, name.isEmpty() ? url : name, getIcon(), nullptr, preset);
			downloadLW->setItemWidget(item, 0, downloadItemW);
			connect(downloadItemW, SIGNAL(start()), this, SLOT(start()));
			connect(downloadItemW, SIGNAL(stop()), this, SLOT(stop()));

			// Workaround: Resize the widget twice to get correct item size
			downloadLW->resize(downloadLW->size() + QSize(0, 1));
			downloadLW->resize(downloadLW->size() - QSize(0, 1));

			break;
		case NAME:
			downloadItemW->setName(name);
			break;
		case SET:
			downloadItemW->setSizeAndFilePath(val, filePath);
			break;
		case SET_POS:
			downloadItemW->setPos(val);
			break;
		case SET_SPEED:
			downloadItemW->setSpeed(val);
			break;
		case DOWNLOAD_ERROR:
			downloadItemW->error();
			break;
		case FINISH:
			downloadItemW->finish();
			break;
	}
}
void DownloaderThread::stop()
{
	ioCtrl.abort();
}
void DownloaderThread::finished()
{
	if (downloadItemW)
		downloadItemW->ssBEnable();
}

void DownloaderThread::run()
{
	ioCtrl.resetAbort();

	QString scheme = Functions::getUrlScheme(url);
	if (scheme.isEmpty())
		url.prepend("http://");
	else if (scheme == "file")
	{
		if (!item)
			deleteLater();
		emit QMPlay2Core.sendMessage(tr("Invalid address"), DownloaderName);
		return;
	}

	emit listSig(ADD_ENTRY);

	QString newUrl = url;
	QString extension;

	if (!prefix.isEmpty())
		for (QMPlay2Extensions *QMPlay2Ext : QMPlay2Extensions::QMPlay2ExtensionsList())
			if (QMPlay2Ext->addressPrefixList(false).contains(prefix))
			{
				newUrl.clear();
				QMPlay2Ext->convertAddress(prefix, url, param, &newUrl, &name, nullptr, &extension, &ioCtrl);
				break;
			}

	const auto getFilePath = [&]()->QString {
		QString filePath;
		quint16 num = 0;
		do
			filePath = downloadLW->getDownloadsDirPath() + (num ? (QString::number(num) + "_") : QString()) + Functions::cleanFileName(name);
		while (QFile::exists(filePath) && ++num < 0xFFFF);
		if (num == 0xFFFF)
			filePath.clear();
		return filePath;
	};

	QElapsedTimer speedT;
	const auto setByteRate = [&](const std::function<qint64()> &getPosDiff) {
		const int elapsed = speedT.elapsed();
		if (elapsed >= 1000)
		{
			emit listSig(SET_SPEED, getPosDiff() * 1000 / elapsed);
			speedT.restart();
		}
	};

	bool err = true;

	const bool isFFmpeg = newUrl.startsWith("FFmpeg://");
	const bool isHLS = (!isFFmpeg && (newUrl.endsWith(".m3u8", Qt::CaseInsensitive) || newUrl.contains(".m3u8?", Qt::CaseInsensitive)));
	if (isFFmpeg || isHLS)
	{
		IOController<Demuxer> &demuxer = ioCtrl.toRef<Demuxer>();
		QMPlay2Core.setWorking(true);
		if (Demuxer::create(newUrl, demuxer) && !demuxer.isAborted())
		{
			if (name.isEmpty())
				name = demuxer->title();
			if (name.isEmpty())
			{
				name = Functions::fileName(newUrl);
				int idx = name.indexOf("?");
				if (idx > -1)
					name.remove(idx, name.size() - idx);
			}
			if (!name.endsWith(".mkv", Qt::CaseInsensitive))
				name += ".mkv";
			emit listSig(NAME);

			QString filePath = getFilePath();
			if (!filePath.isEmpty())
			{
				const QList<StreamInfo *> streamsInfo = demuxer->streamsInfo();
				MkvMuxer muxer(filePath, streamsInfo);
				if (muxer.isOk())
				{
					const double length = demuxer->length();
					const qint64 size = demuxer->size();
					double lastBytesPos = 0.0;
					double bytePos = 0.0;
					double pos = 0.0;

					emit listSig(SET, (size < 0 || isHLS) ? (length < 0 ? -1 : -2) : size, filePath);
					err = false;
					speedT.start();
					while (!demuxer.isAborted())
					{
						Packet packet;
						int idx = -1;
						if (!demuxer->read(packet, idx))
							break;
						if (idx < 0 || idx >= streamsInfo.count())
							continue;

						if (!muxer.write(packet, idx))
						{
							err = true;
							break;
						}

						bytePos += packet.size();
						setByteRate([&] {
							const qint64 tmp = bytePos - lastBytesPos;
							lastBytesPos = bytePos;
							return tmp;
						});

						if (length > 0.0)
						{
							pos = qMax<double>(pos, packet.ts);
							emit listSig(SET_POS, pos * 100 / length);
						}
					}
				}
			}
			demuxer.reset();
		}
		emit listSig(err ? DOWNLOAD_ERROR : FINISH);
		QMPlay2Core.setWorking(false);
		return;
	}

	if (name.isEmpty())
	{
		name = Functions::fileName(newUrl);
		int idx = name.indexOf("?");
		if (idx > -1)
			name.remove(idx, name.size() - idx);
	}
	else if (param.isEmpty() && extension.isEmpty())
	{
		// Extract file extension from URL if exists
		QString tmp = newUrl;
		int idx = tmp.indexOf("?");
		if (idx > -1)
			tmp.remove(idx, tmp.size() - idx);
		const int idx1 = tmp.indexOf("://");
		const int idx2 = tmp.lastIndexOf(".");
		if (idx1 > -1 && idx2 > -1 && tmp.lastIndexOf('/', idx2) != idx1 + 2)
			name += tmp.mid(idx2);
	}
	if (!name.isEmpty() && !extension.isEmpty())
		name += extension;
	emit listSig(NAME);

	if (ioCtrl.isAborted())
		return;

	QMPlay2Core.setWorking(true);

	IOController<Reader> &reader = ioCtrl.toRef<Reader>();
	if (!newUrl.isEmpty())
		Reader::create(newUrl, reader);
	if (reader && reader->readyRead() && !reader->atEnd())
	{
		QFile file(getFilePath());
		if (!file.fileName().isEmpty() && file.open(QFile::WriteOnly))
		{
			qint64 lastBytesPos = 0;
			int lastPos = -1;

			emit listSig(SET, qMax<qint64>(-1, reader->size()), file.fileName());
			err = false;
			speedT.start();
			while (!reader.isAborted() && !(err = !reader->readyRead()) && !reader->atEnd())
			{
				const QByteArray arr = reader->read(16384);
				if (arr.size())
				{
					if (file.write(arr) != arr.size())
					{
						err = true;
						break;
					}
				}
				else
				{
					if (!reader.isAborted() && ((reader->size() < 0 && !file.size()) || (reader->size() > -1 && !reader->atEnd())))
						err = true;
					break;
				}

				const qint64 bytesPos = reader->pos();
				setByteRate([&] {
					const qint64 tmp = bytesPos - lastBytesPos;
					lastBytesPos = bytesPos;
					return tmp;
				});
				if (reader->size() > 0)
				{
					const int pos = bytesPos * 100 / reader->size();
					if (pos != lastPos)
					{
						emit listSig(SET_POS, pos);
						lastPos = pos;
					}
				}
			}
		}
		reader.reset();
	}
	emit listSig(err ? DOWNLOAD_ERROR : FINISH);

	QMPlay2Core.setWorking(false);
}

QIcon DownloaderThread::getIcon()
{
	if (!prefix.isEmpty())
	{
		for (const QMPlay2Extensions *QMPlay2Ext : QMPlay2Extensions::QMPlay2ExtensionsList())
		{
			const QList<QMPlay2Extensions::AddressPrefix> addressPrefixList = QMPlay2Ext->addressPrefixList();
			const int idx = addressPrefixList.indexOf(prefix);
			if (idx > -1)
				return addressPrefixList[idx].icon;
		}
	}
	return QIcon();
}

/**/

Downloader::Downloader(Module &module)
	: m_sets("Downloader")
	, downloadLW(nullptr)
{
	SetModule(module);
}
Downloader::~Downloader()
{
	if (!downloadLW)
		return;

	{
		int count = 0;
		QByteArray data;
		QDataStream stream(&data, QIODevice::WriteOnly);
		for (QTreeWidgetItem *item : downloadLW->findItems(QString(), Qt::MatchContains))
		{
			DownloadItemW *downloadItemW = (DownloadItemW *)downloadLW->itemWidget(item, 0);
			downloadItemW->write(stream);
			++count;
		}
		m_sets.set("Items/Count", count);
		m_sets.set("Items/Data", data.toBase64().constData());
	}

	{
		int count = 0;
		QByteArray data;
		QDataStream stream(&data, QIODevice::WriteOnly);
		for (QAction *act : m_convertsMenu->actions())
		{
			const QString name = act->text();
			const QString command = act->data().toString();
			if (!name.isEmpty() && !command.isEmpty())
			{
				stream << name << command;
				++count;
			}
		}
		m_sets.set("Presets/Count", count);
		m_sets.set("Presets/Data", data.toBase64().constData());
	}
}

void Downloader::init()
{
	dw = new DockWidget;
	dw->setObjectName(DownloaderName);
	dw->setWindowTitle(tr("Downloader"));
	dw->setWidget(this);

	downloadLW = new DownloadListW;
	downloadLW->setHeaderHidden(true);
	downloadLW->setRootIsDecorated(false);
	connect(downloadLW, SIGNAL(itemDoubleClicked(QTreeWidgetItem *, int)), this, SLOT(itemDoubleClicked(QTreeWidgetItem *)));

	m_convertsMenu = new QMenu(this);
	connect(m_convertsMenu->addAction(tr("&Add")), &QAction::triggered, this, &Downloader::addConvertPreset);
	m_convertsMenu->addSeparator();

	setDownloadsDirB = new QToolButton;
	setDownloadsDirB->setIcon(QMPlay2Core.getIconFromTheme("folder-open"));
	setDownloadsDirB->setText(tr("Download directory"));
	setDownloadsDirB->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	setDownloadsDirB->setToolTip(tr("Choose directory for downloaded files"));
	connect(setDownloadsDirB, SIGNAL(clicked()), this, SLOT(setDownloadsDir()));

	clearFinishedB = new QToolButton;
	clearFinishedB->setIcon(QMPlay2Core.getIconFromTheme("archive-remove"));
	clearFinishedB->setToolTip(tr("Clear completed downloads"));
	connect(clearFinishedB, SIGNAL(clicked()), this, SLOT(clearFinished()));

	addUrlB = new QToolButton;
	addUrlB->setIcon(QMPlay2Core.getIconFromTheme("folder-new"));
	addUrlB->setToolTip(tr("Enter the address for download"));
	connect(addUrlB, SIGNAL(clicked()), this, SLOT(addUrl()));

	m_convertsPresetsB = new QToolButton;
	m_convertsPresetsB->setIcon(QMPlay2Core.getIconFromTheme("list-add"));
	m_convertsPresetsB->setToolTip(tr("Add, modify, or remove conversion presets"));
	m_convertsPresetsB->setPopupMode(QToolButton::InstantPopup);
	m_convertsPresetsB->setMenu(m_convertsMenu);

	layout = new QGridLayout(this);
	layout->setMargin(0);
	layout->addWidget(downloadLW, 0, 0, 1, 6);
	layout->addWidget(setDownloadsDirB, 1, 0, 1, 1);
	layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum), 1, 1, 1, 1);
	layout->addWidget(clearFinishedB, 1, 2, 1, 1);
	layout->addWidget(addUrlB, 1, 3, 1, 1);
	layout->addItem(new QSpacerItem(10, 0, QSizePolicy::Fixed, QSizePolicy::Minimum), 1, 4, 1, 1);
	layout->addWidget(m_convertsPresetsB, 1, 5, 1, 1);

	QString defDownloadPath = QStandardPaths::standardLocations(QStandardPaths::DownloadLocation).value(0, QDir::homePath());
#ifdef Q_OS_WIN
	defDownloadPath.replace('\\', '/');
#endif
	downloadLW->downloadsDirPath = Functions::cleanPath(m_sets.getString("DownloadsDirPath", defDownloadPath));

	// Compatibility
	{
		const int count = m_sets.getInt("Count");
		if (count > 0)
		{
			const QByteArray data = m_sets.getByteArray("Items");
			m_sets.remove("Count");
			m_sets.remove("Items");
			m_sets.set("Items/Count", count);
			m_sets.set("Items/Data", data.toBase64().constData());
		}
	}
	// Items
	{
		const int count = m_sets.getInt("Items/Count");
		if (count > 0)
		{
			QDataStream stream(QByteArray::fromBase64(m_sets.getByteArray("Items/Data")));
			for (int i = 0; i < count; ++i)
				new DownloaderThread(&stream, QString(), downloadLW, m_convertsMenu);
			downloadLW->setCurrentItem(downloadLW->invisibleRootItem()->child(0));
		}
	}
	// Presets
	{
		const auto createPreset = [this](const QString &name, const QString &data) {
			QAction *act = m_convertsMenu->addAction(name);
			act->setData(data);
			connect(act, &QAction::triggered, this, &Downloader::editConvertAction);
			return act;
		};

		const int count = m_sets.getInt("Presets/Count");
		bool presetsRestored = false;
		if (count > 0)
		{
			QDataStream stream(QByteArray::fromBase64(m_sets.getByteArray("Presets/Data")));
			for (int i = 0; i < count; ++i)
			{
				QString name, data;
				stream >> name >> data;
				if (name.isEmpty() || data.isEmpty())
					continue;
				createPreset(name, data);
				presetsRestored = true;
			}

		}
		if (!presetsRestored)
		{
			// Create default presets
			createPreset("MP3 224k", g_defaultMp3ConvertCommand);
			createPreset("OGG container", "ffmpeg -i <input/> -vn -sn -c:a copy -f ogg -y <output>%f.ogg</output>");
		}
	}
}

DockWidget *Downloader::getDockWidget()
{
	return dw;
}

QVector<QAction *> Downloader::getActions(const QString &name, double, const QString &url, const QString &prefix, const QString &param)
{
	if (url.startsWith("file://"))
		return {};
	for (Module *module : QMPlay2Core.getPluginsInstance())
		for (const Module::Info &mod : module->getModulesInfo())
			if (mod.type == Module::DEMUXER && mod.name == prefix)
				return {};

	const auto createAction = [&](const QString &actionName, const QString &preset) {
		QAction *act = new QAction(actionName, nullptr);
		act->setIcon(QIcon(":/downloader.svgz"));
		act->connect(act, &QAction::triggered, this, &Downloader::download);
		act->setProperty("name", name);
		if (!prefix.isEmpty())
		{
			act->setProperty("prefix", prefix);
			act->setProperty("param", param);
		}
		act->setProperty("url", url);
		if (!preset.isEmpty())
			act->setProperty("preset", preset);
		return act;
	};

	QVector<QAction *> actions;

	actions += createAction(Downloader::tr("Download"), QString());
	for (QAction *a : m_convertsMenu->actions())
	{
		const QString command = a->data().toString();
		const QString name = a->text();
		if (command.isEmpty() || name.isEmpty())
			continue;
		actions += createAction(Downloader::tr("Download and convert to \"%1\"").arg(name), name);
	}

	return actions;
}

void Downloader::addConvertPreset()
{
	QAction *action = m_convertsMenu->addAction("MP3 224k");
	action->setData(g_defaultMp3ConvertCommand);
	if (modifyConvertAction(action, false))
		connect(action, &QAction::triggered, this, &Downloader::editConvertAction);
	else
		action->deleteLater();
}
void Downloader::editConvertAction()
{
	if (QAction *action = qobject_cast<QAction *>(sender()))
		modifyConvertAction(action);
}
bool Downloader::modifyConvertAction(QAction *action, bool addRemoveButton)
{
	QDialog dialog(this);
	dialog.setWindowTitle(tr("Converter preset"));

	QLineEdit *nameE = new QLineEdit(action->text());
	QLineEdit *commandE = new QLineEdit(action->data().toString());

	commandE->setToolTip(tr("Command line to execute after download.\n\n<input/> - specifies downloaded file.\n<output>%f.mp3</output> - converted file will be input file with \"mp3\" extension."));

	QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
	connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
	if (addRemoveButton)
	{
		QPushButton *removeB = buttons->addButton(tr("Remove"), QDialogButtonBox::DestructiveRole);
		removeB->setIcon(QMPlay2Core.getIconFromTheme("list-remove"));
		connect(buttons, &QDialogButtonBox::clicked, &dialog, [&](QAbstractButton *button) {
			if (button == removeB)
			{
				action->deleteLater();
				dialog.reject();
			}
		});
	}

	QFormLayout *layout = new QFormLayout(&dialog);
	layout->setMargin(4);
	layout->setSpacing(4);
	layout->addRow(tr("Preset name"), nameE);
	layout->addRow(tr("Command line"), commandE);
	layout->addRow(buttons);

	if (QWindow *win = window()->windowHandle())
	{
		if (QScreen *screen = win->screen())
			dialog.resize(screen->availableGeometry().width() / 2, 1);
	}

	while (dialog.exec() == QDialog::Accepted)
	{
		const QString name = nameE->text().simplified();
		const QString command = commandE->text();

		if (name.isEmpty() || !command.contains(' '))
		{
			QMessageBox::warning(this, dialog.windowTitle(), tr("Incorrect/empty name or command line!"));
			continue;
		}

		if (!command.contains("<input/>"))
		{
			QMessageBox::warning(this, dialog.windowTitle(), tr("Command must contain <input/> tag!"));
			continue;
		}
		if (getCommandOutput(command).isEmpty())
		{
			QMessageBox::warning(this, dialog.windowTitle(), tr("Command must contain correct <output>file</output/> tag!"));
			continue;
		}

		const QList<QAction *> actions = m_convertsMenu->actions();
		bool ok = true;
		for (int i = 1; i < actions.count(); ++i) // Skip first "Add" action
		{
			if (actions[i] != action && actions[i]->text().compare(name, Qt::CaseInsensitive) == 0)
			{
				ok = false;
				break;
			}
		}

		if (!ok)
		{
			QMessageBox::warning(this, dialog.windowTitle(), tr("Preset name already exists!"));
			continue;
		}

		action->setText(name);
		action->setData(command);

		return true;
	}

	return false;
}

void Downloader::setDownloadsDir()
{
	QFileInfo dir(QFileDialog::getExistingDirectory(this, tr("Choose directory for downloaded files"), downloadLW->downloadsDirPath));
#ifndef Q_OS_WIN
	if (dir.isDir() && dir.isWritable())
#else
	if (dir.isDir())
#endif
	{
		downloadLW->downloadsDirPath = Functions::cleanPath(dir.filePath());
		m_sets.set("DownloadsDirPath", downloadLW->downloadsDirPath);
	}
	else if (dir.filePath() != QString())
		QMessageBox::warning(this, DownloaderName, tr("Cannot change downloading files directory"));
}
void Downloader::clearFinished()
{
	const QList<QTreeWidgetItem *> items = downloadLW->findItems(QString(), Qt::MatchContains);
	for (int i = items.count() - 1; i >= 0; --i)
		if (((DownloadItemW *)downloadLW->itemWidget(items[i], 0))->isFinished())
			delete items[i];
}
void Downloader::addUrl()
{
	QString clipboardUrl;
	const QMimeData *mime = QApplication::clipboard()->mimeData();
	if (mime && mime->hasText())
	{
		clipboardUrl = mime->text();
		if (clipboardUrl.contains('\n') || Functions::getUrlScheme(clipboardUrl) != "http")
			clipboardUrl.clear();
	}
	QString url = QInputDialog::getText(this, DownloaderName, tr("Enter address"), QLineEdit::Normal, clipboardUrl);
	if (!url.isEmpty())
		new DownloaderThread(nullptr, url, downloadLW, m_convertsMenu);
}
void Downloader::download()
{
	QAction *action = qobject_cast<QAction *>(sender());
	Q_ASSERT(action);
	new DownloaderThread
	(
		nullptr,
		action->property("url").toString(),
		downloadLW,
		m_convertsMenu,
		action->property("name").toString(),
		action->property("prefix").toString(),
		action->property("param").toString(),
		action->property("preset").toString()
	);
	downloadLW->setCurrentItem(downloadLW->invisibleRootItem()->child(0));
}
void Downloader::itemDoubleClicked(QTreeWidgetItem *item)
{
	DownloadItemW *downloadItemW = (DownloadItemW *)downloadLW->itemWidget(item, 0);
	if (!downloadItemW->getFilePath().isEmpty())
		emit QMPlay2Core.processParam("open", downloadItemW->getFilePath());
}
