#include <ColorButton.hpp>

#include <QColorDialog>
#include <QPainter>

ColorButton::ColorButton(bool showAlphaChannel) :
	showAlphaChannel(showAlphaChannel)
{
	setCursor(Qt::PointingHandCursor);
	connect(this, SIGNAL(clicked()), this, SLOT(openColorDialog()));
}

void ColorButton::setColor(const QColor &_color)
{
	setToolTip(QString("#%1").arg(_color.rgba(), 8, 16).replace(' ', '0').toUpper());
	color = _color;
	update();
}

void ColorButton::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	p.fillRect(rect(), color);
}

void ColorButton::openColorDialog()
{
	QColor c = QColorDialog::getColor(getColor(), this, QString(), showAlphaChannel ? QColorDialog::ShowAlphaChannel : (QColorDialog::ColorDialogOptions)0);
	if (c.isValid())
		setColor(c);
}
