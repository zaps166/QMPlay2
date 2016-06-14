#include <AboutWidget.hpp>

#include <Settings.hpp>
#include <Main.hpp>

#include <QFile>
#include <QLabel>
#include <QTabWidget>
#include <QPushButton>
#include <QGridLayout>
#include <QMouseEvent>
#include <QMessageBox>

void CommunityEdit::mouseMoveEvent(QMouseEvent *e)
{
	if (!anchorAt(e->pos()).isEmpty())
		viewport()->setProperty("cursor", QVariant(QCursor(Qt::PointingHandCursor)));
	else
		viewport()->setProperty("cursor", QVariant(QCursor(Qt::ArrowCursor)));
	QTextEdit::mouseMoveEvent(e);
}
void CommunityEdit::mousePressEvent(QMouseEvent *e)
{
	if (e->buttons() & Qt::LeftButton)
	{
		QString anchor = anchorAt(e->pos());
		if (!anchor.isEmpty())
			QMPlay2Core.run(anchor);
	}
	QTextEdit::mousePressEvent(e);
}

/**/

AboutWidget::AboutWidget()
{
	setWindowTitle(tr("About QMPlay2"));
	setAttribute(Qt::WA_DeleteOnClose);

	QLabel *label = new QLabel;
	QString labelText = "<B>QMPlay2</B> - " + tr("video player") + "<BR/><B>" + tr("Version") + ":</B> " + QMPlay2Core.getSettings().getString("Version");
	label->setText(labelText);

	QLabel *iconL = new QLabel;
	iconL->setPixmap(QMPlay2Core.getQMPlay2Pixmap());

	QPalette palette;
	palette.setBrush(QPalette::Base, palette.window());

	QTabWidget *tabW = new QTabWidget;

	logE = new QTextEdit;
	logE->setPalette(palette);
	logE->setFrameShape(QFrame::NoFrame);
	logE->setFrameShadow(QFrame::Plain);
	logE->setReadOnly(true);
	logE->setLineWrapMode(QTextEdit::NoWrap);
	logE->viewport()->setProperty("cursor", QVariant(QCursor(Qt::ArrowCursor)));
	tabW->addTab(logE, tr("Logs"));

	dE = new CommunityEdit;
	dE->setMouseTracking(true);
	dE->setPalette(palette);
	dE->setFrameShape(QFrame::NoFrame);
	dE->setFrameShadow(QFrame::Plain);
	dE->setReadOnly(true);
	dE->setLineWrapMode(QTextEdit::NoWrap);
	dE->viewport()->setProperty("cursor", QVariant(QCursor(Qt::ArrowCursor)));
	tabW->addTab(dE, tr("Software creator"));

	clE = new QTextEdit;
	clE->setPalette(palette);
	clE->setFrameShape(QFrame::NoFrame);
	clE->setFrameShadow(QFrame::Plain);
	clE->setReadOnly(true);
	clE->viewport()->setProperty("cursor", QVariant(QCursor(Qt::ArrowCursor)));
	tabW->addTab(clE, tr("Change log"));


	clrLogB = new QPushButton;
	clrLogB->setText(tr("Clear log"));

	QPushButton *closeB = new QPushButton;
	closeB->setText(tr("Close"));
	closeB->setShortcut(QKeySequence("ESC"));

	QGridLayout *layout = new QGridLayout;
	layout->setContentsMargins(2, 2, 2, 2);
	layout->addWidget(iconL, 0, 0, 1, 1);
	layout->addWidget(label, 0, 1, 1, 2);
	layout->addWidget(tabW, 1, 0, 1, 3);
	layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum), 2, 0, 1, 1); //hSpacer
	layout->addWidget(clrLogB, 2, 1, 1, 1);
	layout->addWidget(closeB, 2, 2, 1, 1);
	setLayout(layout);

	connect(clrLogB, SIGNAL(clicked()), this, SLOT(clrLog()));
	connect(closeB, SIGNAL(clicked()), this, SLOT(close()));
	connect(tabW, SIGNAL(currentChanged(int)), this, SLOT(currentTabChanged(int)));
	connect(&logWatcher, SIGNAL(fileChanged(const QString &)), this, SLOT(refreshLog()));

	show();
}

void AboutWidget::showEvent(QShowEvent *)
{
	QMPlay2GUI.restoreGeometry("AboutWidget/Geometry", this, QSize(630, 440));
	dE->setHtml
	(
		"<table cellpadding='5' border='0' width='100%'>"
			"<tr>"
				"<td width='0' valign='middle' align='center'><a href='http://qt-apps.org/usermanager/search.php?username=zaps166'></a><B>Błażej Szczygieł</B></td>"
				"<td valign='middle'>" + tr("Creator and developer") + "</td>"
				"<td valign='middle'><B>E-Mail</B>: <a href='mailto:spaz16@wp.pl'>spaz16@wp.pl</a><BR/><B>Web page</B>: <a href='http://zaps166.sf.net/'>http://zaps166.sourceforge.net/</a></td>"
			"</tr>"
		"</table>"
	);
	refreshLog();

	QFile cl(QMPlay2Core.getShareDir() + "ChangeLog");
	if (cl.open(QFile::ReadOnly))
	{
		clE->setText(cl.readAll());
		cl.close();
	}

	QFile f(QMPlay2Core.getLogFilePath());
	if (!f.exists() && f.open(QFile::WriteOnly)) //tworzy pusty plik dziennika, jeżeli nie istnieje
		f.close();
	if (f.exists())
		logWatcher.addPath(QMPlay2Core.getLogFilePath());
}
void AboutWidget::closeEvent(QCloseEvent *)
{
	logWatcher.removePaths(logWatcher.files());
	QMPlay2Core.getSettings().set("AboutWidget/Geometry", geometry());
}

void AboutWidget::refreshLog()
{
	logE->clear();
	QFile f(QMPlay2Core.getLogFilePath());
	if (f.open(QFile::ReadOnly))
	{
		QByteArray data = f.readAll();
		f.close();
		if (data.right(1) == "\n")
			data.chop(1);
		logE->setPlainText(data);
		logE->moveCursor(QTextCursor::End);
	}
}
void AboutWidget::clrLog()
{
	QFile f(QMPlay2Core.getLogFilePath());
	if (f.exists())
	{
		if (f.open(QFile::WriteOnly))
			f.close();
		else
			QMessageBox::warning(this, windowTitle(), tr("Can't clear log"));
	}
}
void AboutWidget::currentTabChanged(int idx)
{
	clrLogB->setVisible(idx == 0);
}
