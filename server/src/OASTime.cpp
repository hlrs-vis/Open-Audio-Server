
#ifdef WIN32

#include <WinSock2.h>
#include <Windows.h>
#include <time.h>
#include <sysinfoapi.h>

#endif
#include <OASTime.h>
using namespace oas;
#ifdef WIN32

int oas::clock_gettime(int, struct timespec* spec)      //C-file part
{
	__int64 wintime; GetSystemTimeAsFileTime((FILETIME*)&wintime);
	wintime -= 116444736000000000i64;  //1jan1601 to 1jan1970
	spec->tv_sec = wintime / 10000000i64;           //seconds
	spec->tv_nsec = wintime % 10000000i64 * 100;      //nano-seconds
	return 0;
}
#endif

double Time::asDouble() const
{
    return (getSeconds() +  ((double) getNanoseconds() / OAS_BILLION));
}

Time& Time::operator=(const Time &rhs)
{
    if (this != &rhs)
    {
        _time.tv_sec = rhs.getSeconds();
        _time.tv_nsec = rhs.getNanoseconds();
    }

    return *this;
}

Time& Time::operator +=(const Time &rhs)
{
    long int nanoseconds = _time.tv_nsec + rhs.getNanoseconds();

    _time.tv_nsec = nanoseconds % OAS_BILLION;
    _time.tv_sec += rhs.getSeconds() + (nanoseconds / OAS_BILLION);

    return *this;
}

Time& Time::operator-=(const Time &rhs)
{
    if (_time.tv_nsec < rhs.getNanoseconds())
    {
        _time.tv_sec -= 1;
        _time.tv_nsec += OAS_BILLION;
    }

    _time.tv_nsec -= rhs.getNanoseconds();
    _time.tv_sec -= rhs.getSeconds();

    return *this;
}

Time Time::operator +(const Time &other) const
{
    return Time(*this) += other;
}

Time Time::operator -(const Time &other) const
{
	return Time(*this) -= other;
}

bool Time::operator >(const Time &other) const
{
	if (getSeconds() > other.getSeconds()
		|| (getSeconds() == other.getSeconds() && getNanoseconds() > other.getNanoseconds()))
		return true;
	else
		return false;
}

bool Time::operator ==(const Time &other) const
{
	if (getNanoseconds() == other.getNanoseconds()
		&& getSeconds() == other.getSeconds())
	{
		return true;
	}
	else
		return false;
}

bool Time::operator >=(const Time &other) const
{
	if ((*this) > other
		|| (*this) == other)
	{
		return true;
	}
	else
	{
		return false;
	}
}

Time::Time(long int seconds, long int nanoseconds)
{
	_time.tv_sec = seconds;
	_time.tv_nsec = nanoseconds;
}

Time::Time(double floatingRepresentationInSeconds)
{
    _time.tv_sec = (long int) floatingRepresentationInSeconds;
    _time.tv_nsec = (long int) ((floatingRepresentationInSeconds - _time.tv_sec) * OAS_BILLION);
}

Time::Time()
{
	reset();
}

