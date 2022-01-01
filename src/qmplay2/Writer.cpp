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

#include <Writer.hpp>

#include <Functions.hpp>

#include <QSaveFile>
#include <QBuffer>

#include <memory>

class IODeviceWriter : public Writer
{
    bool readyWrite() const override final
    {
        return m_io->isOpen();
    }

    qint64 write(const QByteArray &arr) override final
    {
        return m_io->write(arr);
    }

protected:
    bool open() override
    {
        return m_io->open(QIODevice::WriteOnly);
    }

    std::unique_ptr<QIODevice> m_io;
};

/**/

class QMPlay2FileWriter : public IODeviceWriter
{
    ~QMPlay2FileWriter()
    {
        if (auto f = static_cast<QSaveFile *>(m_io.get()))
            f->commit();
    }

    QString name() const override final
    {
        return "File Writer";
    }

    bool open() override final
    {
        m_io.reset(new QSaveFile(getUrl().mid(7)));
        return IODeviceWriter::open();
    }
};

/**/

class QMPlay2ResourceWriter : public IODeviceWriter
{
    ~QMPlay2ResourceWriter()
    {
        if (m_io)
            m_io->close();
        QMPlay2Core.addResource(getUrl(), m_data);
    }

    QString name() const override final
    {
        return "Resource Writer";
    }

    bool open() override final
    {
        m_io.reset(new QBuffer(&m_data));
        return IODeviceWriter::open();
    }

    QByteArray m_data;
};

/**/

Writer *Writer::create(const QString &url, const QStringList &modNames)
{
    const QString scheme = Functions::getUrlScheme(url);
    if (url.isEmpty() || scheme.isEmpty())
        return nullptr;
    Writer *writer = nullptr;
    if (modNames.isEmpty())
    {
        if (scheme == "file")
            writer = new QMPlay2FileWriter;
        else if (scheme == "QMPlay2")
            writer = new QMPlay2ResourceWriter;
        if (writer)
        {
            writer->m_url = url;
            if (writer->open())
                return writer;
            delete writer;
            return nullptr;
        }
    }
    QVector<QPair<Module *, Module::Info>> pluginsInstances(modNames.count());
    for (Module *pluginInstance : QMPlay2Core.getPluginsInstance())
        for (const Module::Info &mod : pluginInstance->getModulesInfo())
            if (mod.type == Module::WRITER && mod.extensions.contains(scheme))
            {
                if (modNames.isEmpty())
                    pluginsInstances += {pluginInstance, mod};
                else
                {
                    const int idx = modNames.indexOf(mod.name);
                    if (idx > -1)
                        pluginsInstances[idx] = {pluginInstance, mod};
                }
            }
    for (int i = 0; i < pluginsInstances.count(); i++)
    {
        Module *module = pluginsInstances[i].first;
        Module::Info &moduleInfo = pluginsInstances[i].second;
        if (!module || moduleInfo.name.isEmpty())
            continue;
        writer = (Writer *)module->createInstance(moduleInfo.name);
        if (!writer)
            continue;
        writer->m_url = url;
        if (writer->open())
            return writer;
        delete writer;
    }
    return nullptr;
}
