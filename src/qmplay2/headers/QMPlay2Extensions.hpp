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

#include <IOController.hpp>
#include <ModuleCommon.hpp>
#include <DockWidget.hpp>

#include <QString>
#include <QImage>

class QMPLAY2SHAREDLIB_EXPORT QMPlay2Extensions : public ModuleCommon
{
public:
	static void openExtensions();
	static inline QList<QMPlay2Extensions *> QMPlay2ExtensionsList()
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

	virtual QList<AddressPrefix> addressPrefixList(bool img = true) const;
	virtual void convertAddress(const QString &, const QString &, const QString &, QString *, QString *, QIcon *, QString *extension, IOController<> *ioCtrl);

	virtual QVector<QAction *> getActions(const QString &name, double length, const QString &url, const QString &prefix = QString(), const QString &param = QString());

	virtual bool isVisualization() const;
	virtual void connectDoubleClick(const QObject *, const char *);
	virtual void visState(bool, uchar chn = 0, uint srate = 0);
	virtual void sendSoundData(const QByteArray &);
	virtual void clearSoundData();
protected:
	virtual ~QMPlay2Extensions() = default;

	virtual void init(); //Jeżeli jakieś rozszerzenie używa innego podczas inicjalizacji. Wywoływane po załadowaniu wszystkich.
private:
	static QList<QMPlay2Extensions *> guiExtensionsList;
};
