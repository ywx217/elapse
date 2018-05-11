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
#include "Clock.hpp"
#include <chrono>
#ifdef DEBUG_PRINT
#include <iostream>
#endif


namespace elapse {

TimeUnit ToTimeUnit(std::time_t tm) {
	return tm * 1000;
}

TimeUnit Clock::Now() const {
	return std::chrono::duration_cast<std::chrono::milliseconds>(TimePoint().time_since_epoch()).count();
}

std::time_t Clock::NowTimeT() const {
	return std::chrono::system_clock::to_time_t(TimePoint());
}

std::chrono::time_point<Clock::clock_source> Clock::TimePoint() const {
	return clock_source::now() + std::chrono::milliseconds(offsetInMillis_);
}

void Clock::Advance(TimeOffset delta) {
#ifdef DEBUG_PRINT
	auto before = Now();
#endif
	offsetInMillis_ += delta;
#ifdef DEBUG_PRINT
	std::cout << "  clock adjust " << before << " -> " << Now() << std::endl;
#endif
}

} // namespace elapse