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

#ifndef AMINO_WS_SCHEDULER_H
#define AMINO_WS_SCHEDULER_H

#include <stdexcept>
#include <assert.h>

#include <amino/bdeque.h>

#ifdef DEBUG_SCHEDULER
#include <sstream>
using namespace std;
#endif

namespace amino {
/**
 * @brief A work stealing scheduler which store pointers to TaskType.
 *
 * Node: It won't delete any memory space allocated by caller.
 *
 * @tparam TaskType type of the tasks to be scheduled
 */
template<typename TaskType>
class ws_scheduler {
private:
	typedef BlockingDeque<TaskType *>* pBlockingDeque;
	int fThreadNum;
	pBlockingDeque * queues;
	int randInd;
public:
	ws_scheduler(int threadNum) {
		randInd = 0;
		fThreadNum = threadNum;
		queues = new pBlockingDeque[threadNum];
		for (int i = 0; i < threadNum; i++) {
			queues[i] = new BlockingDeque<TaskType *> ();
		}
	}

	virtual ~ws_scheduler() {
		for (int i = 0; i < fThreadNum; i++) {
			delete queues[i];
		}
		delete[] queues;
	}

	/**
	 * @brief put a shutdown signal into the working queues.
	 *
	 * This method puts a NULL pointer into the end of all the
	 * internal task queues. So if a work thread get a task pointer
	 * with NULL value, it should end the loop.
	 */
	void shutdown() {
		for (int i = 0; i < fThreadNum; i++) {
			queues[i]->pushLeft(NULL);
		}
	}

	/**
	 * Add a task into the interal work queues. The task will be added
	 * to a rand queue.
	 *
	 * @param task inserting task
	 */
	void addTask(TaskType * task) {
        addTask(randInd++, task);
	}

	/**
	 * Add a task into the interal work queues. The task will be added
	 * to the queue which specified by function parameter.
	 *
	 * @param index index of the queue which will be used to store
	 * the task
	 * @param task inserting task
	 */
	void addTask(int index, TaskType* task) {
		index = index % fThreadNum;
		queues[index]->pushLeft(task);
	}

	/**
	 * Get a task from internal queues. The queue with an index of
	 * <code>threadId</code> will be checked at first. This method
	 * will block if current queue is empty and it fails to steal.
	 *
	 * @param threadId id of requesting thread
	 * @return a task which should be executed. Or NULL as the end
	 * signal so executor should NOT call getTask() any more.
	 */
	TaskType * getTask(int threadId) {
		pBlockingDeque curQ = queues[threadId];

		TaskType * res;
		if (curQ->popRight(res)) {
			if (res != NULL)
				return res;
			// res == NULL, which means end signal
			if (!curQ->empty()) {
				cerr<< "Current queue is not empty after retrieved end signal\n";
				curQ ->pushLeft(NULL);
				if(curQ->popRight(res))
                    assert(res!=NULL);
				return res;
			}

			return NULL;

		} else {
			return stealing(threadId);
		}
	}
private:
	TaskType * stealing(int threadId) {
		for (int i = 0; i < fThreadNum - 1; i++) {
			TaskType * res;
			if (queues[(i + threadId) % fThreadNum]->popLeft(res)) {
				//NULL means shutdown
				if (res != NULL) {
					return res;
				}

				//We got shutdown single of another queue, we must put it back
#ifdef DEBUGJ
				if(!queues[(i+threadId)%fThreadNum]->empty()) {
					cout<<"Queue is not empty after retrieved end signal\n";
				}
#endif
				// put it back
				queues[(i + threadId) % fThreadNum]->pushLeft(res);
				break;
			}
		}
		TaskType * ret;
		queues[threadId]->takeRight(ret);
		return ret;
	}
};
}
;

#endif
