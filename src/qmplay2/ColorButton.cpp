#include <ColorButton.hpp>

#include <QColorDialog>
#include <QPainter>

ColorButton::ColorButton(bool showAlphaChannel) :
	m_alphaChannel(showAlphaChannel)
{
	setCursor(Qt::PointingHandCursor);
	setAttribute(Qt::WA_OpaquePaintEvent);
	connect(this, SIGNAL(clicked()), this, SLOT(openColorDialog()));
}

void ColorButton::setColor(const QColor &color)
{
	setToolTip(QString("#%1").arg(color.rgba(), 8, 16).replace(' ', '0').toUpper());
	m_color = color;
	update();
}

void ColorButton::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	p.fillRect(rect(), m_color);
}

void ColorButton::openColorDialog()
{
	const QColor color = QColorDialog::getColor(getColor(), this, QString(), m_alphaChannel ? QColorDialog::ShowAlphaChannel : QColorDialog::ColorDialogOptions());
	if (color.isValid() && m_color != color)
	{
		setColor(color);
		emit colorChanged();
	}
}
