#ifndef MODULEPARAMS_HPP
#define MODULEPARAMS_HPP

#include <QHash>
#include <QString>
#include <QVariant>

class ModuleParams
{
	Q_DISABLE_COPY(ModuleParams)
public:
	inline bool modParam(const QString &key, const QVariant &val)
	{
		if (hasParam(key))
		{
			paramList.insert(key, val);
			return true;
		}
		return false;
	}
	inline QVariant getParam(const QString &key) const
	{
		return paramList.value(key);
	}
	inline bool hasParam(const QString &key) const
	{
		return paramList.contains(key);
	}

	virtual bool processParams(bool *paramsCorrected = NULL)
	{
		Q_UNUSED(paramsCorrected)
		return true;
	}
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
