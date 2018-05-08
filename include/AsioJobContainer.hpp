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
#include <set>
#include <unordered_map>
#include <memory>
//#include <boost/pool/pool_alloc.hpp>
#include "Job.hpp"
#include "JobContainer.hpp"


namespace elapse {

class sync_timer {
public:
	sync_timer(JobId id, TimeUnit expire_time, ExpireCallback const& cb) : id_(id), expire_time_(expire_time), cb_(cb) {}
	virtual ~sync_timer() {}

public:
	JobId id_;
	TimeUnit expire_time_;
	ExpireCallback cb_;
};

typedef std::shared_ptr<sync_timer> sync_timer_ptr;

struct set_cmp {
	bool operator()(sync_timer_ptr const& x, sync_timer_ptr const& y) const {
		if (x->id_ == y->id_) {
			return false;
		}
		if (x->expire_time_ != y->expire_time_) {
			return x->expire_time_ < y->expire_time_;
		}
		return x->id_ < y->id_;
	}
};

// a job container just for comparison
class AsioJobContainer : public JobContainer {
public:
	typedef std::unordered_map<JobId, sync_timer_ptr> timer_map;
	typedef std::vector<sync_timer_ptr> timer_vec;
	//typedef boost::fast_pool_allocator<sync_timer_ptr> pool_type;

public:
	AsioJobContainer() {}
	virtual ~AsioJobContainer() {}

	virtual JobId Add(TimeUnit expireTime, ExpireCallback const& cb);
	virtual bool Remove(JobId handle);
	virtual void RemoveAll();
	virtual size_t PopExpires(TimeUnit now);

protected:
	JobId nextId_;
	timer_map timer_map_;
	std::set<sync_timer_ptr, set_cmp> timer_set_;
};

} // namespace elapse