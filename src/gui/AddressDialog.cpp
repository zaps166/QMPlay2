#include <AddressDialog.hpp>

#include <QMPlay2Core.hpp>
#include <Settings.hpp>

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLineEdit>

AddressDialog::AddressDialog(QWidget *p) :
	QDialog(p), addrB(Qt::Vertical)
{
	setWindowTitle(tr("Add address"));

	QDialogButtonBox *buttonBox = new QDialogButtonBox;
	buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	QComboBox &pB = addrB.getComboBox();
	int idx = pB.findText(QMPlay2Core.getSettings().getString("AddressDialog/Choice"));
	if (idx > -1)
		pB.setCurrentIndex(idx);

	addAndPlayB.setText(tr("Play"));
	addAndPlayB.setChecked(QMPlay2Core.getSettings().getBool("AddressDialog/AddAndPlay", true));

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(&addrB, 0, 0, 1, 2);
	layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding), 1, 0, 1, 2); //vSpacer
	layout->addWidget(&addAndPlayB, 2, 0, 1, 1);
	layout->addWidget(buttonBox, 2, 1, 1, 1);
	layout->setMargin(3);

	addrB.setFocus();
	resize(625, 0);
}
AddressDialog::~AddressDialog()
{
	if (result())
	{
		QMPlay2Core.getSettings().set("AddressDialog/AddAndPlay", addAndPlayB.isChecked());
		QMPlay2Core.getSettings().set("AddressDialog/Choice", addrB.getComboBox().currentText());
	}
}
