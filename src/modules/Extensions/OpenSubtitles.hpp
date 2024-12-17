/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2024  Błażej Szczygieł

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

#include <QMPlay2Extensions.hpp>

#include <QPointer>

class QTreeWidgetItem;
class NetworkAccess;
class NetworkReply;
class QTreeWidget;
class QComboBox;
class LineEdit;
class QTimer;

class OpenSubtitles final : public QWidget, public QMPlay2Extensions
{
    Q_OBJECT

    struct Subtitle;

public:
    OpenSubtitles(Module &);
    ~OpenSubtitles();

    DockWidget *getDockWidget() override;

    QVector<QAction *> getActions(const QString &name, double, const QString &url, const QString &, const QString &) override;

private:
    void parseCompleterJson(const QByteArray &data);
    void parseXml(const QByteArray &data, QTreeWidgetItem *twi = nullptr);

    void complete();

    void search();
    void searchNext();
    void loadSubItem(const QString &url, QTreeWidgetItem *twi);

    inline void clearCompleter();

    inline QString getLanguage() const;

private:
    DockWidget *const m_dw;

    NetworkAccess *const m_net;

    LineEdit *const mSearchEdit;
    QPointer<NetworkReply> m_completerReply;
    QTimer *const m_completerTimer;
    bool mDontComplete = false;

    QComboBox *const mLanguages;

    QTreeWidget *const mResults;
    QPointer<NetworkReply> m_searchReply;
    QVector<NetworkReply *> m_loadSubItemReplies;
    QVector<NetworkReply *> m_loadThumbnailReplies;

    QString mToUrl;
};

#define OpenSubtitlesName "OpenSubtitles"
