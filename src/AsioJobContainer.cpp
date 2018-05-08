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
#include "AsioJobContainer.hpp"


namespace elapse {

JobId AsioJobContainer::Add(TimeUnit expireTime, ExpireCallback const& cb) {
	JobId id = nextId_++;
	sync_timer_ptr p(new sync_timer(id, expireTime, cb));
	timer_set_.insert(p);
	timer_map_.insert(std::make_pair(id, p));
	return id;
}

bool AsioJobContainer::Remove(JobId handle) {
	auto it = timer_map_.find(handle);
	if (it != timer_map_.end()) {
		timer_set_.erase(it->second);
		timer_map_.erase(it);
		return true;
	} else {
		return false;
	}
}

void AsioJobContainer::RemoveAll() {
	timer_set_.clear();
	timer_map_.clear();
}

size_t AsioJobContainer::PopExpires(TimeUnit now) {
	size_t count = 0;
	while (!timer_set_.empty()) {
		sync_timer_ptr now_item = *timer_set_.begin();
		if (now < now_item->expire_time_) {
			break;
		}
		timer_set_.erase(now_item);
		timer_map_.erase(now_item->id_);
		now_item->cb_(now_item->id_);
		++count;
	}
	return count;
}

} // namespace elapse