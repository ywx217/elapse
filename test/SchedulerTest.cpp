#include "gtest/gtest.h"
#include <list>
#include "Scheduler.hpp"
#include "TreeJobContainer.hpp"

using namespace elapse;

class EmptyCallback : public ExpireCallback {
public:
	virtual void operator()(JobId id) override {
		// do nothing.
	}
};

TEST(Scheduler, Init) {
	Scheduler<std::string> s(new TreeJobContainer());
	s.ScheduleLambda("foo", 100, [](JobId id) {
		EXPECT_TRUE(true);
	});
}


TEST(Scheduler, NormalSchedule) {
	Scheduler<int> s(new TreeJobContainer());
	size_t counter = 0;
	for (int i = 0; i < 10; ++i) {
		s.ScheduleWithDelayLambda(i, (i + 1) * 100, [&counter](JobId id) {
			++counter;
		});
	}
	for (int i = 0; i < 10; ++i) {
		s.Advance(98); s.Tick();
		ASSERT_EQ(i, counter);
		s.Advance(2); s.Tick();
		ASSERT_EQ(i + 1, counter);
	}
}

TEST(Scheduler, CancelInCB) {
	Scheduler<int> s(new TreeJobContainer());
	size_t counter = 0;
	for (int i = 0; i < 10; ++i) {
		s.ScheduleWithDelayLambda(i, i * 10, [&s, &counter, i](JobId id) {
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
		s.ScheduleWithDelayLambda(i, 10, [&s, &counter](JobId id) {
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
		s.ScheduleWithDelayLambda(i, 10, [&s, &counter](JobId id) {
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
	s.ScheduleWithDelayLambda(1, 10, [&s, &counter](JobId id) {
		++counter;
		s.Cancel(1);
	});
	s.Advance(8); s.Tick();
	ASSERT_EQ(0, counter);
	s.Advance(2); s.Tick();
	ASSERT_EQ(1, counter);
}

TEST(Scheduler, ScheduleInCB) {
	Scheduler<int> s(new TreeJobContainer());
	size_t counter = 0;
	ECPtr pCB;
	auto cb = [&counter, &s, &pCB](JobId id) {
		s.ScheduleWithDelayLambda(1, 10, [&pCB](JobId id) {
			(*pCB)(id);
		});
		++counter;
	};
	pCB = WrapLambdaPtr(cb);
	s.ScheduleWithDelayLambda(1, 10, cb);

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
	ECPtr pCB = nullptr;
	auto cb = [&counter, &s, &pCB](JobId id) {
		s.Cancel(1);
		s.ScheduleWithDelayLambda(1, 0, [&pCB](JobId id) {
			(*pCB)(id);
		});
		++counter;
	};
	pCB = WrapLambdaPtr(cb);
	s.ScheduleWithDelayLambda(1, 10, cb);

	for (int i = 0; i < 1024; ++i) {
		ASSERT_EQ(i, counter);
		s.Advance(10);
		s.Tick();
	}
}

TEST(Scheduler, CrontabSchedule) {
	Scheduler<std::string> s(new TreeJobContainer());
	size_t counter = 0;

	auto cron = std::make_shared<crontab::Crontab>();
	cron->SetAll();
	cron->Second().Clear().SetSingle(0);
	s.ScheduleRepeatLambda("foo", cron, [&counter](JobId id) { ++counter; });

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

	auto cron = std::make_shared<crontab::Crontab>();
	cron->SetAll();
	cron->Second().Clear().SetSingle(0);
	s.ScheduleRepeatLambda(alias, cron, [&counter, &s, &alias](JobId id) {
		++counter;
		if (counter >= 100) {
			s.Cancel(alias);
		}
	});

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

	auto cron = std::make_shared<crontab::Crontab>();
	cron->SetAll();
	cron->Second().Clear().SetSingle(0);
	s.ScheduleRepeatLambda(alias, cron, [&counter, &s, &alias](JobId id) {
		++counter;
		if (counter >= 100) {
			s.ScheduleWithDelayLambda(alias, 100, [&counter](JobId id) { ++counter; });
		}
	});

	for (int i = 0; i < 1024; ++i) {
		if (i < 100) {
			s.Advance(60 * 1000);
			s.Tick();
			ASSERT_EQ(i + 1, counter);
		} else if (i == 100) {
			s.Advance(98); s.Tick();
			ASSERT_EQ(100, counter);
			s.Advance(2); s.Tick();
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

	s.ScheduleRepeatLambda(1, repeatable, [&s, &counter](JobId id) {
		++counter;
	});

	for (int i = 0; i < 10; ++i) {
		s.Advance(98); s.Tick();
		ASSERT_EQ(i, counter);
		s.Advance(2); s.Tick();
		ASSERT_EQ(i + 1, counter);
	}
	s.Advance(10000); s.Tick();
	ASSERT_EQ(10, counter);
}

TEST(Scheduler, CycleScheduleAndCancel) {
	Scheduler<int> s(new TreeJobContainer());
	size_t counter = 0;
	auto repeatable = std::make_shared<crontab::Cycle>(100, 10);

	s.ScheduleRepeatLambda(1, repeatable, [&s, &counter](JobId id) {
		if (++counter >= 5) {
			s.Cancel(1);
		}
	});

	for (int i = 0; i < 5; ++i) {
		s.Advance(98); s.Tick();
		ASSERT_EQ(i, counter);
		s.Advance(2); s.Tick();
		ASSERT_EQ(i + 1, counter);
	}
	s.Advance(10000); s.Tick();
	ASSERT_EQ(5, counter);
}

class ConstructCounter {
public:
	ConstructCounter(size_t& copyCount, size_t& moveCount) : copy_(copyCount), move_(moveCount) {}
	ConstructCounter(ConstructCounter&& c) : copy_(c.copy_), move_(c.move_) {
		++move_;
	}
	ConstructCounter(ConstructCounter const& c) : copy_(c.copy_), move_(c.move_) {
		++copy_;
	}

private:
	size_t& copy_;
	size_t& move_;
};

TEST(Scheduler, LambdaCopyCount) {
	Scheduler<int> s(new TreeJobContainer());
	size_t nCopy = 0, nMove = 0;
	ConstructCounter c(nCopy, nMove);
	s.ScheduleWithDelayLambda(1, 100, [c](JobId id) {
		return;
	});
	s.Advance(1000); s.Tick();
	std::cout << "copy=" << nCopy << " move=" << nMove << std::endl;
}

#if 1
TEST(Scheduler, BenchAdd) {
	Scheduler<int> s(new TreeJobContainer());
	std::vector<int> v;
	int size = 8;
	v.resize(size);
	for (int n = 0; n < 10; ++n) {
		for (int i = 0; i < 100000; ++i) {
			s.ScheduleWithDelayLambda(i, i, [&s, v, size](JobId id) {});
		}
	}
}

TEST(Scheduler, BenchTick) {
	Scheduler<int> s(new TreeJobContainer());
	std::vector<int> v;
	int size = 8;
	v.resize(size);
	for (int n = 0; n < 1; ++n) {
		for (int i = 0; i < 1000; ++i) {
			for (int j = 0; j < 1000; ++j) {
				s.ScheduleWithDelayLambda(j, j, [&s, v, size](JobId id) {});
			}
			for (int j = 0; j < 1000; ++j) {
				s.Advance(1); s.Tick();
			}
		}
	}
}
#endif
