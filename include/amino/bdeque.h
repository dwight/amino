/*
 * (c) Copyright 2008, IBM Corporation.
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef AMINO_BLOCKING_DEQUE_H
#define AMINO_BLOCKING_DEQUE_H

#include <stdexcept>

#include <amino/deque.h>
#include <amino/mutex.h>
#include <amino/lock.h>
#include <amino/condition.h>

namespace amino {

template<typename T>
class BlockingDeque: public LockFreeDeque<T> {
private:
	mutex f_mutex;
	condition_variable f_push_notifier;
public:
	virtual void pushRight(const T& data) {
		unique_lock<mutex> llock(f_mutex);
		LockFreeDeque<T>::pushRight(data);
		f_push_notifier.notify_all();
		llock.unlock();
	}

	virtual void pushLeft(const T& data) {
		unique_lock<mutex> llock(f_mutex);
		LockFreeDeque<T>::pushLeft(data);
		f_push_notifier.notify_all();
		llock.unlock();
	}

	virtual bool takeRight(T& ret) {
		while (true) {
			if (this->popRight(ret))
				return true;
			//				return this->popRight();

			unique_lock<mutex> llock(f_mutex);
			if (this->empty())
				f_push_notifier.wait(llock);
			llock.unlock();
		}
	}

	virtual bool takeLeft(T& ret) {
		while (true) {
			if (this->popLeft(ret))
				return true;
			//				return this->popLeft();

			unique_lock<mutex> llock(f_mutex);
			if (this->empty())
				f_push_notifier.wait(llock);
			llock.unlock();
		}
	}
};
}

#endif
