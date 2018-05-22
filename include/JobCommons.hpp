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
#include <cstdint>
#include <vector>
#include <functional>
#include <memory>


namespace elapse {

typedef std::uint64_t TimeUnit;
typedef std::uint64_t JobId;

class Job;
typedef std::function<bool(Job const&)> JobPredicate;

class ExpireCallback {
public:
	virtual ~ExpireCallback() {}
	virtual void operator()(JobId id) = 0;
	virtual void Release() {}
};

typedef std::unique_ptr<ExpireCallback> ECPtr;

template <class Functor>
class ECLambda : public ExpireCallback {
public:
	ECLambda(Functor&& f) : f_(std::forward<Functor>(f)) {}
	virtual ~ECLambda() {}

	virtual void operator()(JobId id) override {
		f_(id);
	}

private:
	Functor f_;
};

template <class Functor>
inline ECLambda<Functor> WrapLambda(Functor&& f) {
	return ECLambda<Functor>(std::forward<Functor>(f));
}

template <class Functor>
inline ECPtr WrapLambdaPtr(Functor&& f) {
	return ECPtr(new ECLambda<Functor>(std::forward<Functor>(f)));
}

#define ELAPSE_CB_LAMBDA_WRAPPER(varname) ECPtr(new ECLambda<Functor>(std::forward<Functor>(varname)))

} // namespace elapse
