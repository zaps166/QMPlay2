#include <Module.hpp>

#include <QCoreApplication>

class VFilters : public Module
{
	Q_DECLARE_TR_FUNCTIONS(VFilters)
public:
	VFilters();
private:
	QList<Info> getModulesInfo(const bool) const;
	void *createInstance(const QString &);
};
