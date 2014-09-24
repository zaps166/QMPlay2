#ifndef ADDRESSBOX_HPP
#define ADDRESSBOX_HPP

#include <QComboBox>
#include <QLineEdit>

class AddressBox : public QWidget
{
	Q_OBJECT
public:
	enum PrefixType { DIRECT = 'A', DEMUXER = 'D', GUI_EXT = 'G' };

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
signals:
	void directAddressChanged();
private slots:
	void pBIdxChanged();
	void aETextChanged();
private:
	QComboBox pB;
	QLineEdit aE, pE;
};

#endif //ADDRCOMBOBOX_HPP
