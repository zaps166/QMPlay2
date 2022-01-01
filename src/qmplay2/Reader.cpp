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

#include <Reader.hpp>

#include <Functions.hpp>

#include <QBuffer>
#include <QFile>

#include <memory>

class IODeviceReader : public Reader
{
    bool readyRead() const override final
    {
        return m_io->isOpen();
    }
    bool canSeek() const override final
    {
        return true;
    }

    bool seek(qint64 pos) override final
    {
        return m_io->seek(pos);
    }
    QByteArray read(qint64 len) override final
    {
        return m_io->read(len);
    }
    bool atEnd() const override final
    {
        return m_io->atEnd();
    }

    virtual qint64 size() const override final
    {
        return m_io->size();
    }
    virtual qint64 pos() const override final
    {
        return m_io->pos();
    }

protected:
    bool open() override
    {
        return m_io->open(QIODevice::ReadOnly);
    }

    std::unique_ptr<QIODevice> m_io;
};

/**/

class QMPlay2FileReader : public IODeviceReader
{
    QString name() const override final
    {
        return "File Reader";
    }

    bool open() override final
    {
        m_io.reset(new QFile(getUrl().mid(7)));
        return IODeviceReader::open();
    }
};

/**/

class QMPlay2ResourceReader : public IODeviceReader
{
    QString name() const override final
    {
        return "Resource Reader";
    }

    bool open() override final
    {
        m_data = QMPlay2Core.getResource(getUrl());
        if (!m_data.isNull())
        {
            m_io.reset(new QBuffer(&m_data));
            return IODeviceReader::open();
        }
        return false;
    }

    QByteArray m_data;
};

/**/

bool Reader::create(const QString &url, IOController<Reader> &reader, const QString &plugName)
{
    const QString scheme = Functions::getUrlScheme(url);
    if (reader.isAborted() || url.isEmpty() || scheme.isEmpty())
        return false;
    if (plugName.isEmpty())
    {
        if (scheme == "file")
            reader.assign(new QMPlay2FileReader);
        else if (scheme == "QMPlay2")
            reader.assign(new QMPlay2ResourceReader);
        if (reader)
        {
            reader->_url = url;
            if (reader->open())
                return true;
            reader.reset();
        }
    }
    for (Module *module : QMPlay2Core.getPluginsInstance())
        for (const Module::Info &mod : module->getModulesInfo())
            if (mod.type == Module::READER && mod.extensions.contains(scheme) && (plugName.isEmpty() || mod.name == plugName))
            {
                if (reader.assign((Reader *)module->createInstance(mod.name)))
                {
                    reader->_url = url;
                    if (reader->open())
                        return true;
                    reader.reset();
                }
                if (reader.isAborted())
                    break;
            }
    return false;
}
