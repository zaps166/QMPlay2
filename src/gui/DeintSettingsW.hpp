#ifndef DEINTSETTINGSW_HPP
#define DEINTSETTINGSW_HPP

#include <QGroupBox>

class QCheckBox;
class QComboBox;

class DeintSettingsW : public QGroupBox
{
	Q_OBJECT
public:
	static void init();

	DeintSettingsW();

	void writeSettings();
private slots:
	void softwareMethods(bool doubler);
	void setSoftwareMethodsToolTip(int idx);
private:
	QCheckBox *autoDeintB, *doublerB, *autoParityB;
	QComboBox *softwareMethodsCB, *parityCB;
};

#endif
