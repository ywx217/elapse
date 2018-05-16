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
#if !defined(NDEBUG)
#define BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING
#define BOOST_MULTI_INDEX_ENABLE_SAFE_MODE
#endif

#include <set>
#include <unordered_map>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include "Job.hpp"
#include "JobContainer.hpp"


namespace elapse {

// set + map vs boost::mic
// http://david-grs.github.io/why_boost_multi_index_container-part1/

/* tags for accessing the corresponding indices of JobSet */
struct id {};
struct expire {};

/* see Compiler specifics: Use of member_offset for info on
* BOOST_MULTI_INDEX_MEMBER
*/

/* Define a multi_index_container of JobSet with following indices:
*   - a unique index sorted by Job::id_,
*   - a non-unique index sorted by Job::expired_,
*/


typedef boost::multi_index_container<
	Job,
	boost::multi_index::indexed_by<
		boost::multi_index::hashed_unique<
			boost::multi_index::tag<id>, BOOST_MULTI_INDEX_MEMBER(Job, JobId, id_)>,
		boost::multi_index::ordered_non_unique<
			boost::multi_index::tag<expire>, BOOST_MULTI_INDEX_MEMBER(Job, TimeUnit, expire_)> >
> JobSet;

// a job container based on boost::multi_index_container (RB-Tree & unordered map)
class TreeJobContainer : public JobContainer {
public:
	TreeJobContainer() : nextId_(1) {}
	virtual ~TreeJobContainer() {}

	virtual JobId Add(TimeUnit expireTime, ExpireCallback const& cb);
	virtual bool Remove(JobId handle);
	virtual void RemoveAll();
	virtual size_t PopExpires(TimeUnit now);
	virtual void IterJobs(JobPredicate pred) const;
	virtual void RemoveJobs(JobPredicate pred);
	virtual size_t Size() const { return jobs_.size(); }

protected:
	template <class Tag, class Key>
	inline JobSet::iterator Find(Key const& key) {
		return boost::multi_index::get<Tag>(jobs_).find(key);
	}

protected:
	JobId nextId_;
	JobSet jobs_;
};

} // namespace elapse