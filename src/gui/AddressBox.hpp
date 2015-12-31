#ifndef ADDRESSBOX_HPP
#define ADDRESSBOX_HPP

#include <QComboBox>
#include <QLineEdit>

class AddressBox : public QWidget
{
	Q_OBJECT
public:
	enum PrefixType { DIRECT = 'A', MODULE = 'M' };

	AddressBox( Qt::Orientation, QString url = QString() );

	inline void setFocus()
	{
		aE.setFocus();
	}

	inline QComboBox &getComboBox()
	{
		return pB;
	}

	inline PrefixType currentPrefixType() const
	{
		return ( PrefixType )pB.itemData( pB.currentIndex() ).toInt();
	}
	QString url() const;
	QString cleanUrl() const;
signals:
	void directAddressChanged();
private slots:
	void pBIdxChanged();
	void aETextChanged();
private:
	QComboBox pB;
	QLineEdit aE, pE;
	QString filePrefix;
};

#endif //ADDRCOMBOBOX_HPP
