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

#include <AddressBox.hpp>

#include <QMPlay2Extensions.hpp>
#include <Functions.hpp>
#include <Module.hpp>
#include <Main.hpp>

#include <QGridLayout>

AddressBox::AddressBox(Qt::Orientation o, QString url)
{
	if (url.isNull())
		pB.addItem(QMPlay2Core.getQMPlay2Icon(), tr("Autodetect address"), AUTODETECT);
	pB.addItem(QMPlay2Core.getQMPlay2Icon(), tr("Direct address"), DIRECT);

	for (Module *module : QMPlay2Core.getPluginsInstance())
		for (const Module::Info &mod : module->getModulesInfo())
			if (mod.type == Module::DEMUXER && !mod.name.contains(' '))
			{
				QIcon icon;
				if (!mod.icon.isNull())
					icon = mod.icon;
				else if (!module->icon().isNull())
					icon = module->icon();
				else
					icon = QMPlay2Core.getQMPlay2Icon();
				pB.addItem(icon, mod.name, MODULE);
			}

	for (const QMPlay2Extensions *QMPlay2Ext : QMPlay2Extensions::QMPlay2ExtensionsList())
		for (const QMPlay2Extensions::AddressPrefix &addressPrefix : QMPlay2Ext->addressPrefixList())
			pB.addItem(addressPrefix.icon, addressPrefix, MODULE);

	if (!url.isEmpty())
	{
		QString prefix, param;
		if (Functions::splitPrefixAndUrlIfHasPluginPrefix(url, &prefix, &url, &param))
		{
			pB.setCurrentIndex(pB.findText(prefix));
			pE.setText(param);
		}
		else
		{
			QString scheme = Functions::getUrlScheme(url);
			int idx = pB.findText(scheme);
			if (idx > -1)
			{
				pB.setCurrentIndex(idx);
				url.remove(scheme + "://");
			}
		}
		if (url.startsWith("file://"))
		{
			url.remove(0, 7);
			filePrefix = "file://";
		}
		aE.setText(url);
	}

	connect(&pB, SIGNAL(currentIndexChanged(int)), this, SLOT(pBIdxChanged()));
	connect(&aE, SIGNAL(textEdited(const QString &)), this, SIGNAL(directAddressChanged()));

	pE.setToolTip(tr("Additional module parameter"));
	pE.setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);

	QGridLayout *layout = new QGridLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	switch (o)
	{
		case Qt::Horizontal:
			layout->addWidget(&pB, 0, 0);
			layout->addWidget(&aE, 0, 1);
			layout->addWidget(&pE, 0, 2);
			break;
		case Qt::Vertical:
			layout->addWidget(&pB, 0, 0, 1, 2);
			layout->addWidget(&aE, 1, 0, 1, 1);
			layout->addWidget(&pE, 1, 1, 1, 1);
			break;
	}

	pBIdxChanged();
}

QString AddressBox::url() const
{
	const QString url = filePrefix + cleanUrl();
	switch (currentPrefixType())
	{
		case AddressBox::AUTODETECT:
			return Functions::maybeExtensionAddress(url);
		case AddressBox::DIRECT:
			return url;
		case AddressBox::MODULE:
			return pB.currentText() + "://{" + url + "}" + pE.text();
	}
	return QString();
}
QString AddressBox::cleanUrl() const
{
	return aE.text();
}

void AddressBox::pBIdxChanged()
{
	const PrefixType prefixType = currentPrefixType();
	pE.setVisible(prefixType != AUTODETECT && prefixType != DIRECT);
	emit directAddressChanged();
}
