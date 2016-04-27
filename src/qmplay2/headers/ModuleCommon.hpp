#ifndef MODULECOMMON_HPP
#define MODULECOMMON_HPP

#include <Module.hpp>

class ModuleCommon
{
public:
	virtual bool set();
protected:
	inline ModuleCommon() :
		module(NULL)
	{}
	~ModuleCommon();

	void SetModule(Module &m);

	inline Settings &sets()
	{
		return *module;
	}

	inline Module &getModule()
	{
		return *module;
	}

	template<typename T>
	inline void setInstance()
	{
		module->setInstance<T>();
	}
private:
	Module *module;
};

#endif //MODULECOMMON_HPP
