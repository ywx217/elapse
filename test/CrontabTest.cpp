#include "gtest/gtest.h"
#include <tuple>
#include "Crontab.hpp"

using namespace elapse::crontab;


typedef std::tuple<int, int, int, int, int, int> DateTuple;

std::time_t MakeTime(int year, int month, int date, int hour, int minute, int second) {
	std::tm tm;
	tm.tm_year = year - 1900;
	tm.tm_mon = month - 1;
	tm.tm_mday = date;
	tm.tm_hour = hour;
	tm.tm_min = minute;
	tm.tm_sec = second;
	return std::mktime(&tm);
}

std::time_t MakeTime(DateTuple dt) {
	int year, month, date, hour, minute, second;
	std::tie(year, month, date, hour, minute, second) = dt;
	return MakeTime(year, month, date, hour, minute, second);
}

void AssertFindNext(Crontab& cron, std::time_t now, std::time_t expect) {
	std::time_t result = now;
	ASSERT_TRUE(cron.FindNext(result));

	std::string sNow(std::asctime(std::localtime(&now)));
	std::string sResult(std::asctime(std::localtime(&result)));
	std::string sExpect(std::asctime(std::localtime(&expect)));
	ASSERT_EQ(sResult, sExpect);
}

TEST(Crontab, EveryMinute) {
	Crontab cron;
	cron.Parse(0, 0, 0);
	cron.Hour().SetFitsAll();
	cron.Minute().SetFitsAll();
	cron.Second().SetSingle(30);

	AssertFindNext(cron, MakeTime(2018, 5, 7, 17, 20, 0), MakeTime(2018, 5, 7, 17, 20, 30));
	AssertFindNext(cron, MakeTime(2018, 5, 7, 17, 20, 30), MakeTime(2018, 5, 7, 17, 21, 0));
	AssertFindNext(cron, MakeTime(2018, 5, 31, 23, 59, 31), MakeTime(2018, 6, 1, 0, 0, 0));
	AssertFindNext(cron, MakeTime(2018, 12, 31, 23, 59, 59), MakeTime(2019, 1, 1, 0, 0, 0));
	AssertFindNext(cron, MakeTime(2019, 2, 28, 23, 59, 59), MakeTime(2019, 3, 1, 0, 0, 0));
}

TEST(Crontab, EverySecond) {
	Crontab cron;
	cron.Parse(12, 0, 0);
	cron.Second().SetFitsAll();
	cron.Minute().SetFitsAll();

	AssertFindNext(cron, MakeTime(2018, 5, 7, 11, 20, 0), MakeTime(2018, 5, 7, 12, 0, 0));
	AssertFindNext(cron, MakeTime(2018, 5, 7, 11, 20, 30), MakeTime(2018, 5, 7, 12, 0, 0));
	AssertFindNext(cron, MakeTime(2018, 5, 7, 12, 59, 59), MakeTime(2018, 5, 8, 12, 0, 0));
}

TEST(Crontab, EveryMonday) {
	Crontab cron;
	cron.Parse(10, 59, 59);
	cron.DayOfWeek()
		.Clear()
		.SetSingle(1)
		;

	AssertFindNext(cron, MakeTime(2018, 5, 7, 10, 59, 58), MakeTime(2018, 5, 7, 10, 59, 59));
	AssertFindNext(cron, MakeTime(2018, 5, 7, 10, 59, 59), MakeTime(2018, 5, 14, 10, 59, 59));
	AssertFindNext(cron, MakeTime(2018, 5, 28, 11, 0, 0), MakeTime(2018, 6, 4, 10, 59, 59));
}

TEST(Crontab, WeekdayAndHour) {
	Crontab cron;
	cron.Parse(9, 0, 0);
	cron.Hour().SetSingle(18);
	cron.DayOfWeek()
		.Clear()
		.SetSingle(1)
		.SetSingle(3)
		.SetSingle(5)
		;

	AssertFindNext(cron, MakeTime(2018, 5, 7, 12, 0, 0), MakeTime(2018, 5, 7, 18, 0, 0));
	AssertFindNext(cron, MakeTime(2018, 5, 7, 19, 0, 0), MakeTime(2018, 5, 9, 9, 0, 0));
}

TEST(Crontab, LeapYearEverySecond) {
	Crontab cron;
	cron.Second().SetFitsAll();
	cron.Minute().SetFitsAll();
	cron.Hour().SetFitsAll();
	cron.DayOfMonth().SetSingle(29);
	cron.DayOfWeek().SetFitsAll();
	cron.Month().SetSingle(2);
	cron.Year().SetFitsAll();

	AssertFindNext(cron, MakeTime(2018, 5, 7, 12, 0, 0), MakeTime(2020, 2, 29, 0, 0, 0));
	AssertFindNext(cron, MakeTime(2020, 2, 29, 0, 0, 0), MakeTime(2020, 2, 29, 0, 0, 1));
}
