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
#include "Crontab.hpp"
#include <memory>
#include "Clock.hpp"


namespace elapse {
namespace crontab {

inline void TimeNormalize(std::tm& tm) {
	auto t = std::mktime(&tm);
	auto p = std::localtime(&t);
	if (p) {
		tm = *p;
	}
}

TimeUnit Crontab::NextExpire(Clock const& clock) {
	auto expire = clock.NowTimeT();
	if (!FindNext(expire, 1)) {
		return 0;
	}
	return ToTimeUnit(expire);
}

bool Crontab::FindNext(std::time_t& timestamp, int offset) const {
	std::tm now;
	timestamp += offset;
	auto p = std::localtime(&timestamp);
	if (!p) {
		return false;
	}
	now = *p;

	size_t next;
	while (true) {
		// second
		if (!second_.NextFit(now.tm_sec, next)) {
			return false;
		}
		if (now.tm_sec > next) {
			now.tm_sec = next;
			++now.tm_min;
			TimeNormalize(now);
		} else {
			now.tm_sec = next;
		}
		// minute
		if (!minute_.NextFit(now.tm_min, next)) {
			return false;
		}
		if (now.tm_min != next) {
			if (now.tm_min > next) {
				++now.tm_hour;
				TimeNormalize(now);
			}
			now.tm_min = next;
			now.tm_sec = 0;
			continue;
		}
		// hour
		if (!hour_.NextFit(now.tm_hour, next)) {
			return false;
		}
		if (now.tm_hour != next) {
			if (now.tm_hour > next) {
				++now.tm_mday;
				TimeNormalize(now);
			}
			now.tm_hour = next;
			now.tm_min = 0;
			now.tm_sec = 0;
			continue;
		}
		// day (including dow and dom)
		if (!dow_.NextFit(now.tm_wday, next)) {
			return false;
		}
		if (now.tm_wday != next) {
			int deltaDays = next - now.tm_wday;
			if (deltaDays < 0) {
				deltaDays += 7;
			}
			now.tm_mday += deltaDays;
			now.tm_hour = 0;
			now.tm_min = 0;
			now.tm_sec = 0;
			TimeNormalize(now);
			continue;
		}
		if (!dom_.NextFit(now.tm_mday, next)) {
			return false;
		}
		if (now.tm_mday != next) {
			if (now.tm_mday > next) {
				++now.tm_mon;
			}
			now.tm_mday = next;
			now.tm_hour = 0;
			now.tm_min = 0;
			now.tm_sec = 0;
			TimeNormalize(now);
			continue;
		}
		// month
		if (!month_.NextFit(now.tm_mon + 1, next)) {
			return false;
		}
		if (now.tm_mon != next - 1) {
			if (now.tm_mon > next - 1) {
				++now.tm_year;
			}
			now.tm_mon = next - 1;
			now.tm_mday = 1;
			now.tm_hour = 0;
			now.tm_min = 0;
			now.tm_sec = 0;
			TimeNormalize(now);
			continue;
		}
		// year
		if (!year_.NextFit(now.tm_year + 1900, next)) {
			return false;
		}
		if (now.tm_year + 1900 != next) {
			return false;
		}
		break;
	}

	timestamp = std::mktime(&now);
	return true;
}

void Crontab::Parse(size_t hour, size_t minute, size_t second) {
	year_.SetFitsAll();
	month_.SetFitsAll();
	dom_.SetFitsAll();
	dow_.SetFitsAll();
	hour_.SetSingle(hour);
	minute_.SetSingle(minute);
	second_.SetSingle(second);
}

void Crontab::Parse(size_t week, size_t hour, size_t minute, size_t second) {
	year_.SetFitsAll();
	month_.SetFitsAll();
	dom_.SetFitsAll();
	dow_.SetSingle(week == 7 ? 0 : week);
	hour_.SetSingle(hour);
	minute_.SetSingle(minute);
	second_.SetSingle(second);
}

void Crontab::Parse(size_t month, size_t date, size_t hour, size_t minute, size_t second) {
	year_.SetFitsAll();
	month_.SetSingle(month);
	dom_.SetSingle(date);
	dow_.SetFitsAll();
	hour_.SetSingle(hour);
	minute_.SetSingle(minute);
	second_.SetSingle(second);
}

} // namespace crontab
} // namespace elapse