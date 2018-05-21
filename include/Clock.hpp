#pragma once
/*
Author: ywx217@gmail.com

This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/
#include <ctime>
#include <chrono>
#include <memory>
#include "JobCommons.hpp"


namespace elapse {

typedef std::int64_t TimeOffset;

TimeUnit ToTimeUnit(std::time_t tm);

class Clock {
public:
	typedef std::chrono::system_clock clock_source;
	typedef std::chrono::time_point<clock_source> time_point;

public:
	Clock() : offsetInMillis_(0) {}
	virtual ~Clock() {}

	// get current clock time
	virtual TimeUnit Now() const;
	virtual std::time_t NowTimeT() const;
	virtual time_point TimePoint() const;
	// adjust clock with advance
	virtual void Advance(TimeOffset delta);

private:
	TimeOffset offsetInMillis_;
};

template <class T>
struct LazyValue {
	bool isDirty;
	T value;

	LazyValue() : isDirty(true) {}
};

class LazyClock : public Clock {
public:
	LazyClock() { Refresh(); }
	virtual ~LazyClock() {}

	virtual TimeUnit Now() const override;
	virtual std::time_t NowTimeT() const override;
	virtual time_point TimePoint() const override;
	virtual void Advance(TimeOffset delta) override;

private:
	void Refresh();

private:
	TimeUnit lazyNow_;
	std::time_t lazyTimeT_;
	time_point lazyTimePoint_;
};

} // namespace elapse