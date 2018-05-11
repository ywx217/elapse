#include "gtest/gtest.h"
#include <list>
#include "Scheduler.hpp"
#include "TreeJobContainer.hpp"

using namespace elapse;

TEST(Scheduler, Init) {
	Scheduler<std::string> s(new TreeJobContainer());
	s.Schedule("foo", 100, [](JobId id) {
		EXPECT_TRUE(true);
	});
}


TEST(Scheduler, NormalSchedule) {
	Scheduler<int> s(new TreeJobContainer());
	size_t counter = 0;
	ExpireCallback cb = [&counter](JobId id) {
		++counter;
	};
	for (int i = 0; i < 10; ++i) {
		s.ScheduleWithDelay(i, (i + 1) * 100, cb);
	}
	for (int i = 0; i < 10; ++i) {
		s.Advance(99); s.Tick();
		ASSERT_EQ(i, counter);
		s.Advance(1); s.Tick();
		ASSERT_EQ(i + 1, counter);
	}
}

TEST(Scheduler, CancelInCB) {
	Scheduler<int> s(new TreeJobContainer());
	size_t counter = 0;
	ExpireCallback cb = [&counter](JobId id) {
		++counter;
	};
	for (int i = 0; i < 10; ++i) {
		s.ScheduleWithDelay(i, i * 10, [&s, &counter, i](JobId id) {
			++counter;
			s.Cancel(i);
		});
	}
	for (int i = 0; i < 10; ++i) {
		s.Tick();
		ASSERT_EQ(i + 1, counter);
		s.Advance(10);
	}
}

TEST(Scheduler, CancelAllInCB) {
	Scheduler<int> s(new TreeJobContainer());
	size_t counter = 0;
	for (int i = 0; i < 10; ++i) {
		s.ScheduleWithDelay(i, 10, [&s, &counter](JobId id) {
			++counter;
			for (int i = 9; i >= 0; --i) {
				s.Cancel(i);
			}
		});
	}
	s.Advance(10);
	s.Tick();
	ASSERT_EQ(1, counter);
}

TEST(Scheduler, CancelAllInCB2) {
	Scheduler<int> s(new TreeJobContainer());
	size_t counter = 0;
	for (int i = 0; i < 10; ++i) {
		s.ScheduleWithDelay(i, 10, [&s, &counter](JobId id) {
			++counter;
			for (int i = 0; i < 10; ++i) {
				s.Cancel(i);
			}
		});
	}
	s.Advance(10);
	s.Tick();
	ASSERT_EQ(1, counter);
}

TEST(Scheduler, CancelSingleInCB) {
	Scheduler<int> s(new TreeJobContainer());
	size_t counter = 0;
	s.ScheduleWithDelay(1, 10, [&s, &counter](JobId id) {
		++counter;
		s.Cancel(1);
	});
	s.Advance(9); s.Tick();
	ASSERT_EQ(0, counter);
	s.Advance(1); s.Tick();
	ASSERT_EQ(1, counter);
}

TEST(Scheduler, ScheduleInCB) {
	Scheduler<int> s(new TreeJobContainer());
	size_t counter = 0;
	ExpireCallback *pCB = nullptr;
	ExpireCallback cb = [&counter, &s, &pCB](JobId id) {
		s.ScheduleWithDelay(1, 10, *pCB);
		++counter;
	};
	pCB = &cb;
	s.ScheduleWithDelay(1, 10, cb);
	
	for (int i = 0; i < 1024; ++i) {
		s.Advance(8); s.Tick();
		ASSERT_EQ(i, counter);
		s.Advance(2); s.Tick();
		ASSERT_EQ(i + 1, counter);
	}
}

TEST(Scheduler, ScheduleImmediatlyInCB) {
	Scheduler<int> s(new TreeJobContainer());
	size_t counter = 0;
	ExpireCallback *pCB = nullptr;
	ExpireCallback cb = [&counter, &s, &pCB](JobId id) {
		s.Cancel(1);
		s.ScheduleWithDelay(1, 0, *pCB);
		++counter;
	};
	pCB = &cb;
	s.ScheduleWithDelay(1, 10, cb);
	
	for (int i = 0; i < 1024; ++i) {
		ASSERT_EQ(i, counter);
		s.Advance(10);
		s.Tick();
	}
}

TEST(Scheduler, CrontabSchedule) {
	Scheduler<std::string> s(new TreeJobContainer());
	size_t counter = 0;
	ExpireCallback cb = [&counter](JobId id) { ++counter; };

	auto cron = std::make_shared<crontab::Crontab>();
	cron->SetAll();
	cron->Second().Clear().SetSingle(0);
	s.ScheduleRepeat("foo", cron, cb);

	for (int i = 0; i < 1024; ++i) {
		s.Tick();
		ASSERT_EQ(i, counter);
		s.Advance(60 * 1000);
	}
}

TEST(Scheduler, CrontabCancel) {
	const std::string alias("foo");
	Scheduler<std::string> s(new TreeJobContainer());
	size_t counter = 0;
	ExpireCallback cb = [&counter, &s, &alias](JobId id) {
		++counter;
		if (counter >= 100) {
			s.Cancel(alias);
		}
	};

	auto cron = std::make_shared<crontab::Crontab>();
	cron->SetAll();
	cron->Second().Clear().SetSingle(0);
	s.ScheduleRepeat(alias, cron, cb);

	for (int i = 0; i < 1024; ++i) {
		s.Tick();
		ASSERT_EQ(std::min(100, i), counter);
		s.Advance(60 * 1000);
	}
}

TEST(Scheduler, CrontabScheduleAnother) {
	const std::string alias("foo");
	Scheduler<std::string> s(new TreeJobContainer());
	size_t counter = 0;
	ExpireCallback cb = [&counter, &s, &alias](JobId id) {
		++counter;
		if (counter >= 100) {
			s.ScheduleWithDelay(alias, 100, [&counter](JobId id) { ++counter; });
		}
	};

	auto cron = std::make_shared<crontab::Crontab>();
	cron->SetAll();
	cron->Second().Clear().SetSingle(0);
	s.ScheduleRepeat(alias, cron, cb);

	for (int i = 0; i < 1024; ++i) {
		if (i < 100) {
			s.Advance(60 * 1000);
			s.Tick();
			ASSERT_EQ(i + 1, counter);
		} else if (i == 100) {
			s.Advance(99); s.Tick();
			ASSERT_EQ(100, counter);
			s.Advance(1); s.Tick();
			ASSERT_EQ(101, counter);
		} else {
			s.Advance(100); s.Tick();
			ASSERT_EQ(101, counter);
		}
	}
}

TEST(Scheduler, CycleSchedule) {
	Scheduler<int> s(new TreeJobContainer());
	size_t counter = 0;
	auto repeatable = std::make_shared<crontab::Cycle>(100, 10);

	ExpireCallback cb = [&s, &counter](JobId id) {
		++counter;
	};
	s.ScheduleRepeat(1, repeatable, cb);

	for (int i = 0; i < 10; ++i) {
		s.Advance(99); s.Tick();
		ASSERT_EQ(i, counter);
		s.Advance(1); s.Tick();
		ASSERT_EQ(i + 1, counter);
	}
	s.Advance(10000); s.Tick();
	ASSERT_EQ(10, counter);
}

TEST(Scheduler, CycleScheduleAndCancel) {
	Scheduler<int> s(new TreeJobContainer());
	size_t counter = 0;
	auto repeatable = std::make_shared<crontab::Cycle>(100, 10);

	ExpireCallback cb = [&s, &counter](JobId id) {
		if (++counter >= 5) {
			s.Cancel(1);
		}
	};
	s.ScheduleRepeat(1, repeatable, cb);

	for (int i = 0; i < 5; ++i) {
		s.Advance(99); s.Tick();
		ASSERT_EQ(i, counter);
		s.Advance(1); s.Tick();
		ASSERT_EQ(i + 1, counter);
	}
	s.Advance(10000); s.Tick();
	ASSERT_EQ(5, counter);
}
