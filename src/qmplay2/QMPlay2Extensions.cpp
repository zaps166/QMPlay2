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

#include <QMPlay2Extensions.hpp>

QList<QMPlay2Extensions *> QMPlay2Extensions::guiExtensionsList;

void QMPlay2Extensions::openExtensions()
{
    if (!guiExtensionsList.isEmpty())
        return;
    for (Module *module : QMPlay2Core.getPluginsInstance())
        for (const Module::Info &mod : module->getModulesInfo())
            if (mod.type == Module::QMPLAY2EXTENSION)
            {
                QMPlay2Extensions *QMPlay2Ext = (QMPlay2Extensions *)module->createInstance(mod.name);
                if (QMPlay2Ext)
                    guiExtensionsList.append(QMPlay2Ext);
            }
    for (QMPlay2Extensions *QMPlay2Ext : qAsConst(guiExtensionsList))
        QMPlay2Ext->init();
}

void QMPlay2Extensions::closeExtensions()
{
    while (!guiExtensionsList.isEmpty())
        delete guiExtensionsList.takeFirst();
}

DockWidget *QMPlay2Extensions::getDockWidget()
{
    return nullptr;
}

bool QMPlay2Extensions::canConvertAddress() const
{
    return false;
}

QString QMPlay2Extensions::matchAddress(const QString &url) const
{
    Q_UNUSED(url);
    return QString();
}
QList<QMPlay2Extensions::AddressPrefix> QMPlay2Extensions::addressPrefixList(bool img) const
{
    Q_UNUSED(img)
    return {};
}
void QMPlay2Extensions::convertAddress(const QString &, const QString &, const QString &, QString *, QString *, QIcon *, QString *extension, IOController<> *ioCtrl)
{
    Q_UNUSED(extension)
    Q_UNUSED(ioCtrl)
}

QVector<QAction *> QMPlay2Extensions::getActions(const QString &name, double length, const QString &url, const QString &prefix, const QString &param)
{
    Q_UNUSED(name)
    Q_UNUSED(length)
    Q_UNUSED(url)
    Q_UNUSED(prefix)
    Q_UNUSED(param)
    return {};
}

bool QMPlay2Extensions::isVisualization() const
{
    return false;
}
void QMPlay2Extensions::connectDoubleClick(const QObject *, const char *)
{}
void QMPlay2Extensions::visState(bool, uchar chn, uint srate)
{
    Q_UNUSED(chn)
    Q_UNUSED(srate)
}
void QMPlay2Extensions::sendSoundData(const QByteArray &)
{}
void QMPlay2Extensions::clearSoundData()
{}

void QMPlay2Extensions::init()
{}
