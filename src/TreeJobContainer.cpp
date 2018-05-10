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


namespace elapse {

JobId TreeJobContainer::Add(TimeUnit expireTime, ExpireCallback const& cb) {
	JobId id = nextId_++;
	while (nextId_ == 0 || jobs_.find(nextId_) != jobs_.end()) {
		++nextId_;
	}
	jobs_.emplace(id, expireTime, cb);
	return id;
}

bool TreeJobContainer::Remove(JobId handle) {
	auto it = Find<id>(handle);
	if (it == jobs_.end()) {
		return false;
	}
	jobs_.erase(it);
	return true;
}

void TreeJobContainer::RemoveAll() {
	jobs_.clear();
}

size_t TreeJobContainer::PopExpires(TimeUnit now) {
	std::vector<Job> expiredJobs;
	auto& index = boost::multi_index::get<expire>(jobs_);
	auto eraseTo = std::find_if(index.begin(), index.end(), [&expiredJobs, now](Job const& job) {
		// iterate expire time in ascending order
		if (job.IsExpired(now)) {
			expiredJobs.push_back(job);
			return false;
		}
		return true;
	});
	index.erase(index.begin(), eraseTo);

	for (auto const& job : expiredJobs) {
		job.Fire();
	}
	return expiredJobs.size();
}

} // namespace elapse