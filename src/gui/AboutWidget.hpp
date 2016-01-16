#ifndef ABOUTWIDGET_HPP
#define ABOUTWIDGET_HPP

#include <QWidget>
#include <QTextEdit>
#include <QFileSystemWatcher>

class CommunityEdit : public QTextEdit
{
	Q_OBJECT
private:
	void mouseMoveEvent(QMouseEvent *);
	void mousePressEvent(QMouseEvent *);
};

/**/

class QPushButton;

class AboutWidget : public QWidget
{
	Q_OBJECT
public:
	AboutWidget();
private:
	void showEvent(QShowEvent *);
	void closeEvent(QCloseEvent *);

	CommunityEdit *dE;
	QTextEdit *logE, *clE;
	QPushButton *clrLogB;
	QFileSystemWatcher logWatcher;
private slots:
	void refreshLog();
	void clrLog();
	void currentTabChanged(int);
};

#endif //ABOUTWIDGET_HPP
