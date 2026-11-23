// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <AddressBox.hpp>

#include <QMPlay2Extensions.hpp>
#include <Functions.hpp>
#include <Module.hpp>
#include <Main.hpp>

AddressBox::AddressBox(Qt::Orientation o, QString url, const QString &choice)
{
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

    const int pBIdx = pB.findText(choice);

    if (url.isNull())
    {
        const auto text = QGuiApplication::clipboard()->text();
        if (text.contains("://") && text.indexOf("://") == text.indexOf(":") && QUrl(text).isValid())
            url = text;
    }

    if (!url.isEmpty())
    {
        QString prefix, param;
        bool hasPrefix = false, isFile = false;
        if (Functions::splitPrefixAndUrlIfHasPluginPrefix(url, &prefix, &url, &param))
        {
            pB.setCurrentIndex(pB.findText(prefix));
            pE.setText(param);
            hasPrefix = true;
        }
        else
        {
            QString scheme = Functions::getUrlScheme(url);
            int idx = pB.findText(scheme);
            if (idx > -1)
            {
                pB.setCurrentIndex(idx);
                url.remove(scheme + "://");
                hasPrefix = true;
            }
        }
        if (url.startsWith("file://"))
        {
            url.remove(0, 7);
            filePrefix = "file://";
            isFile = true;
        }
        else if (!hasPrefix && !url.contains("://"))
        {
            isFile = true;
        }
        aE.setText(isFile ? QDir::toNativeSeparators(url) : url);
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

    if (pBIdx > -1 && pBIdx != pB.currentIndex())
        pB.setCurrentIndex(pBIdx);
    else
        pBIdxChanged();
}

QString AddressBox::url() const
{
    const QString url = filePrefix + cleanUrl();
    switch (currentPrefixType())
    {
        case AddressBox::DIRECT:
            return Functions::maybeExtensionAddress(url);
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
    pE.setVisible(prefixType != DIRECT);
    emit directAddressChanged();
}
