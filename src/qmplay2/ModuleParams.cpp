#include <ModuleParams.hpp>

bool ModuleParams::modParam(const QString &key, const QVariant &val)
{
	QHash<QString, QVariant>::iterator it = paramList.find(key);
	if (it != paramList.end())
	{
		*it = val;
		return true;
	}
	return false;
}

bool ModuleParams::processParams(bool *paramsCorrected)
{
	Q_UNUSED(paramsCorrected)
	return true;
}
