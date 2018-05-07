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
