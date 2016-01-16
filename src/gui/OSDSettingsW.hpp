#ifndef OSDSETTINGSW_HPP
#define OSDSETTINGSW_HPP

#include <QWidget>

class QDoubleSpinBox;
class QFontComboBox;
class QRadioButton;
class ColorButton;
class QGridLayout;
class QGroupBox;
class QSpinBox;
class QLabel;

class OSDSettingsW : public QWidget
{
	Q_OBJECT
public:
	static void init(const QString &, int, int, int, int, int, int, double, double, const QColor &, const QColor &, const QColor &);

	OSDSettingsW(const QString &);

	void writeSettings();

	QGridLayout *layout, *fontL, *marginsL, *alignL, *frameL, *colorsL;
private:
	void readSettings();

	QString prefix;

	QGroupBox *fontGB, *marginsGB, *alignGB, *frameGB, *colorsGB;

	QLabel *fontSizeL, *spacingL, *leftMarginL, *rightMarginL, *vMarginL, *outlineL, *shadowL, *textColorL, *outlineColorL, *shadowColorL;
	QFontComboBox *fontCB;
	QSpinBox *fontSizeB, *spacingB, *leftMarginB, *rightMarginB, *vMarginB;
	QDoubleSpinBox *outlineB, *shadowB;
	ColorButton *textColorB, *outlineColorB, *shadowColorB;
	QRadioButton *alignB[ 9 ];

};

#endif //OSDSETTINGSW_HPP
