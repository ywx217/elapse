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
#include <cstdint>
#include <bitset>
#include <ctime>
#include <memory>
#include "JobCommons.hpp"


namespace elapse {

class Clock;

namespace crontab {

template <std::size_t Bits, std::size_t BaseOffset = 0>
class Field {
public:
	typedef Field<Bits, BaseOffset> MyTy;
public:
	Field() {}
	virtual ~Field() {}

	MyTy& SetFitsAll() {
		fits_.set();
		return *this;
	}

	MyTy& SetSingle(std::size_t idx) {
		if (idx < BaseOffset || idx - BaseOffset >= Bits) {
			return *this;
		}
		fits_[idx - BaseOffset] = true;
		return *this;
	}

	MyTy& SetRange(std::size_t from, std::size_t to) {
		for (; from < to; ++from) {
			if (from < BaseOffset || from - BaseOffset >= Bits) {
				break;
			}
			fits_[from - BaseOffset] = true;
		}
		return *this;
	}

	MyTy& Clear() {
		fits_.reset();
		return *this;
	}

	bool Fits(std::size_t idx) const {
		if (idx < BaseOffset || idx - BaseOffset >= Bits) {
			return false;
		}
		return fits_[idx - BaseOffset];
	}

	bool NextFit(int idx, int& result) const {
		size_t r = 0;
		if (!NextFit(idx, r)) {
			return false;
		}
		result = static_cast<int>(r);
		return true;
	}

	bool NextFit(std::size_t idx, std::size_t& result) const {
		if (idx < BaseOffset || idx - BaseOffset >= Bits) {
			return false;
		}
		for (std::size_t i = idx; i < idx + Bits; ++i) {
			result = (i - BaseOffset) % Bits;
			if (fits_[result]) {
				result += BaseOffset;
				return true;
			}
		}
		// not found
		return false;
	}

protected:
	std::bitset<Bits> fits_;
};

// seconds, minutes in 0-59
typedef Field<60> SecondField;
typedef Field<60> MinuteField;
// hour in 0-23
typedef Field<24> HourField;
// day of month in 1-31
typedef Field<31, 1> DayOfMonthField;
// month in 1-12
typedef Field<12, 1> MonthField;
// day of week in 0-6
typedef Field<7> DayOfWeekField;
// year in 1970-2099
typedef Field<130, 1970> YearField;


class IRepeatable {
public:
	// finds next repeat timestamp in-place
	virtual TimeUnit NextExpire(Clock const& clock) = 0;
};

typedef std::shared_ptr<IRepeatable> RepeatablePtr;
static const RepeatablePtr NullRepeatablePtr(nullptr);


class Crontab : public IRepeatable {
public:
	Crontab() {}
	virtual ~Crontab() {}

	TimeUnit NextExpire(Clock const& clock) override;

	SecondField& Second() { return second_; }
	SecondField const& Second() const { return second_; }
	MinuteField& Minute() { return minute_; }
	MinuteField const& Minute() const { return minute_; }
	HourField& Hour() { return hour_; }
	HourField const& Hour() const { return hour_; }
	DayOfMonthField& DayOfMonth() { return dom_; }
	DayOfMonthField const& DayOfMonth() const { return dom_; }
	DayOfWeekField& DayOfWeek() { return dow_; }
	DayOfWeekField const& DayOfWeek()const { return dow_; }
	MonthField& Month() { return month_; }
	MonthField const& Month()const { return month_; }
	YearField& Year() { return year_; }
	YearField const& Year()const { return year_; }

	bool FindNext(std::time_t& timestamp, int offset = 1) const;
	void Parse(size_t hour, size_t minute, size_t second);
	void Parse(size_t week, size_t hour, size_t minute, size_t second);
	void Parse(size_t month, size_t date, size_t hour, size_t minute, size_t second);
	void ClearAll();
	void SetAll();

protected:
	SecondField second_;
	MinuteField minute_;
	HourField hour_;
	DayOfMonthField dom_;
	DayOfWeekField dow_;
	MonthField month_;
	YearField year_;
};

typedef std::shared_ptr<Crontab> CrontabPtr;


class Cycle :public IRepeatable {
public:
	Cycle(TimeUnit delay, int repeats, TimeUnit firstDelay=0) :
		repeats_(repeats),
		delay_(delay),
		firstDelay_(firstDelay) {
	}
	virtual ~Cycle() {}

	TimeUnit NextExpire(Clock const& clock) override;

protected:
	int repeats_;
	TimeUnit delay_, firstDelay_;
};

} // namespace crontab
} // namespace elapse