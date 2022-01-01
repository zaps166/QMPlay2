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

#include <EntryProperties.hpp>

#ifdef QMPlay2_TagEditor
    #include <TagEditor.hpp>
#endif
#include <PlaylistWidget.hpp>
#include <AddressBox.hpp>
#include <Functions.hpp>
#include <Main.hpp>

#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QToolButton>
#include <QGridLayout>
#include <QMessageBox>
#include <QFileInfo>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QUrl>

EntryProperties::EntryProperties(QWidget *p, QTreeWidgetItem *_tWI, bool &sync, bool &accepted) :
    QDialog(p), sync(sync)
{
    sync = false;
    tWI = _tWI;
    if (!tWI)
        return;

    catalogCB = nullptr;
    browseDirB = browseFileB = openUrlB = nullptr;
    pthE = nullptr;
    addrB = nullptr;
#ifdef QMPlay2_TagEditor
    tagEditor = nullptr;
#endif
    fileSizeL = nullptr;

    setWindowTitle(tr("Properties"));

    QGridLayout layout(this);
    int row = 0;

    nameE = new QLineEdit(tWI->text(0));

    QString url = tWI->data(0, Qt::UserRole).toString();
    if (url.startsWith("file://"))
    {
        url.remove(0, 7);
        url = QDir::toNativeSeparators(url);
    }

    const bool isGroup = PlaylistWidget::isGroup(tWI);

    if (isGroup)
    {
        pthE = new QLineEdit(url);
        origDirPth = url;

        nameE->selectAll();

        catalogCB = new QCheckBox(tr("Synchronize with file or directory"));
        catalogCB->setChecked(!pthE->text().isEmpty());
        connect(catalogCB, SIGNAL(stateChanged(int)), this, SLOT(setDirPthEEnabled(int)));

        browseDirB = new QToolButton;
        browseDirB->setToolTip(tr("Browse for a directory"));
        browseDirB->setIcon(QMPlay2Core.getIconFromTheme("folder-open"));
        connect(browseDirB, SIGNAL(clicked()), this, SLOT(browse()));

        browseFileB = new QToolButton;
        browseFileB->setToolTip(tr("Browse for a file that contains more than one track"));
        browseFileB->setIcon(*QMPlay2GUI.mediaIcon);
        connect(browseFileB, SIGNAL(clicked()), this, SLOT(browse()));

        setDirPthEEnabled(catalogCB->isChecked());

        layout.addWidget(nameE, row++, 0, 1, 3);
        layout.addWidget(catalogCB, row++, 0, 1, 1);
        layout.addWidget(pthE, row, 0, 1, 1);
        layout.addWidget(browseDirB, row, 1, 1, 1);
        layout.addWidget(browseFileB, row, 2, 1, 1);
    }
    else
    {
        layout.addWidget(nameE, row++, 0, 1, 3);

        addrB = new AddressBox(Qt::Horizontal, url);
        layout.addWidget(addrB, row, 0, 1, 2);

        openUrlB = new QToolButton;
        openUrlB->setToolTip(tr("Open URL or directory containing chosen file"));
        openUrlB->setIcon(QMPlay2Core.getIconFromTheme("folder-open"));
        connect(openUrlB, &QToolButton::clicked, this, &EntryProperties::openUrl);
        layout.addWidget(openUrlB, row, 2, 1, 1);

        fileSizeL = new QLabel;

#ifdef QMPlay2_TagEditor
        tagEditor = new TagEditor;
        connect(addrB, SIGNAL(directAddressChanged()), this, SLOT(directAddressChanged()));
        directAddressChanged();
        layout.addWidget(tagEditor, ++row, 0, 1, 3);
#endif
    }

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

#ifdef QMPlay2_TagEditor
    if (!tagEditor)
#endif
        layout.addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), ++row, 0, 1, 2); //vSpacer
    layout.addWidget(buttonBox, ++row, isGroup ? 0 : 1, 1, isGroup ? 3 : 2);
    if (fileSizeL)
        layout.addWidget(fileSizeL, row, 0, 1, 1);
    layout.setSpacing(3);
    layout.setMargin(3);

#ifdef QMPlay2_TagEditor
    resize(625, tagEditor ? 470 : 0);
#else
    resize(625, 0);
#endif

    accepted = exec() == QDialog::Accepted;
}

void EntryProperties::setDirPthEEnabled(int e)
{
    pthE->setEnabled(e);
    browseDirB->setEnabled(e);
    browseFileB->setEnabled(e);
}
#ifdef QMPlay2_TagEditor
void EntryProperties::directAddressChanged()
{
    bool e = (addrB->currentPrefixType() == AddressBox::DIRECT);
    if (e)
    {
        QFileInfo fi(addrB->cleanUrl());
        if (fi.isFile())
        {
            fileSizeL->setText(tr("File size") + ": " + Functions::sizeString(fi.size()));
            nameE->setReadOnly(true);
        }
        else
        {
            fileSizeL->clear();
            nameE->setReadOnly(false);
        }

        e = tagEditor->open(addrB->url());
    }
    if (!e)
        tagEditor->clear();
    tagEditor->setEnabled(e);
}
#endif
void EntryProperties::browse()
{
    QString pth;
    if (sender() == browseDirB)
        pth = QFileDialog::getExistingDirectory(this, tr("Choose a directory"), pthE->text());
    else if (sender() == browseFileB)
        pth = QFileDialog::getOpenFileName(this, tr("Choose a file that contains more than one track"), pthE->text());
    if (!pth.isEmpty())
    {
        if (nameE->text().isEmpty())
            nameE->setText(Functions::fileName(pth));
        pthE->setText(QDir::toNativeSeparators(pth));
    }
}
void EntryProperties::openUrl()
{
    const QString url = addrB->cleanUrl();
    const QUrl qurl = QUrl(url);
    if (qurl.scheme().isEmpty())
    {
        const QFileInfo fileInfo(url);
        QDesktopServices::openUrl(QUrl::fromLocalFile(fileInfo.isDir() ? url : fileInfo.path()));
    }
    else
    {
        QDesktopServices::openUrl(qurl);
    }
}
void EntryProperties::accept()
{
    if (catalogCB)
    {
        if (catalogCB->isChecked())
        {
            const QString newPth = pthE->text();
            const QFileInfo pthInfo = newPth;
            if (newPth.contains("://") || pthInfo.isDir() || pthInfo.isFile())
            {
                if (nameE->text().isEmpty())
                    nameE->setText(Functions::fileName(newPth));
                const QString newPthWithScheme = (pthInfo.isFile() ? "file://" : "") + newPth;
                if (!pthInfo.isDir() && (nameE->text() != tWI->text(0) || newPthWithScheme == tWI->data(0, Qt::UserRole).toString()))
                    tWI->setData(0, Qt::UserRole + 2, true); //Don't allow to change the group name automatically
                tWI->setData(0, Qt::UserRole, newPthWithScheme);
                QMPlay2GUI.setTreeWidgetItemIcon(tWI, *QMPlay2GUI.folderIcon);
                sync = true;
            }
            else
            {
                QMessageBox::information(this, tr("Incorrect path"), tr("The specified path does not exist"));
                return;
            }
        }
        else
        {
            tWI->setData(0, Qt::UserRole, QString());
            QMPlay2GUI.setTreeWidgetItemIcon(tWI, *QMPlay2GUI.groupIcon);
        }
        if (!nameE->isReadOnly())
            tWI->setText(0, nameE->text());
    }
    else
    {
        QString url = addrB->url();
        const QString scheme = Functions::getUrlScheme(url);
        if (addrB->currentPrefixType() == AddressBox::DIRECT && (scheme.isEmpty() || scheme.length() == 1 /*Drive letter in Windows*/))
            url.prepend("file://");
        if (!url.startsWith("file://") && !nameE->text().simplified().isEmpty())
            tWI->setText(0, nameE->text());
        tWI->setData(0, Qt::UserRole, url);
#ifdef QMPlay2_TagEditor
        if (tagEditor->isEnabled() && !tagEditor->save())
            QMessageBox::critical(this, tagEditor->title(), tr("Error while writing tags, please check if you have permission to modify the file!"));
#endif
    }
    QDialog::accept();
}
void EntryProperties::reject()
{
    if (PlaylistWidget::isGroup(tWI) && tWI->text(0).isEmpty())
        delete tWI;
    QDialog::reject();
}
