#ifndef TAGEDITOR_HPP
#define TAGEDITOR_HPP

#include <QGroupBox>

namespace TagLib {
	class FileRef;
	class ByteVector;
}
class QPushButton;
class QLineEdit;
class QSpinBox;
class QLabel;

class PictureW : public QWidget
{
public:
	PictureW(TagLib::ByteVector &picture);
private:
	void paintEvent(QPaintEvent *);

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

#endif //TAGEDITOR_HPP
