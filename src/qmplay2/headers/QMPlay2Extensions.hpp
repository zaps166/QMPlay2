#ifndef QMPLAY2EXTENSIONS_HPP
#define QMPLAY2EXTENSIONS_HPP

#include <IOController.hpp>
#include <ModuleCommon.hpp>
#include <DockWidget.hpp>

#include <QString>
#include <QImage>

class QMPlay2Extensions : public ModuleCommon
{
public:
	static void openExtensions();
	static inline QList<QMPlay2Extensions *> QMPlay2ExtensionsList()
	{
		return guiExtensionsList;
	}
	static void closeExtensions();

	class AddressPrefix
	{
	public:
		inline AddressPrefix(const QString &prefix, const QImage &img = QImage()) :
			prefix(prefix), img(img) {}

		inline bool operator ==(const AddressPrefix &other)
		{
			return other.prefix == prefix;
		}
		inline operator QString() const
		{
			return prefix;
		}

		QString prefix;
		QImage img;
	};

	virtual DockWidget *getDockWidget()
	{
		return NULL;
	}

	virtual QList<AddressPrefix> addressPrefixList(bool img = true)
	{
		Q_UNUSED(img)
		return QList<AddressPrefix>();
	}
	virtual void convertAddress(const QString &, const QString &, const QString &, QString *, QString *, QImage *, QString *extension, IOController<> *ioCtrl)
	{
		Q_UNUSED(extension)
		Q_UNUSED(ioCtrl)
	}

	virtual QAction *getAction(const QString &, int, const QString &, const QString &prefix = QString(), const QString &param = QString())
	{
		Q_UNUSED(prefix)
		Q_UNUSED(param)
		return NULL;
	}

	virtual bool isVisualization() const
	{
		return false;
	}
	virtual void connectDoubleClick(const QObject *, const char *)
	{}
	virtual void visState(bool, uchar chn = 0, uint srate = 0)
	{
		Q_UNUSED(chn)
		Q_UNUSED(srate)
	}
	virtual void sendSoundData(const QByteArray &)
	{}
protected:
	virtual ~QMPlay2Extensions() {}

	virtual void init() {} //Jeżeli jakieś rozszerzenie używa innego podczas inicjalizacji. Wywoływane po załadowaniu wszystkich.
private:
	static QList<QMPlay2Extensions *> guiExtensionsList;
};

#endif //QMPLAY2EXTENSIONS_HPP
