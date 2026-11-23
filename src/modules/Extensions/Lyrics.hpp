// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <ExtensionsHeaders.hpp>

#include <QMPlay2Extensions.hpp>
#include <NetworkAccess.hpp>

class Lyrics final : public QTextEdit, public QMPlay2Extensions
{
    Q_OBJECT

public:
    Lyrics(Module &module);
    ~Lyrics();

    DockWidget *getDockWidget() override;

private slots:
    void visibilityChanged(bool v);

private:
    void updatePlaying(bool play, const QString &title, const QString &artist, const QString &album, int length, bool needCover, const QString &fileName, const QString &url, const QString &lyrics);

private slots:
    void finished(NetworkReply *reply);

private:
    bool loadLrcFile(const QString &url);

    void search();

    void lyricsNotFound();

private:
    DockWidget *m_dW;

    bool m_visible, m_pending;

    NetworkAccess m_net;

    QString m_title, m_artist, m_name;
    bool m_titleArtistFromDash = false;

    QPointer<NetworkReply> m_tekstowoSearchReply;
    QPointer<NetworkReply> m_tekstowoLyricsReply;
};

#define LyricsName "Lyrics"
