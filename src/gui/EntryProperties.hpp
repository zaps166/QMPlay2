// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <GUIHeaders.hpp>

class AddressBox;
#ifdef QMPlay2_TagEditor
class TagEditor;
#endif

class EntryProperties final : public QDialog
{
    Q_OBJECT
public:
    EntryProperties(QWidget *, QTreeWidget *, bool &, bool &);
private:
    QTreeWidget *const m_treeWidget;
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
