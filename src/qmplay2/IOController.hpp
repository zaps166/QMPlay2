/*
    QMPlay2 is a video and audio player.
    Copyright (C) 2010-2022  Błażej Szczygieł

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

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
    volatile bool br = false;
};
