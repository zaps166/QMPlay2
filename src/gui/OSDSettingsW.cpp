#include <OSDSettingsW.hpp>

#include <Main.hpp>

#include <ColorButton.hpp>
#include <Settings.hpp>

#include <QDoubleSpinBox>
#include <QFontComboBox>
#include <QRadioButton>
#include <QPushButton>
#include <QGridLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QLabel>

void OSDSettingsW::init(const QString &prefix, int a, int b, int c, int d, int e, int f, double g, double h, const QColor &i, const QColor &j, const QColor &k)
{
	Settings &QMPSettings = QMPlay2Core.getSettings();
	QMPSettings.init(prefix + "/FontName", QFontComboBox().currentText());
	QMPSettings.init(prefix + "/FontSize", a);
	QMPSettings.init(prefix + "/Spacing", b);
	QMPSettings.init(prefix + "/LeftMargin", c);
	QMPSettings.init(prefix + "/RightMargin", d);
	QMPSettings.init(prefix + "/VMargin", e);
	QMPSettings.init(prefix + "/Alignment", f);
	QMPSettings.init(prefix + "/Outline", g);
	QMPSettings.init(prefix + "/Shadow", h);
	QMPSettings.init(prefix + "/TextColor", i);
	QMPSettings.init(prefix + "/OutlineColor", j);
	QMPSettings.init(prefix + "/ShadowColor", k);
	int align = QMPSettings.getInt(prefix + "/Alignment");
	if (align < 0 || align > 8)
		QMPSettings.set(prefix + "/Alignment", f);
}

OSDSettingsW::OSDSettingsW(const QString &prefix) :
	prefix(prefix)
{
	//Font GroupBox
	fontCB = new QFontComboBox;
	fontSizeL = new QLabel;
	fontSizeL->setText(tr("Size") + ": ");
	fontSizeB = new QSpinBox;
	spacingL = new QLabel;
	spacingL->setText(tr("Spacing") + ": ");
	spacingB = new QSpinBox;
	spacingB->setMinimum(-20);
	spacingB->setMaximum(20);

	fontGB = new QGroupBox;
	fontGB->setTitle(tr("Font"));

	fontL = new QGridLayout(fontGB);
	fontL->addWidget(fontCB, 0, 0, 1, 2);
	fontL->addWidget(fontSizeL, 1, 0, 1, 1);
	fontL->addWidget(fontSizeB, 1, 1, 1, 1);
	fontL->addWidget(spacingL, 2, 0, 1, 1);
	fontL->addWidget(spacingB, 2, 1, 1, 1 );

	//Margins GroupBox
	leftMarginL = new QLabel;
	leftMarginL->setText(tr("Left") + ": ");
	leftMarginB = new QSpinBox;
	rightMarginL = new QLabel;
	rightMarginL->setText(tr("Right") + ": ");
	rightMarginB = new QSpinBox;
	vMarginL = new QLabel;
	vMarginL->setText(tr("Vertical") + ": ");
	vMarginB = new QSpinBox;

	marginsGB = new QGroupBox;
	marginsGB->setTitle(tr("Margins"));

	marginsL = new QGridLayout(marginsGB);
	marginsL->addWidget(leftMarginL, 0, 0);
	marginsL->addWidget(leftMarginB, 0, 1);
	marginsL->addWidget(rightMarginL, 1, 0);
	marginsL->addWidget(rightMarginB, 1, 1);
	marginsL->addWidget(vMarginL, 2, 0);
	marginsL->addWidget(vMarginB, 2, 1);

	//Alignment GroupBox
	alignGB = new QGroupBox;
	alignGB->setTitle(tr("Subtitles alignment"));

	alignL = new QGridLayout(alignGB);
	for (int align = 0; align < 9; align++)
	{
		alignB[align] = new QRadioButton;
		alignL->addWidget(alignB[align], align/3, align%3);
	}

	//Frame GroupBox
	outlineL = new QLabel;
	outlineL->setText(tr("Outline") + ": ");
	outlineB = new QDoubleSpinBox;
	outlineB->setMaximum(4.);
	shadowL = new QLabel;
	shadowL->setText(tr("Shadow") + ": ");
	shadowB = new QDoubleSpinBox;
	shadowB->setMaximum(4.);

	frameGB = new QGroupBox;
	frameGB->setTitle(tr("Border"));

	frameL = new QGridLayout(frameGB);
	frameL->addWidget(outlineL, 0, 0);
	frameL->addWidget(outlineB, 0, 1);
	frameL->addWidget(shadowL, 1, 0);
	frameL->addWidget(shadowB, 1, 1);

	//Colors GroupBox
	textColorL = new QLabel;
	textColorL->setText(tr("Text") + ": ");
	textColorB = new ColorButton;
	outlineColorL = new QLabel;
	outlineColorL->setText(tr("Border") + ": ");
	outlineColorB = new ColorButton;
	shadowColorL = new QLabel;
	shadowColorL->setText(tr("Shadow") + ": ");
	shadowColorB = new ColorButton;

	colorsGB = new QGroupBox;
	colorsGB->setTitle(tr("Colors"));

	colorsL = new QGridLayout(colorsGB);
	colorsL->addWidget(textColorL, 0, 0);
	colorsL->addWidget(textColorB, 0, 1);
	colorsL->addWidget(outlineColorL, 1, 0);
	colorsL->addWidget(outlineColorB, 1, 1);
	colorsL->addWidget(shadowColorL, 2, 0);
	colorsL->addWidget(shadowColorB, 2, 1);

	//Widget layout
	layout = new QGridLayout(this);
	layout->setMargin(1);
	layout->addWidget(fontGB, 0, 0, 1, 2);
	layout->addWidget(marginsGB, 0, 2, 1, 3);
	layout->addWidget(alignGB, 1, 0, 1, 1);
	layout->addWidget(frameGB, 1, 1, 1, 2);
	layout->addWidget(colorsGB, 1, 3, 1, 2);

	readSettings();
}

void OSDSettingsW::writeSettings()
{
	Settings &QMPSettings = QMPlay2Core.getSettings();
	QMPSettings.set(prefix + "/FontName", fontCB->currentText());
	QMPSettings.set(prefix + "/FontSize", fontSizeB->value());
	QMPSettings.set(prefix + "/Spacing", spacingB->value());
	QMPSettings.set(prefix + "/LeftMargin", leftMarginB->value());
	QMPSettings.set(prefix + "/RightMargin", rightMarginB->value());
	QMPSettings.set(prefix + "/VMargin", vMarginB->value());
	for (int align = 0; align < 9; ++align)
		if (alignB[align]->isChecked())
		{
			QMPSettings.set(prefix + "/Alignment", align);
			break;
		}
	QMPSettings.set(prefix + "/Outline", outlineB->value());
	QMPSettings.set(prefix + "/Shadow", shadowB->value());
	QMPSettings.set(prefix + "/TextColor", textColorB->getColor());
	QMPSettings.set(prefix + "/OutlineColor", outlineColorB->getColor());
	QMPSettings.set(prefix + "/ShadowColor", shadowColorB->getColor());
}

void OSDSettingsW::readSettings()
{
	Settings &QMPSettings = QMPlay2Core.getSettings();
	int idx = fontCB->findText(QMPSettings.getString(prefix + "/FontName"));
	if (idx > -1)
		fontCB->setCurrentIndex(idx);
	fontSizeB->setValue(QMPSettings.getInt(prefix + "/FontSize"));
	spacingB->setValue(QMPSettings.getInt(prefix + "/Spacing"));
	leftMarginB->setValue(QMPSettings.getInt(prefix + "/LeftMargin"));
	rightMarginB->setValue(QMPSettings.getInt(prefix + "/RightMargin"));
	vMarginB->setValue(QMPSettings.getInt(prefix + "/VMargin"));
	int align = QMPSettings.getInt(prefix + "/Alignment");
	alignB[align]->setChecked(true);
	outlineB->setValue(QMPSettings.getDouble(prefix + "/Outline"));
	shadowB->setValue(QMPSettings.getDouble(prefix + "/Shadow"));
	textColorB->setColor(QMPSettings.get(prefix + "/TextColor").value< QColor >());
	outlineColorB->setColor(QMPSettings.get(prefix + "/OutlineColor").value< QColor >());
	shadowColorB->setColor(QMPSettings.get(prefix + "/ShadowColor").value< QColor >());
}
