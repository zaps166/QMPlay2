#include <VFilters.hpp>
#include <BobDeint.hpp>
#include <BlendDeint.hpp>
#include <DiscardDeint.hpp>
#include <MotionBlur.hpp>

VFilters::VFilters() :
	Module( "VideoFilters" )
{
	moduleImg = QImage( ":/videofilters" );
}

QList< VFilters::Info > VFilters::getModulesInfo( const bool ) const
{
	QList< Info > modulesInfo;
	modulesInfo += Info( BobDeintName, VIDEOFILTER | DEINTERLACE | DOUBLER );
	modulesInfo += Info( BlendDeintName, VIDEOFILTER | DEINTERLACE );
	modulesInfo += Info( DiscardDeintName, VIDEOFILTER | DEINTERLACE );
	modulesInfo += Info( MotionBlurName, VIDEOFILTER, tr( "Tworzy jedną dodatkową ramkę, która jest średnią z dwóch sąsiadujących klatek" ) );
	return modulesInfo;
}
void *VFilters::createInstance( const QString &name )
{
	if ( name == BobDeintName )
		return new BobDeint();
	else if ( name == BlendDeintName )
		return new BlendDeint();
	else if ( name == DiscardDeintName )
		return new DiscardDeint();
	else if ( name == MotionBlurName )
		return new MotionBlur();
	return NULL;
}

QMPLAY2_EXPORT_PLUGIN( VFilters )
