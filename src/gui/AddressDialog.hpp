#ifndef ADDRESSDIALOG_HPP
#define ADDRESSDIALOG_HPP

#include <AddressBox.hpp>

#include <QCheckBox>
#include <QDialog>

class AddressDialog : public QDialog
{
	Q_OBJECT
public:
	AddressDialog(QWidget *);
	~AddressDialog();

	inline bool addAndPlay() const
	{
		return addAndPlayB.isChecked();
	}

	inline QString url() const
	{
		return addrB.url();
	}
private:
	AddressBox addrB;
	QCheckBox addAndPlayB;
};

#endif //ADDRESSDIALOG_HPP
