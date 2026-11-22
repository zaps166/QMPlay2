// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QMPlay2Extensions.hpp>

#include <QPointer>

class QTreeWidgetItem;
class NetworkAccess;
class NetworkReply;
class QComboBox;
class LineEdit;
class QTimer;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
class TreeWidget;
#else
class QTreeWidget;
using TreeWidget = QTreeWidget;
#endif

class OpenSubtitles final : public QWidget, public QMPlay2Extensions
{
    Q_OBJECT

    struct Subtitle;

public:
    OpenSubtitles(Module &module, const QIcon &icon);
    ~OpenSubtitles();

    DockWidget *getDockWidget() override;

    QVector<QAction *> getActions(const QString &name, double, const QString &url, const QString &, const QString &) override;

private:
    void parseCompleterJson(const QByteArray &data);
    void parseXml(const QByteArray &data, QTreeWidgetItem *parentTwi = nullptr);

    void complete();

    void scanForToUrl();

    void search();
    void searchNext();
    void loadSubItem(const QString &url, const QPersistentModelIndex &index, bool next);

    void setBusyCursor(bool set);

    inline void clearCompleter();

    inline QString getLanguage() const;

private:
    const QIcon m_icon;

    DockWidget *const m_dw;

    NetworkAccess *const m_net;

    int m_busyCursor = 0;

    LineEdit *const m_searchEdit;
    QPointer<NetworkReply> m_completerReply;
    QTimer *const m_completerTimer;
    bool m_dontComplete = false;

    QComboBox *const m_languages;

    TreeWidget *const m_results;
    QPointer<NetworkReply> m_searchReply;
    QVector<NetworkReply *> m_loadSubItemReplies;
    QVector<NetworkReply *> m_loadThumbnailReplies;

    QString m_toUrl;
};

#define OpenSubtitlesName "OpenSubtitles"
