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
#include <type_traits>
#include <boost/noncopyable.hpp>
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
	Scheduler(JobContainer* containerPtr) : clock_(new LazyClock()), container_(containerPtr), firingCB_(nullptr) {}
	Scheduler(std::shared_ptr<Clock> clock, std::shared_ptr<JobContainer> containerPtr) : clock_(clock), container_(containerPtr), firingCB_(nullptr) {}
	virtual ~Scheduler() {
		CancelAll();
		if (firingCB_) {
			firingCB_->Release();
			firingCB_ = nullptr;
		}
	}

	// member variable accessing
	map_type const& Jobs() const { return jobs_; }
	JobContainer const& Container() const { return *container_; }
	std::shared_ptr<JobContainer> const& ContainerPtr() const { return container_; }

	// clock manipulation
	void Advance(TimeOffset delta);
	// bookkeeping all scheduled jobs
	void Tick();

	// schedule a new call with delay
	void Schedule(Key const& alias, TimeUnit expireTime, ECPtr&& cb);
	// schedule a new repeated callback
	void ScheduleRepeat(Key const& alias, crontab::RepeatablePtr const& repeatConfig, ECPtr&& cb);
	// cancel a call
	bool Cancel(Key const& alias);
	void CancelAll();
	// check has a callback
	bool HasCallback(Key const& alias) const;

	// --------------------------------------------------
	// enhanced schedule methods
	// --------------------------------------------------
	void ScheduleWithDelay(Key const& alias, TimeUnit delayInMillis, ECPtr&& cb);
	void ScheduleAt(Key const& alias, size_t hour, size_t minute, size_t second, ECPtr&& cb);

	// --------------------------------------------------
	// lambda wrapper
	// --------------------------------------------------
	// schedule a new call with delay
	template <class Functor>
	void ScheduleLambda(Key const& alias, TimeUnit expireTime, Functor&& cb);
	// schedule a new repeated callback
	template <class Functor>
	void ScheduleRepeatLambda(Key const& alias, crontab::RepeatablePtr const& repeatConfig, Functor&& cb);
	template <class Functor>
	void ScheduleWithDelayLambda(Key const& alias, TimeUnit delayInMillis, Functor&& cb);
	template <class Functor>
	void ScheduleAtLambda(Key const& alias, size_t hour, size_t minute, size_t second, Functor&& cb);

protected:
	// replace a call (more effecient than cancel & add)
	bool ReplaceJob(Key const& alias, TimeUnit expireTime, crontab::RepeatablePtr const& repeatConfig, ECPtr&& wrappedCallback);
	// callback triggered, remove from alias map
	bool OnTriggered(Key const& alias, JobId id);

	template <class K, class H>
	friend class ECOneTimeSchedule;
	template <class K, class H>
	friend class ECRepeatSchedule;

protected:
	std::shared_ptr<Clock> clock_;
	map_type jobs_;
	std::shared_ptr<JobContainer> container_;
	ExpireCallback *firingCB_;
};

template <class Key, class Hash>
class ECOneTimeSchedule : public ExpireCallback, private boost::noncopyable {
public:
	ECOneTimeSchedule(Scheduler<Key, Hash> *scheduler, Key const& alias, ECPtr&& cb) :
		scheduler_(scheduler),
		alias_(alias),
		cb_(std::move(cb)) {}
	virtual ~ECOneTimeSchedule() {}

	virtual void operator()(JobId id) override {
		scheduler_->OnTriggered(alias_, id);
		(*cb_)(id);
	}

private:
	Scheduler<Key, Hash> *scheduler_;
	Key alias_;
	ECPtr cb_;
};

template <class Key, class Hash>
class ECRepeatSchedule : public ExpireCallback, private boost::noncopyable {
public:
	ECRepeatSchedule(Scheduler<Key, Hash> *scheduler, Key const& alias, ECPtr&& cb) :
		scheduler_(scheduler),
		alias_(alias),
		cb_(std::move(cb)) {}
	virtual ~ECRepeatSchedule() {}

	virtual void operator()(JobId id) override {
		// TODO: test reschedule in the callback
		if (!scheduler_) {
			return;
		}
		auto it = scheduler_->jobs_.find(alias_);
		if (it == scheduler_->jobs_.end()) {
			return;
		}
		it->second.first = 0;
		scheduler_->firingCB_ = this;
		(*cb_)(id);
		if (!scheduler_) {
			return;
		}
		scheduler_->firingCB_ = nullptr;
		it = scheduler_->jobs_.find(alias_);
		if (it == scheduler_->jobs_.end() || it->second.first != 0 || !it->second.second) {
			return;
		}
		scheduler_->ScheduleRepeat(alias_, it->second.second, std::move(cb_));
	}

	virtual void Release() override {
		scheduler_ = nullptr;
	}

private:
	Scheduler<Key, Hash> *scheduler_;
	Key alias_;
	ECPtr cb_;
};

template <class Key, class Hash>
void Scheduler<Key, Hash>::Advance(TimeOffset delta) {
	clock_->Advance(delta);
}

template <class Key, class Hash>
void Scheduler<Key, Hash>::Tick() {
	auto now = clock_->Now();
	container_->PopExpires(now);
}

template <class Key, class Hash>
void Scheduler<Key, Hash>::Schedule(Key const& alias, TimeUnit expireTime, ECPtr&& cb) {
	ReplaceJob(alias, expireTime, crontab::NullRepeatablePtr, ECPtr(new ECOneTimeSchedule<Key, Hash>(this, alias, std::move(cb))));
}

template <class Key, class Hash>
void Scheduler<Key, Hash>::ScheduleRepeat(
			Key const& alias, crontab::RepeatablePtr const& repeatConfig, ECPtr&& cb) {
	auto expireTime = repeatConfig->NextExpire(*clock_);
	if (!expireTime) {
		Cancel(alias);
		return;
	}
	ReplaceJob(alias, expireTime, repeatConfig, ECPtr(new ECRepeatSchedule<Key, Hash>(this, alias, std::move(cb))));
}

template <class Key, class Hash>
bool Scheduler<Key, Hash>::Cancel(Key const& alias) {
	auto it = jobs_.find(alias);
	if (it == jobs_.end()) {
		return false;
	}
	container_->Remove(it->second.first);
	jobs_.erase(it);
	return true;
}

template <class Key, class Hash>
void Scheduler<Key, Hash>::CancelAll() {
	for (auto const& it : jobs_) {
		container_->Remove(it.second.first);
	}
	jobs_.clear();
}

template <class Key, class Hash>
bool Scheduler<Key, Hash>::HasCallback(Key const& alias) const {
	return jobs_.find(alias) != jobs_.end();
}

template <class Key, class Hash>
void Scheduler<Key, Hash>::ScheduleWithDelay(
			Key const& alias, TimeUnit delayInMillis, ECPtr&& cb) {
	Schedule(alias, clock_->Now() + delayInMillis, std::move(cb));
}

template <class Key, class Hash>
void Scheduler<Key, Hash>::ScheduleAt(
			Key const& alias, size_t hour, size_t minute, size_t second, ECPtr&& cb) {
	crontab::Crontab cron;
	cron.Parse(hour, minute, second);
	auto expireTime = cron.NextExpire(*clock_);
	if (!expireTime) {
		return;
	}
	Schedule(alias, expireTime, std::move(cb));
}

template <class Key, class Hash>
template <class Functor>
void Scheduler<Key, Hash>::ScheduleLambda(Key const& alias, TimeUnit expireTime, Functor&& cb) {
	Schedule(alias, expireTime, ELAPSE_CB_LAMBDA_WRAPPER(cb));
}

template <class Key, class Hash>
template <class Functor>
void Scheduler<Key, Hash>::ScheduleRepeatLambda(Key const& alias, crontab::RepeatablePtr const& repeatConfig, Functor&& cb) {
	ScheduleRepeat(alias, repeatConfig, ELAPSE_CB_LAMBDA_WRAPPER(cb));
}

template <class Key, class Hash>
template <class Functor>
void Scheduler<Key, Hash>::ScheduleWithDelayLambda(Key const& alias, TimeUnit delayInMillis, Functor&& cb) {
	ScheduleWithDelay(alias, delayInMillis, ELAPSE_CB_LAMBDA_WRAPPER(cb));
}

template <class Key, class Hash>
template <class Functor>
void Scheduler<Key, Hash>::ScheduleAtLambda(Key const& alias, size_t hour, size_t minute, size_t second, Functor&& cb) {
	ScheduleAt(alias, hour, minute, second, ELAPSE_CB_LAMBDA_WRAPPER(cb));
}

template <class Key, class Hash>
bool Scheduler<Key, Hash>::ReplaceJob(
			Key const& alias, TimeUnit expireTime, crontab::RepeatablePtr const& repeatConfig, ECPtr&& wrappedCallback) {
	auto id = container_->Add(std::max(expireTime, clock_->Now() + 1), std::move(wrappedCallback));
	bool isInserted;
	typename map_type::iterator it;
	std::tie(it, isInserted) = jobs_.insert(std::make_pair(alias, std::make_pair(id, repeatConfig)));
	if (isInserted) {
		return false;
	}
	container_->Remove(it->second.first);
	it->second = std::make_pair(id, repeatConfig);
	return true;
}

template <class Key, class Hash>
bool Scheduler<Key, Hash>::OnTriggered(Key const& alias, JobId id) {
	auto it = jobs_.find(alias);
	if (it != jobs_.end()) {
		jobs_.erase(it);
		return true;
	}
	return false;
}

} // namespace elapse
