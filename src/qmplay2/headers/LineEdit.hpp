#ifndef LINEEDIT_HPP
#define LINEEDIT_HPP

#include <QLineEdit>
#include <QLabel>

class LineEditButton : public QLabel
{
	Q_OBJECT
public:
	LineEditButton();
private:
	void mousePressEvent(QMouseEvent *);
signals:
	void clicked();
};

/**/

class LineEdit : public QLineEdit
{
	Q_OBJECT
public:
	LineEdit(QWidget *parent = NULL);
private:
	void resizeEvent(QResizeEvent *);
	void mousePressEvent(QMouseEvent *);
	void mouseMoveEvent(QMouseEvent *);

	LineEditButton b;
private slots:
	void textChangedSlot(const QString &);
	void clear_text();
signals:
	void clearButtonClicked();
};

#endif
