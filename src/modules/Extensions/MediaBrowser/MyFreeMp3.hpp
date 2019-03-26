/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2019  Błażej Szczygieł

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

#pragma once

#include <MediaBrowser/Common.hpp>

#include <QCoreApplication>

class MyFreeMP3 final : public MediaBrowserCommon
{
    Q_DECLARE_TR_FUNCTIONS(MyFreeMP3)

public:
    MyFreeMP3(NetworkAccess &net);
    ~MyFreeMP3();

    void prepareWidget(QTreeWidget *treeW) override;

    void finalize() override;

    QString getQMPlay2Url(const QString &text) const override;

    NetworkReply *getSearchReply(const QString &text, const qint32 page) override;
    Description addSearchResults(const QByteArray &reply, QTreeWidget *treeW) override;

    PagesMode pagesMode() const override;

    bool hasWebpage() const override;
    QString getWebpageUrl(const QString &text) const override;

    CompleterMode completerMode() const override;
    NetworkReply *getCompleterReply(const QString &text) override;
    QStringList getCompletions(const QByteArray &reply) override;


    QAction *getAction() const override;

    bool convertAddress(const QString &prefix, const QString &url, const QString &param, QString *streamUrl, QString *name, QIcon *icon, QString *extension, IOController<> *ioCtrl) override;

private:
    QString encode(int input);;

private:
    QStringList m_urlNames;
    QString m_streamHost;
};
