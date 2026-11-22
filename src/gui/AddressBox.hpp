// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QComboBox>
#include <QLineEdit>

class AddressBox : public QWidget
{
    Q_OBJECT
public:
    enum PrefixType {DIRECT = 'A', MODULE = 'M'};

    AddressBox(Qt::Orientation, QString url = QString(), const QString &choice = QString());

    inline void setFocus()
    {
        aE.setFocus();
    }

    inline QString getCurrentText() const
    {
        return pB.currentText();
    }

    inline PrefixType currentPrefixType() const
    {
        return (PrefixType)pB.itemData(pB.currentIndex()).toInt();
    }
    QString url() const;
    QString cleanUrl() const;
signals:
    void directAddressChanged();
private slots:
    void pBIdxChanged();
private:
    QComboBox pB;
    QLineEdit aE, pE;
    QString filePrefix;
};
