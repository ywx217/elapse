#include "gtest/gtest.h"
#include <list>
#include "TreeJobContainer.hpp"
#ifdef BENCHMARK_ASIO_JOB_CONTAINER
#include "AsioJobContainer.hpp"
#endif

using namespace elapse;
#define TIME_BEGIN 1525436318156L

TEST(TreeContainer, InsertAndExpire) {
	TreeJobContainer ctn;
	TimeUnit now = TIME_BEGIN;
	std::list<JobId> jobSequence;
	for (int i = 0; i < 100; ++i, now += 100) {
		jobSequence.push_back(ctn.Add(now, WrapLambdaPtr([&jobSequence](JobId id) {
			ASSERT_EQ(id, jobSequence.front());
			jobSequence.pop_front();
		})));
	}
	now = TIME_BEGIN;
	for (int i = 0; i < 10; ++i, now += 100) {
		ASSERT_EQ(1, ctn.PopExpires(now));
	}
	++now;
	for (int i = 10; i < 20; ++i, now += 100) {
		ASSERT_EQ(1, ctn.PopExpires(now));
	}
	--now;
	now += 100;
	for (int i = 20; i < 100; i += 2, now += 200) {
		ASSERT_EQ(2, ctn.PopExpires(now));
	}
}

TEST(TreeContainer, Remove) {
	TreeJobContainer ctn;
	auto cb = [](JobId id) {};
	auto id_1 = ctn.Add(1, WrapLambdaPtr(cb));
	auto id_2 = ctn.Add(2, WrapLambdaPtr(cb));
	auto id_3 = ctn.Add(3, WrapLambdaPtr(cb));
	auto id_4 = ctn.Add(4, WrapLambdaPtr(cb));

	ASSERT_TRUE(ctn.Remove(id_2));
	ASSERT_FALSE(ctn.Remove(id_2));
	ASSERT_EQ(2, ctn.PopExpires(3));

	ASSERT_FALSE(ctn.Remove(id_3));
	ASSERT_TRUE(ctn.Remove(id_4));
	ASSERT_EQ(0, ctn.PopExpires(1000));
}

TEST(TreeContainer, Iterate) {
	TreeJobContainer ctn;
	auto cb = [](JobId id) {};
	size_t counter = 0;
	ctn.Add(1, WrapLambdaPtr(cb));
	ctn.Add(2, WrapLambdaPtr(cb));
	ctn.Add(3, WrapLambdaPtr(cb));
	ctn.Add(4, WrapLambdaPtr(cb));

	ctn.IterJobs([&counter](Job const& job) { ++counter; return false; });
	ASSERT_EQ(1, counter);
	ctn.IterJobs([&counter](Job const& job) { ++counter; return true; });
	ASSERT_EQ(5, counter);
}

TEST(TreeContainer, RemoveAll) {
	TreeJobContainer ctn;
	auto cb = [](JobId id) {};
	size_t counter = 0;
	ctn.Add(1, WrapLambdaPtr(cb));
	ctn.Add(2, WrapLambdaPtr(cb));
	ctn.Add(3, WrapLambdaPtr(cb));
	ctn.Add(4, WrapLambdaPtr(cb));

	ASSERT_EQ(4, ctn.Size());
	ctn.RemoveAll();
	ASSERT_EQ(0, ctn.Size());
}

TEST(TreeContainer, RemoveIf) {
	TreeJobContainer ctn;
	auto cb = [](JobId id) {};
	size_t counter = 0;
	ctn.Add(1, WrapLambdaPtr(cb));
	ctn.Add(2, WrapLambdaPtr(cb));
	ctn.Add(3, WrapLambdaPtr(cb));
	ctn.Add(4, WrapLambdaPtr(cb));

	ASSERT_EQ(4, ctn.Size());
	ctn.RemoveJobs([](Job const& job) { return job.id_ % 2 == 0; });
	ASSERT_EQ(2, ctn.Size());
	ctn.RemoveJobs([](Job const& job) { return job.id_ % 2 == 1; });
	ASSERT_EQ(0, ctn.Size());
}


#ifdef BENCHMARK_ASIO_JOB_CONTAINER
TEST(Scheduler, BenchTreeJobContainer) {
	TreeJobContainer ctn;
	ExpireCallback cb = [](JobId id) {};

	for (int i = 0; i < 1000000; ++i) {
		ctn.Add(i, cb);
	}
}


TEST(Scheduler, BenchAsioJobContainer) {
	AsioJobContainer ctn;
	ExpireCallback cb = [](JobId id) {};

	for (int i = 0; i < 1000000; ++i) {
		ctn.Add(i, cb);
	}
}
#endif
