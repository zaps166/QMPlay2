#include <VFilters.hpp>
#include <BobDeint.hpp>
#include <YadifDeint.hpp>
#include <BlendDeint.hpp>
#include <DiscardDeint.hpp>
#include <MotionBlur.hpp>

VFilters::VFilters() :
	Module("VideoFilters")
{
	moduleImg = QImage(":/videofilters");
}

QList<VFilters::Info> VFilters::getModulesInfo(const bool) const
{
	QList<Info> modulesInfo;
	modulesInfo += Info(BobDeintName, VIDEOFILTER | DEINTERLACE | DOUBLER);
	modulesInfo += Info(Yadif2xDeintName, VIDEOFILTER | DEINTERLACE | DOUBLER, YadifDescr);
	modulesInfo += Info(Yadif2xNoSpatialDeintName, VIDEOFILTER | DEINTERLACE | DOUBLER, YadifDescr);
	modulesInfo += Info(YadifDeintName, VIDEOFILTER | DEINTERLACE, YadifDescr);
	modulesInfo += Info(BlendDeintName, VIDEOFILTER | DEINTERLACE);
	modulesInfo += Info(DiscardDeintName, VIDEOFILTER | DEINTERLACE);
	modulesInfo += Info(YadifNoSpatialDeintName, VIDEOFILTER | DEINTERLACE, YadifDescr);
	modulesInfo += Info(MotionBlurName, VIDEOFILTER, tr("Produces one extra frame which is average of two neighbour frames"));
	return modulesInfo;
}
void *VFilters::createInstance(const QString &name)
{
	if (name == BobDeintName)
		return new BobDeint;
	else if (name == Yadif2xDeintName)
		return new YadifDeint(true, true);
	else if (name == Yadif2xNoSpatialDeintName)
		return new YadifDeint(true, false);
	else if (name == BlendDeintName)
		return new BlendDeint;
	else if (name == DiscardDeintName)
		return new DiscardDeint;
	else if (name == YadifDeintName)
		return new YadifDeint(false, true);
	else if (name == YadifNoSpatialDeintName)
		return new YadifDeint(false, false);
	else if (name == MotionBlurName)
		return new MotionBlur;
	return NULL;
}

QMPLAY2_EXPORT_PLUGIN(VFilters)
