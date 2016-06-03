#ifndef COLORBUTTON_HPP
#define COLORBUTTON_HPP

#include <QPushButton>

class ColorButton : public QPushButton
{
	Q_OBJECT
public:
	ColorButton(bool showAlphaChannel = true);

	void setColor(const QColor &color);
	inline QColor getColor() const
	{
		return m_color;
	}
protected:
	void paintEvent(QPaintEvent *);
private:
	QColor m_color;
	bool m_alphaChannel;
private slots:
	void openColorDialog();
signals:
	void colorChanged();
};

#endif //COLORBUTTON_HPP
