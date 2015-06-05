#include <Module.hpp>

class Inputs : public Module
{
	Q_OBJECT
public:
	Inputs();
private:
	QList< Info > getModulesInfo( const bool ) const;
	void *createInstance( const QString & );

	QList< QAction * > getAddActions();

	SettingsWidget *getSettingsWidget();

	QImage sine, ray2;
private slots:
	void add();
};

/**/

#include <QSpinBox>
#include <QDialog>

class HzW : public QWidget
{
public:
	HzW( int, const QStringList & );

	QString getFreqs() const;
	inline void connectFreqs( const QObject *receiver, const char *method )
	{
		for ( int i = 0 ; i < hzB.count() ; ++i )
			hzB[ i ]->connect( hzB[ i ], SIGNAL( valueChanged( int ) ), receiver, method );
	}
private:
	QList< QSpinBox * > hzB;
};

class QGridLayout;

class AddD : public QDialog
{
	Q_OBJECT
public:
	AddD( Settings &, QWidget *parent = NULL, QObject *moduleSetsW = NULL );

	inline int getSampleRate() const
	{
		return srateB->value();
	}
	inline QString getFreqs() const
	{
		return hzW->getFreqs();
	}

	void save();

	QString execAndGet();
private slots:
	void channelsChanged( int );
	void add();
private:
	QObject *moduleSetsW;
	QGridLayout *layout;
	QSpinBox *srateB;
	Settings &sets;
	HzW *hzW;
};

class QRadioButton;
class QComboBox;
class QGroupBox;
class QCheckBox;
class QLineEdit;

class ModuleSettingsWidget : public Module::SettingsWidget
{
	Q_OBJECT
public:
	ModuleSettingsWidget( Module & );
private slots:
	void applyFreqs();
private:
	void saveSettings();

	AddD *toneGenerator;
	QGroupBox *pcmB;
	QLineEdit *pcmExtsE;
	QList< QRadioButton * > formatB;
	QSpinBox *chnB, *srateB, *offsetB;
	QComboBox *endianB;

	QCheckBox *rayman2EB;
};
