/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

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

#include <AboutWidget.hpp>

#include <Functions.hpp>
#include <Settings.hpp>
#include <Main.hpp>

#include <QUrl>
#include <QFile>
#include <QLabel>
#include <QSysInfo>
#include <QTabWidget>
#include <QPushButton>
#include <QGridLayout>
#include <QMouseEvent>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QDesktopServices>

AboutWidget::AboutWidget()
{
    setWindowTitle(tr("About QMPlay2"));
    setAttribute(Qt::WA_DeleteOnClose);

    QString labelText;
    labelText += "<b>QMPlay2:</b> " + tr("video and audio player");
    labelText += "<br/><b>" + tr("Programmer") + ":</b> <a href='mailto:spaz16@wp.pl'>Błażej Szczygieł</a>";
    labelText += "<br/><b>" + tr("Version") + ":</b> " + QMPlay2Core.getSettings().getString("Version") + " (" + QSysInfo::buildCpuArchitecture() + ")";
    QLabel *label = new QLabel(labelText);

    QLabel *iconL = new QLabel;
    iconL->setPixmap(Functions::getPixmapFromIcon(QMPlay2Core.getQMPlay2Icon(), QSize(128, 128), this));

    QPalette palette;
    palette.setBrush(QPalette::Base, palette.window());

    QTabWidget *tabW = new QTabWidget;

    const QFont font(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    logE = new QPlainTextEdit;
    logE->setFont(font);
    logE->setPalette(palette);
    logE->setFrameShape(QFrame::NoFrame);
    logE->setFrameShadow(QFrame::Plain);
    logE->setReadOnly(true);
    logE->setLineWrapMode(QPlainTextEdit::NoWrap);
    logE->viewport()->setProperty("cursor", QCursor(Qt::ArrowCursor));
    tabW->addTab(logE, tr("Logs"));

    clE = new QPlainTextEdit;
    clE->setFont(font);
    clE->setPalette(palette);
    clE->setFrameShape(QFrame::NoFrame);
    clE->setFrameShadow(QFrame::Plain);
    clE->setReadOnly(true);
    clE->viewport()->setProperty("cursor", QCursor(Qt::ArrowCursor));
    tabW->addTab(clE, tr("Change log"));

    auE = new QPlainTextEdit;
    auE->setFont(font);
    auE->setPalette(palette);
    auE->setFrameShape(QFrame::NoFrame);
    auE->setFrameShadow(QFrame::Plain);
    auE->setReadOnly(true);
    auE->viewport()->setProperty("cursor", QCursor(Qt::ArrowCursor));
    tabW->addTab(auE, tr("Contributors"));

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

    connect(label, SIGNAL(linkActivated(const QString &)), this, SLOT(linkActivated(const QString &)));
    connect(clrLogB, SIGNAL(clicked()), this, SLOT(clrLog()));
    connect(closeB, SIGNAL(clicked()), this, SLOT(close()));
    connect(tabW, SIGNAL(currentChanged(int)), this, SLOT(currentTabChanged(int)));
    connect(&logWatcher, SIGNAL(fileChanged(const QString &)), this, SLOT(refreshLog()));

    show();
}

void AboutWidget::showEvent(QShowEvent *)
{
    QMPlay2GUI.restoreGeometry("AboutWidget/Geometry", this, 55);

    refreshLog();

    QFile cl(QMPlay2Core.getShareDir() + "ChangeLog");
    if (cl.open(QFile::ReadOnly))
    {
        clE->setPlainText(cl.readAll());
        cl.close();
    }

    QFile au(QMPlay2Core.getShareDir() + "AUTHORS");
    if (au.open(QFile::ReadOnly))
    {
        auE->setPlainText(au.readAll());
        au.close();
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

void AboutWidget::linkActivated(const QString &link)
{
    QDesktopServices::openUrl(link);
}
void AboutWidget::refreshLog()
{
    logE->clear();
    QFile f(QMPlay2Core.getLogFilePath());
    if (f.open(QFile::ReadOnly))
    {
        QByteArray data = f.readAll();
        f.close();
        if (data.endsWith("\n"))
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
