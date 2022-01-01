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

#include <QComboBox>
#include <QLineEdit>

class AddressBox : public QWidget
{
    Q_OBJECT
public:
    enum PrefixType {AUTODETECT = 'D', DIRECT = 'A', MODULE = 'M'};

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
