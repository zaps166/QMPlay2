// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <GUIHeaders.hpp>

#include <QMPlay2Core.hpp>

class VideoAdjustmentW;
class ShortcutHandler;
class ScreenSaver;
class IPCServer;
class VideoDock;
class MenuBar;

class QMPlay2GUIClass final : private QMPlay2CoreClass
{
    Q_DECLARE_TR_FUNCTIONS(QMPlay2GUIClass)

public:
    static QMPlay2GUIClass &instance();

    static QString getPipe();
    static void saveCover(QByteArray cover);

    static void setTreeWidgetItemIcon(QTreeWidgetItem *tWI, const QIcon &icon, const int column = 0, QTreeWidget *treeWidget = nullptr);

#ifdef UPDATER
    void runUpdate(const QString &);
#endif

    void setStyle();

    void loadIcons();
    void deleteIcons();

    QString getCurrentPth(QString pth = QString(), bool leaveFilename = false);
    void setCurrentPth(const QString &);

    void restoreGeometry(const QString &pth, QWidget *w, const int defaultSizePercent);

    inline QIcon getIcon(const QIcon &icon)
    {
        return icon.isNull() ? getQMPlay2Icon() : icon;
    }

    void updateInDockW();

    QWidget *getVideoDock() const override;
    QWidget *getMainWindow() const override;

    QColor grad1, grad2, qmpTxt;
    QIcon *groupIcon, *mediaIcon, *folderIcon;

    MenuBar *menuBar;
    QWidget *mainW;
    std::unique_ptr<IPCServer> pipe;
    ScreenSaver *screenSaver;
    VideoAdjustmentW *videoAdjustment;
    ShortcutHandler *shortcutHandler;

    bool restartApp, removeSettings, noAutoPlay, startInvisible;
    QString newProfileName, cmdLineProfile;
private:
    QMPlay2GUIClass();
    ~QMPlay2GUIClass();
};

#define QMPlay2GUI \
    QMPlay2GUIClass::instance()
