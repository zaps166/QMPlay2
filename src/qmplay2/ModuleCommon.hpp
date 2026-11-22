// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <Module.hpp>

class QMPLAY2SHAREDLIB_EXPORT ModuleCommon
{
public:
    virtual bool set();

protected:
    ModuleCommon() = default;
    virtual ~ModuleCommon();

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
    Module *module = nullptr;
};
