#include <FileAssociation.hpp>

#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QTimer>

#define VER 3

FileAssociation::FileAssociation() :
	Module("FileAssociation")
{
	moduleImg = QImage(":/regedit");

	init("Associate", true);
	init("Dirs", true);
	init("Drvs", true);
	init("AudioCD", true);
	if (getUInt("Ver") < VER)
	{
		reallyFirsttime = contains("Ver");
		set("Ver", VER);
		QTimer::singleShot(0, this, SLOT(firsttime()));
	}
}

QList< FileAssociation::Info > FileAssociation::getModulesInfo(const bool) const
{
	return QList< Info >();
}
void *FileAssociation::createInstance(const QString &)
{
	return NULL;
}

FileAssociation::SettingsWidget *FileAssociation::getSettingsWidget()
{
	return new ModuleSettingsWidget(*this);
}

void FileAssociation::firsttime()
{
	if (QMessageBox::question(NULL, tr("Skojarzenie plików"), tr("Czy chcesz skojarzyć pliki z programem QMPlay2"), QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes)
	{
		QDialog d;
		QDialogButtonBox bb;
		QVBoxLayout layout(&d);
		ModuleSettingsWidget *settingsW = (ModuleSettingsWidget *)getSettingsWidget();
		bb.setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
		connect(&bb, SIGNAL(accepted()), &d, SLOT(accept()));
		connect(&bb, SIGNAL(rejected()), &d, SLOT(reject()));
		if (!settingsW->selectAllB->isChecked())
			settingsW->selectAllB->click();
		layout.addWidget(settingsW);
		layout.addWidget(&bb);
		layout.setMargin(2);
		d.resize(0, 380);
		if (d.exec() == QDialog::Accepted)
			settingsW->saveSettings();
	}
	else if (reallyFirsttime)
		set("Associate", false);
}

QMPLAY2_EXPORT_PLUGIN(FileAssociation)

/**/

#include <windows.h>
#include <shlwapi.h>

enum ShellContextMenu { DIRECTORY, DRIVE };

static void AddStringToReg(const QString &regPath, const QString &value)
{
	HKEY key;
	if (!RegCreateKeyW(HKEY_CURRENT_USER, (WCHAR *)QString("Software\\Classes\\" + regPath).utf16(), &key))
	{
		RegSetValueExW(key, NULL, 0, REG_SZ, (const BYTE *)value.utf16(), value.size() * sizeof(WCHAR) + 2);
		RegCloseKey(key);
	}
}
static void CreateQMPlay2Key(const QString &name, const QString &path, const unsigned icon, const QString &enqueueInQMPlay2)
{
	AddStringToReg(name + "\\DefaultIcon", path + "," + QString::number(icon));
	AddStringToReg(name + "\\Shell\\Open\\command", "\"" + path + "\" \"%1\"");
	AddStringToReg(name + "\\Shell\\" + enqueueInQMPlay2 + "\\command", "\"" + path + "\" --enqueue \"%1\"");
}
static void CreateShellContextMenu(const ShellContextMenu shellContextMenu, const QString &path, const QString &playInQMPlay2)
{
	QString name, p1;
	switch (shellContextMenu)
	{
		case DIRECTORY:
			name = "Directory";
			p1 = "\"%1\"";
			break;
		case DRIVE:
			name = "Drive";
			p1 = "%1";
			break;
	}
	AddStringToReg(name + "\\shell\\QMPlay2", playInQMPlay2);
	AddStringToReg(name + "\\shell\\QMPlay2\\command", "\"" + path + "\" " + p1);
}
static QStringList EnumExtensionKeys()
{
	HKEY key;
	QStringList keys;
	if (!RegOpenKeyW(HKEY_CURRENT_USER, L"Software\\Classes", &key))
	{
		WCHAR name[ 256 ];
		for (DWORD d = 0; !RegEnumKeyW(key, d, name, sizeof name / sizeof *name); ++d)
		{
			WCHAR value[ 16 ];
			DWORD valueLen = sizeof value;
			QString keyName = QString::fromUtf16((const ushort *)name);
			if (!SHGetValue(HKEY_CURRENT_USER, (WCHAR *)QString("Software\\Classes\\" + keyName).utf16(), NULL, NULL, value, &valueLen) && !wcsncmp(value, L"QMPlay2", 7))
				keys += keyName;
		}
		RegCloseKey(key);
	}
	return keys;
}
static void RemoveQMPlay2Keys()
{
	SHDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Classes\\QMPlay2");
	SHDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Classes\\AudioCD");
	SHDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Classes\\QMPlay2Playlist");
	SHDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Classes\\Drive\\shell\\QMPlay2");
	SHDeleteKeyW(HKEY_CURRENT_USER, L"Software\\Classes\\Directory\\shell\\QMPlay2");
	foreach (const QString &extension, EnumExtensionKeys())
		SHDeleteKeyW(HKEY_CURRENT_USER, (WCHAR *)QString("Software\\Classes\\" + extension).utf16());
}

/**/

#include <QCoreApplication>
#include <QListWidget>
#include <QCheckBox>
#include <QCheckBox>
#include <QGroupBox>

ModuleSettingsWidget::ModuleSettingsWidget(Module &module) :
	Module::SettingsWidget(module),
	ignoreCheck(false)
{
	associateB = new QGroupBox(tr("Skojarz pliki z QMPlay2"));
	associateB->setCheckable(true);
	associateB->setChecked(sets().getBool("Associate"));

	selectAllB = new QPushButton(tr("Wybierz wszystkie"));
	connect(selectAllB, SIGNAL(clicked()), this, SLOT(selectAll()));
	selectAllB->setCheckable(true);

	extensionLW = new QListWidget;
	connect(extensionLW, SIGNAL(itemChanged(QListWidgetItem *)), this, SLOT(checkSelected()));
	extensionLW->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	extensionLW->setResizeMode(QListView::Adjust);
	extensionLW->setSortingEnabled(true);
	extensionLW->setWrapping(true);

	static const char *defaultVideoExtensions[] = { "mkv", "mp4", "mpg", "mpeg", "asf", "wmv", "ogv", "ogm", "webm", "3gp", "dv", "mts", "m2t", "m2ts", "ts", "m4v", "vob", "qt", "mov", "flv", "avi", "divx", "rmvb", "rm", "bik" };
	static const char *defaultAudioExtensions[] = { "ogg", "mp3", "wma", "aac", "ac3", "amr", "wav", "flac", "alac", "ape", "wv", "wvp", "mp2", "aiff", "aif", "mus" };
	static const size_t defaultVideoCount = sizeof defaultVideoExtensions / sizeof *defaultVideoExtensions;
	static const size_t defaultAudioCount = sizeof defaultAudioExtensions / sizeof *defaultAudioExtensions;

	foreach (const QString &extension, EnumExtensionKeys())
		addExtension(extension.right(extension.length() - 1), true);
	for (size_t s = 0; s < defaultAudioCount; ++s)
		addExtension(defaultAudioExtensions[ s ], false);
	for (size_t s = 0; s < defaultVideoCount; ++s)
		addExtension(defaultVideoExtensions[ s ], false);
	foreach (Module *module, QMPlay2Core.getPluginsInstance())
			foreach (const Module::Info &mod, module->getModulesInfo())
				if (mod.type == Module::PLAYLIST || mod.type == Module::DEMUXER)
					foreach (const QString &extension, mod.extensions)
						addExtension(extension, false, mod.type == Module::PLAYLIST);

	addDirB = new QCheckBox(tr("Dodaj wpis do menu kontekstowego z katalogami"));
	addDirB->setChecked(sets().getBool("Dirs"));

	addDrvB = new QCheckBox(tr("Dodaj wpis do menu kontekstowego z dyskami"));
	addDrvB->setChecked(sets().getBool("Drvs"));

	audioCDB = new QCheckBox(tr("Ustaw jako domyślny odtwarzacz AudioCD"));
	audioCDB->setChecked(sets().getBool("AudioCD"));

	QVBoxLayout *aLayout = new QVBoxLayout(associateB);
	aLayout->addWidget(selectAllB);
	aLayout->addWidget(extensionLW);
	aLayout->addWidget(addDirB);
	aLayout->addWidget(addDrvB);
	aLayout->addWidget(audioCDB);

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(associateB);
	layout->setMargin(2);
}

void ModuleSettingsWidget::saveSettings()
{
	RemoveQMPlay2Keys();
	if (associateB->isChecked())
	{
		const QString QMPlay2Path = qApp->applicationFilePath().replace('/', '\\');
		CreateQMPlay2Key("QMPlay2", QMPlay2Path, 1, tr("Kolejkuj w QMPlay2"));
		CreateQMPlay2Key("QMPlay2Playlist", QMPlay2Path, 2, tr("Kolejkuj w QMPlay2"));
		if (addDirB->isChecked())
			CreateShellContextMenu(DIRECTORY, QMPlay2Path, tr("Odtwórz w QMPlay2"));
		if (addDrvB->isChecked())
			CreateShellContextMenu(DRIVE, QMPlay2Path, tr("Odtwórz w QMPlay2"));
		if (audioCDB->isChecked())
			AddStringToReg("AudioCD\\shell\\play\\command", "\"" + QMPlay2Path + "\"" + " AudioCD://%L");
		for (int i = 0; i < extensionLW->count(); ++i)
		{
			QListWidgetItem *lWI = extensionLW->item(i);
			if (lWI->checkState() == Qt::Checked)
				AddStringToReg("." + lWI->text(), lWI->data(Qt::UserRole).toBool() ? "QMPlay2Playlist" : "QMPlay2");
		}
	}
	sets().set("Associate", associateB->isChecked());
	sets().set("Dirs", addDirB->isChecked());
	sets().set("Drvs", addDrvB->isChecked());
	sets().set("AudioCD", audioCDB->isChecked());
}
void ModuleSettingsWidget::addExtension(const QString &extension, const bool isChecked, const bool isPlaylist)
{
	QList< QListWidgetItem * > items = extensionLW->findItems(extension, Qt::MatchCaseSensitive);
	if (items.isEmpty())
	{
		QListWidgetItem *lWI = new QListWidgetItem(extension, extensionLW);
		lWI->setCheckState(isChecked ? Qt::Checked : Qt::Unchecked);
		lWI->setData(Qt::UserRole, isPlaylist);
	}
	else
		items[ 0 ]->setData(Qt::UserRole, isPlaylist);
}

void ModuleSettingsWidget::selectAll()
{
	ignoreCheck = true;
	for (int i = 0; i < extensionLW->count(); ++i)
		extensionLW->item(i)->setCheckState(selectAllB->isChecked() ? Qt::Checked : Qt::Unchecked);
	ignoreCheck = false;
}
void ModuleSettingsWidget::checkSelected()
{
	if (!ignoreCheck)
	{
		for (int i = 0; i < extensionLW->count(); ++i)
			if (extensionLW->item(i)->checkState() != Qt::Checked)
			{
				selectAllB->setChecked(false);
				return;
			}
		selectAllB->setChecked(true);
	}
}
