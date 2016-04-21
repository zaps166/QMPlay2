#ifndef IOCONTROLLER_HPP
#define IOCONTROLLER_HPP

class BasicIO
{
public:
	virtual ~BasicIO() {}

	virtual void pause() {}
	virtual void abort() {} //must be thread-safe!
};

#include <QSharedPointer>

template<typename T = BasicIO>
class IOController : public QSharedPointer<BasicIO>
{
	Q_DISABLE_COPY(IOController)
public:
	inline IOController() :
		br(false)
	{}

	inline bool isAborted() const
	{
		return br;
	}
	void abort()
	{
		br = true;
		if (QSharedPointer<BasicIO> strongThis = *this)
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
			clear();
			delete ptr;
			return false;
		}
		QSharedPointer<BasicIO> sPtr(ptr);
		swap(sPtr);
		return (bool)ptr;
	}

	template<typename objT>
	inline objT *rawPtr()
	{
		return static_cast<objT *>(data());
	}
	inline T *rawPtr()
	{
		return static_cast<T *>(data());
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
		return static_cast<T *>(data());
	}
	inline const T *operator ->() const
	{
		return static_cast<const T *>(data());
	}
private:
	volatile bool br;
};

#endif // IOCONTROLLER_HPP
