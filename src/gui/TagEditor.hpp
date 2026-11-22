// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <QGroupBox>

namespace TagLib {
    class FileRef;
    class ByteVector;
}
class QPushButton;
class QLineEdit;
class QPlainTextEdit;
class QMimeData;
class QSpinBox;
class QLabel;

class PictureW final : public QWidget
{
    Q_OBJECT

public:
    PictureW(TagLib::ByteVector &picture);

private:
    bool verifyMimeData(const QMimeData *m) const;

private:
    void paintEvent(QPaintEvent *) override;

    void dragEnterEvent(QDragEnterEvent *e) override;
    void dropEvent(QDropEvent *e) override;

signals:
    void loadImage(const QString &filePath);

private:
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

private:
    void loadImage(QString filePath);
    void saveImage();

private:
    void clearValues();

    TagLib::FileRef *fRef;
    TagLib::ByteVector *picture;
    QByteArray pictureMimeType;
    bool pictureModificated, pictureBChecked;

    QLineEdit *titleE, *artistE, *albumE, *genreE;
    QPlainTextEdit *commentE;
    QSpinBox *yearB, *trackB;
    QGroupBox *pictureB;
    PictureW *pictureW;
    QPushButton *loadImgB, *saveImgB;
};
