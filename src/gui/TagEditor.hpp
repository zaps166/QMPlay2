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

#include <QGroupBox>

namespace TagLib {
    class FileRef;
    class ByteVector;
}
class QPushButton;
class QLineEdit;
class QSpinBox;
class QLabel;

class PictureW final : public QWidget
{
public:
    PictureW(TagLib::ByteVector &picture);
private:
    void paintEvent(QPaintEvent *) override;

    TagLib::ByteVector &picture;
};

class TagEditor : public QGroupBox
{
    Q_OBJECT
public:
    TagEditor();
    ~TagEditor();

    bool open(const QString &fileName);
    void clear();
    bool save();
private slots:
    void loadImage();
    void saveImage();
private:
    void clearValues();

    TagLib::FileRef *fRef;
    TagLib::ByteVector *picture;
    QByteArray pictureMimeType;
    bool pictureModificated, pictureBChecked;

    QLineEdit *titleE, *artistE, *albumE, *commentE, *genreE;
    QSpinBox *yearB, *trackB;
    QGroupBox *pictureB;
    PictureW *pictureW;
    QPushButton *loadImgB, *saveImgB;
};
