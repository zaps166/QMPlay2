#ifndef MODULECOMMON_HPP
#define MODULECOMMON_HPP

#include <Module.hpp>

class ModuleCommon
{
public:
	virtual bool set()
	{
		return true;
	}
protected:
	inline ModuleCommon() :
		module(NULL) {}
	inline ~ModuleCommon()
	{
		if (module)
		{
			module->mutex.lock();
			module->instances.removeOne(this);
			module->mutex.unlock();
		}
	}

	inline void SetModule(Module &m)
	{
		if (!module)
		{
			module = &m;
			module->mutex.lock();
			module->instances.append(this);
			module->mutex.unlock();
			set();
		}
	}

	inline Settings &sets()
	{
		return *module;
	}

	inline Module &getModule()
	{
		return *module;
	}

	template<typename T>
	inline void SetInstance()
	{
		module->SetInstance<T>();
	}
private:
	Module *module;
};

#endif //MODULECOMMON_HPP
