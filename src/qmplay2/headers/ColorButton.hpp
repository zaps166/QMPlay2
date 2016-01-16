#ifndef COLORBUTTON_HPP
#define COLORBUTTON_HPP

#include <QPushButton>

class ColorButton : public QPushButton
{
	Q_OBJECT
public:
	ColorButton(bool showAlphaChannel = true);

	void setColor(const QColor &);
	inline QColor getColor() const
	{
		return color;
	}
protected:
	void paintEvent(QPaintEvent *);
private:
	QColor color;
	bool showAlphaChannel;
private slots:
	void openColorDialog();
};

#endif //COLORBUTTON_HPP
