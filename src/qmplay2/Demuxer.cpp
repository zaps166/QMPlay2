#include <Demuxer.hpp>

#include <QMPlay2Core.hpp>
#include <Functions.hpp>
#include <Module.hpp>

bool Demuxer::create(const QString &url, IOController<Demuxer> &demuxer, FetchTracks *fetchTracks)
{
	const QString scheme = Functions::getUrlScheme(url);
	if (demuxer.isAborted() || url.isEmpty() || scheme.isEmpty())
		return false;
	QString extension = Functions::fileExt(url).toLower();
	for (int i = 0; i <= 1; ++i)
		foreach (Module *module, QMPlay2Core.getPluginsInstance())
			foreach (const Module::Info &mod, module->getModulesInfo())
				if (mod.type == Module::DEMUXER && (mod.name == scheme || (!i ? mod.extensions.contains(extension) : mod.extensions.isEmpty())))
				{
					if (!demuxer.assign((Demuxer *)module->createInstance(mod.name)))
						continue;
					bool canDoOpen = true;
					if (fetchTracks)
					{
						fetchTracks->isOK = true;
						fetchTracks->tracks = demuxer->fetchTracks(url, fetchTracks->isOK);
						if (fetchTracks->isOK) //If tracks are fetched correctly (even if track list is empty)
						{
							if (!fetchTracks->tracks.isEmpty()) //Return tracks list
							{
								demuxer.clear();
								return true;
							}
							if (fetchTracks->onlyTracks) //If there are no tracks and we want only track list - return false
							{
								demuxer.clear();
								return false;
							}
						}
						else //Tracks can't be fetched - an error occured
						{
							fetchTracks->tracks.clear(); //Clear if list is not empty
							canDoOpen = false;
						}
					}
					if (canDoOpen && demuxer->open(url))
						return true;
					demuxer.clear();
					if (mod.name == scheme || demuxer.isAborted())
						return false;
				}
	return false;
}
