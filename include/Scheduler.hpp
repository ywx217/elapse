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
#include <functional>
#include <unordered_map>
#include <memory>
#include "JobCommons.hpp"
#include "JobContainer.hpp"
#include "Clock.hpp"
#include "Crontab.hpp"


namespace elapse {

// timer scheduler for more convenient uses.
template <class Key, class Hash=std::hash<Key>>
class Scheduler {
public:
	typedef Key key_type;
	typedef std::pair<JobId, crontab::RepeatablePtr> value_type;
	typedef std::unordered_map<Key, value_type, Hash> map_type;

public:
	Scheduler(JobContainer* containerPtr) : container_(containerPtr) {}
	Scheduler(std::unique_ptr<JobContainer>&& containerPtr) : container_(containerPtr) {}
	virtual ~Scheduler() {}

	// clock manipulation
	void Advance(TimeOffset delta) {
		clock_.Advance(delta);
	}

	// member variable accessing
	map_type const& Jobs() const { return jobs_; }
	std::unique_ptr<JobContainer> const& Container() const { return container_; }

	// bookkeeping all scheduled jobs
	void Tick() {
		auto now = clock_.Now();
		container_->PopExpires(now);
	}

	// schedule a new call with delay
	void Schedule(Key const& alias, TimeUnit expireTime, ExpireCallback const& cb) {
		Cancel(alias);
		auto id = container_->Add(expireTime, [cb, alias, this](JobId id) {
			this->OnTriggered(alias, id);
			cb(id);
		});
		jobs_[alias] = std::make_pair(id, crontab::RepeatablePtr(nullptr));
	}

	// schedule a new repeated callback
	void ScheduleRepeat(Key const& alias, crontab::RepeatablePtr repeatConfig, ExpireCallback const& cb) {
		Cancel(alias);
		auto expireTime = repeatConfig->NextExpire(clock_);
		if (!expireTime) {
			return;
		}
		auto id = container_->Add(expireTime, [this, alias, cb](JobId id) {
			// TODO: test reschedule in the callback
			auto it = jobs_.find(alias);
			if (it == jobs_.end()) {
				return;
			}
			it->second.first = 0;
			cb(id);
			it = jobs_.find(alias);
			if (it == jobs_.end() || it->second.first != 0 || !it->second.second) {
				return;
			}
			ScheduleRepeat(alias, it->second.second, cb);
		});
		jobs_[alias] = std::make_pair(id, repeatConfig);
	}

	// cancel a call
	bool Cancel(Key const& alias) {
		auto it = jobs_.find(alias);
		if (it == jobs_.end()) {
			return false;
		}
		container_->Remove(it->second.first);
		jobs_.erase(it);
		return true;
	}

	// check has a callback
	bool HasCallback(Key const& alias) const {
		return jobs_.find(alias) != jobs_.end();
	}

	// --------------------------------------------------
	// enhanced schedule methods
	// --------------------------------------------------
	void ScheduleWithDelay(Key const& alias, TimeUnit delayInMillis, ExpireCallback const& cb) {
		Schedule(alias, clock_.Now() + delayInMillis, cb);
	}

	void ScheduleAt(Key const& alias, size_t hour, size_t minute, size_t second, ExpireCallback const& cb) {
		crontab::Crontab cron;
		cron.Parse(hour, minute, second);
		auto expireTime = cron.NextExpire(clock_);
		if (!expireTime) {
			return;
		}
		Schedule(alias, expireTime, cb);
	}

protected:
	bool OnTriggered(Key const& alias, JobId id) {
		auto it = jobs_.find(alias);
		if (it != jobs_.end()) {
			jobs_.erase(it);
			return true;
		}
		return false;
	}

protected:
	Clock clock_;
	map_type jobs_;
	std::unique_ptr<JobContainer> container_;
};

} // namespace elapse