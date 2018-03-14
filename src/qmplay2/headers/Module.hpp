/*
	QMPlay2 is a video and audio player.
	Copyright (C) 2010-2018  Błażej Szczygieł

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

#include <QStringList>
#include <QWidget>
#include <QMutex>

#include <CppUtils.hpp>
#include <Settings.hpp>

class ModuleCommon;

class QMPLAY2SHAREDLIB_EXPORT Module : public Settings
{
	friend class ModuleCommon;
public:
	enum TYPE {NONE, DEMUXER, DECODER, READER, WRITER, PLAYLIST, QMPLAY2EXTENSION, SUBSDEC, AUDIOFILTER, VIDEOFILTER};
	enum FILTERTYPE {DEINTERLACE = 0x400000, DOUBLER = 0x800000, USERFLAG = 0x80000000};
	enum HWTYPE {VIDEOHWFILTER = 0x4000000};

	inline Module(const QString &mName) :
		Settings(mName),
		mName(mName)
	{}
	virtual ~Module() = default;

	inline QString name() const
	{
		return mName;
	}

	class Info
	{
	public:
		Info() = default;
		inline Info(const QString &name, const quint32 type, const QIcon &icon = QIcon(), const QString &description = QString()) :
			name(name), description(description), type(type), icon(icon)
		{}
		inline Info(const QString &name, const quint32 type, const QString &description) :
			name(name), description(description), type(type)
		{}
		inline Info(const QString &name, const quint32 type, const QStringList &extensions, const QIcon &icon = QIcon(), const QString &description = QString()) :
			name(name), description(description), type(type), icon(icon), extensions(extensions)
		{}

		QString name, description;
		quint32 type = NONE;
		QIcon icon;
		QStringList extensions;
	};
	virtual QList<Info> getModulesInfo(const bool showDisabled = false) const = 0;
	virtual void *createInstance(const QString &) = 0;

	virtual QList<QAction *> getAddActions();

	class SettingsWidget : public QWidget
	{
	public:
		virtual void saveSettings() = 0;
		inline void flushSettings()
		{
			sets().flush();
		}
	protected:
		inline SettingsWidget(Module &module) :
			module(module)
		{
			setAttribute(Qt::WA_DeleteOnClose);
		}

		inline Settings &sets()
		{
			return module;
		}

		template<typename T>
		inline void SetInstance()
		{
			module.setInstance<T>();
		}
	private:
		Module &module;
	};
	virtual SettingsWidget *getSettingsWidget();

	virtual void videoDeintSave();

	inline QIcon icon() const
	{
		return m_icon;
	}

	void setInstances(bool &);

	template<typename T>
	void setInstance();

protected:
	QIcon m_icon;

private:
	QMutex mutex;
	QString mName;
	QList<ModuleCommon *> instances;
};

template<typename T>
void Module::setInstance()
{
	QMutexLocker locker(&mutex);
	for (ModuleCommon *mc : asConst(instances))
	{
		T *t = dynamic_cast<T *>(mc);
		if (t)
			t->set();
	}
}

/**/

#define QMPLAY2_MODULES_API_VERSION 8

#define QMPLAY2_EXPORT_MODULE(ModuleClass) \
	extern "C" Q_DECL_EXPORT quint32 getQMPlay2ModuleAPIVersion() \
	{ \
		return (QT_VERSION << 8) | QMPLAY2_MODULES_API_VERSION; \
	} \
	extern "C" Q_DECL_EXPORT Module *createQMPlay2ModuleInstance() \
	{ \
		return new ModuleClass; \
	}
