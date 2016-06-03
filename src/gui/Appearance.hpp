#ifndef APPEARANCE_HPP
#define APPEARANCE_HPP

#include <QDialog>

class QDialogButtonBox;
class QAbstractButton;
class QDoubleSpinBox;
class ColorButton;
class QGroupBox;
class QComboBox;
class QSettings;

class WallpaperW : public QWidget
{
public:
	void setPixmap(const QPixmap &pix)
	{
		pixmap = pix;
		update();
	}
	inline const QPixmap &getPixmap() const
	{
		return pixmap;
	}
private:
	void paintEvent(QPaintEvent *);

	QPixmap pixmap;
};

class Appearance : public QDialog
{
	Q_OBJECT
public:
	static void init();
	static void applyPalette(const QPalette &pal, const QPalette &sliderButton_pal, const QPalette &mainW_pal);

	Appearance(QWidget *p);
private slots:
	void schemesIndexChanged(int idx);
	void newScheme();
	void saveScheme();
	void deleteScheme();
	void chooseWallpaper();
	void buttonBoxClicked(QAbstractButton *b);
	void showReadOnlyWarning();
private:
	void saveScheme(QSettings &colorScheme);
	void reloadSchemes();
	void loadCurrentPalette();
	void loadDefaultPalette();
	void apply();

	int rwSchemesIdx;
	bool warned;

	QGroupBox *colorSchemesB;
	QComboBox *schemesB;
	QPushButton *newB, *saveB, *deleteB;

	QGroupBox *useColorsB;
	ColorButton *buttonC, *windowC, *shadowC, *highlightC, *baseC, *textC, *highlightedTextC, *sliderButtonC;

	QGroupBox *useWallpaperB;
	WallpaperW *wallpaperW;
	QPushButton *wallpaperB;
	QDoubleSpinBox *alphaB;

	QGroupBox *gradientB;
	ColorButton *grad1C, *grad2C, *qmpTxtC;

	QDialogButtonBox *buttonBox;
};

#endif // APPEARANCE_HPP
