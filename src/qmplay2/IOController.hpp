// SPDX-FileCopyrightText: 2010-2026 Błażej Szczygieł
// SPDX-License-Identifier: LGPL-3.0-or-later

#pragma once

#include <atomic>

class BasicIO
{
public:
    virtual ~BasicIO() = default;

    virtual void pause() {}
    virtual void abort() {} //must be thread-safe!
};

#include <memory>

template<typename T = BasicIO>
class IOController : public std::shared_ptr<BasicIO>
{
    IOController(const IOController &) = delete;
    IOController &operator=(const IOController &) = delete;

public:
    IOController() = default;

    inline bool isAborted() const
    {
        return br;
    }
    void abort()
    {
        br = true;
        if (std::shared_ptr<BasicIO> strongThis = *this)
            strongThis->abort();
    }
    inline void resetAbort()
    {
        br = false;
    }

    bool assign(BasicIO *ptr)
    {
        if (br)
        {
            reset();
            delete ptr;
            return false;
        }
        std::shared_ptr<BasicIO> sPtr(ptr);
        swap(sPtr);
        return (bool)ptr;
    }

    template<typename objT>
    inline objT *rawPtr()
    {
        return static_cast<objT *>(get());
    }
    inline T *rawPtr()
    {
        return static_cast<T *>(get());
    }

    template<typename objT>
    inline IOController<objT> &toRef()
    {
        return reinterpret_cast<IOController<objT> &>(*this);
    }
    template<typename objT>
    inline IOController<objT> *toPtr()
    {
        return reinterpret_cast<IOController<objT> *>(this);
    }

    inline T *operator ->()
    {
        return static_cast<T *>(get());
    }
    inline const T *operator ->() const
    {
        return static_cast<const T *>(get());
    }
private:
    std::atomic_bool br = false;
};
