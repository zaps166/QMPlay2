// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <IOController.hpp>
#include <ModuleCommon.hpp>
#include <DockWidget.hpp>

class QMPLAY2SHAREDLIB_EXPORT QMPlay2Extensions : public ModuleCommon
{
public:
    static void openExtensions();
    static inline const QList<QMPlay2Extensions *> QMPlay2ExtensionsList()
    {
        return guiExtensionsList;
    }
    static void closeExtensions();

    class AddressPrefix
    {
    public:
        inline AddressPrefix(const QString &prefix, const QIcon &icon = QIcon()) :
            prefix(prefix),
            icon(icon)
        {}

        inline bool operator ==(const AddressPrefix &other)
        {
            return other.prefix == prefix;
        }
        inline operator QString() const
        {
            return prefix;
        }

        QString prefix;
        QIcon icon;
    };

    virtual DockWidget *getDockWidget();

    virtual bool canConvertAddress() const;

    virtual QString matchAddress(const QString &url) const;
    virtual QList<AddressPrefix> addressPrefixList(bool img = true) const;
    virtual void convertAddress(const QString &, const QString &, const QString &, QString *, QString *, QIcon *, QString *extension, IOController<> *ioCtrl);

    virtual QVector<QAction *> getActions(const QString &name, double length, const QString &url, const QString &prefix = QString(), const QString &param = QString());

    virtual QMenu *getTrayMenu();
    virtual void ensureTrayMenu();

    virtual bool isVisualization() const;
    virtual void connectDoubleClick(const QObject *, const char *);
    virtual void visState(bool, uchar chn = 0, uint srate = 0);
    virtual void sendSoundData(const QByteArray &);
    virtual void clearSoundData();

protected:
    virtual void init(); //Jeżeli jakieś rozszerzenie używa innego podczas inicjalizacji. Wywoływane po załadowaniu wszystkich.

private:
    static QList<QMPlay2Extensions *> guiExtensionsList;
};
