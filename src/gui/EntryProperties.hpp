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

#pragma once

#include <QDialog>

class QTreeWidgetItem;
class QToolButton;
class AddressBox;
class QLineEdit;
class QCheckBox;
#ifdef QMPlay2_TagEditor
    class TagEditor;
#endif
class QLabel;

class EntryProperties final : public QDialog
{
    Q_OBJECT
public:
    EntryProperties(QWidget *, QTreeWidgetItem *, bool &, bool &);
private:
    QTreeWidgetItem *tWI;
    QLineEdit *nameE, *pthE;
#ifdef QMPlay2_TagEditor
    TagEditor *tagEditor;
#endif
    QLabel *fileSizeL;
    QCheckBox *catalogCB;
    QToolButton *browseDirB, *browseFileB, *openUrlB;
    AddressBox *addrB;
    QString origDirPth;
    bool &sync;
private slots:
    void setDirPthEEnabled(int);
#ifdef QMPlay2_TagEditor
    void directAddressChanged();
#endif
    void browse();
    void openUrl();
    void accept() override;
    void reject() override;
};
