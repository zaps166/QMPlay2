#ifndef TIMESTAMP_HPP
#define TIMESTAMP_HPP

class TimeStamp
{
public:
	inline bool isValid() const
	{
		return _dts == _dts && _pts == _pts;
	}

	inline void set( double dts, double pts, double start_time = 0.0 )
	{
		_dts = dts - start_time;
		_pts = pts - start_time;
	}
	inline void setInvalid()
	{
		_dts = _pts = 0.0 / 0.0;
	}

	inline double operator =( double t )
	{
		return ( _pts = _dts = t );
	}
	inline void operator +=( double t )
	{
		_dts += t;
		_pts += t;
	}
	inline operator double() const
	{
		if ( _dts < 0.0 )
		{
			if ( _pts >= 0.0 )
				return _pts;
		}
		else
			return _dts;
		return 0.0;
	}

	inline double pts() const
	{
		return _pts;
	}
	inline double dts() const
	{
		return _dts;
	}
private:
	double _pts, _dts;
};

#endif //TIMESTAMP_HPP
