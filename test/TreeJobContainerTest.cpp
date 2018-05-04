#include "gtest/gtest.h"
#include <list>
#include "TreeJobContainer.hpp"

using namespace elapse;
#define TIME_BEGIN 1525436318156L

TEST(TreeContainer, InsertAndExpire) {
	TreeJobContainer ctn;
	TimeUnit now = TIME_BEGIN;
	std::list<JobId> jobSequence;
	for (int i = 0; i < 100; ++i, now += 100) {
		jobSequence.push_back(ctn.Add(now, [&jobSequence](JobId id) {
			ASSERT_EQ(id, jobSequence.front());
			jobSequence.pop_front();
		}));
	}
	now = TIME_BEGIN;
	for (int i = 0; i < 10; ++i, now += 100) {
		ASSERT_EQ(1, ctn.PopExpires(now).size());
	}
	++now;
	for (int i = 10; i < 20; ++i, now += 100) {
		ASSERT_EQ(1, ctn.PopExpires(now).size());
	}
	--now;
	now += 100;
	for (int i = 20; i < 100; i += 2, now += 200) {
		ASSERT_EQ(2, ctn.PopExpires(now).size());
	}
}

TEST(TreeContainer, Remove) {
	TreeJobContainer ctn;
	auto cb = [](JobId id) {};
	auto id_1 = ctn.Add(1, cb);
	auto id_2 = ctn.Add(2, cb);
	auto id_3 = ctn.Add(3, cb);
	auto id_4 = ctn.Add(4, cb);

	ASSERT_TRUE(ctn.Remove(id_2));
	ASSERT_FALSE(ctn.Remove(id_2));
	ASSERT_EQ(2, ctn.PopExpires(3).size());

	ASSERT_FALSE(ctn.Remove(id_3));
	ASSERT_TRUE(ctn.Remove(id_4));
	ASSERT_EQ(0, ctn.PopExpires(1000).size());
}