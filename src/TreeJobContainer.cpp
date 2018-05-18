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
#include "TreeJobContainer.hpp"
#ifdef DEBUG_PRINT
#include <iostream>
#endif


namespace elapse {

JobId TreeJobContainer::Add(TimeUnit expireTime, ECPtr&& cb) {
	JobId id = nextId_++;
	while (nextId_ == 0 || jobs_.find(nextId_) != jobs_.end()) {
		++nextId_;
	}
	jobs_.emplace(id, expireTime, std::move(cb));
	#ifdef DEBUG_PRINT
	std::cout << "  + job-" << id << " expire=" << expireTime << std::endl;
	#endif
	return id;
}

bool TreeJobContainer::Remove(JobId handle) {
	auto it = Find<id>(handle);
	if (it == jobs_.end()) {
		return false;
	}
	#ifdef DEBUG_PRINT
	std::cout << "  - job-" << it->id_ << " removed" << std::endl;
	#endif
	jobs_.erase(it);
	return true;
}

void TreeJobContainer::RemoveAll() {
	jobs_.clear();
}

size_t TreeJobContainer::PopExpires(TimeUnit now) {
	size_t nExpires = 0;
	JobId expiredId;
	auto& expireIndex = boost::multi_index::get<expire>(jobs_);
	auto& idIndex = boost::multi_index::get<id>(jobs_);
	while (true) {
		auto it = expireIndex.begin();
		if (it == expireIndex.end() || !it->IsExpired(now)) {
			break;
		}
		#ifdef DEBUG_PRINT
		std::cout << "[" << now << "] - job-" << it->id_ << " fired" << std::endl;
		#endif
		expiredId = it->id_;
		it->Fire();
		idIndex.erase(expiredId);
		++nExpires;
	}
	return nExpires;
}

void TreeJobContainer::IterJobs(JobPredicate pred) const {
	for (auto const& it : jobs_) {
		if (!pred(it)) {
			break;
		}
	}
}

void TreeJobContainer::RemoveJobs(JobPredicate pred) {
	auto& idIndex = boost::multi_index::get<id>(jobs_);
	for (auto it = idIndex.begin(); it != idIndex.end();) {
		if (pred(*it)) {
			it = idIndex.erase(it);
		} else {
			++it;
		}
	}
}

} // namespace elapse