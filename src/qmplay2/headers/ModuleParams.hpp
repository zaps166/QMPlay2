#ifndef MODULEPARAMS_HPP
#define MODULEPARAMS_HPP

#include <QHash>
#include <QString>
#include <QVariant>

class ModuleParams
{
	Q_DISABLE_COPY(ModuleParams)
public:
	bool modParam(const QString &key, const QVariant &val);
	inline QVariant getParam(const QString &key) const
	{
		return paramList.value(key);
	}
	inline bool hasParam(const QString &key) const
	{
		return paramList.contains(key);
	}

	virtual bool processParams(bool *paramsCorrected = NULL);
protected:
	ModuleParams() {}
	~ModuleParams() {}

	inline void addParam(const QString &key, const QVariant &val = QVariant())
	{
		paramList.insert(key, val);
	}
private:
	QHash<QString, QVariant> paramList;
};

#endif
