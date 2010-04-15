/*
 * (c) Copyright 2008, IBM Corporation.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef AMINO_UTIL_THREAD
#define AMINO_UTIL_THREAD

#include <pthread.h>
#include <string>
#include <vector>
#include <stdio.h>

namespace amino {
using namespace std;

/**
 * This class is used as the root of all runnable class.
 * And it should be copiable since Executor will duplicate
 * it.
 *
 * @warning This class should not be used unless using executor framework.
 *
 * Function object should be used in order to be compatible with
 * C++ 0X standard.
 */
class Runnable {
public:
	virtual void* run() = 0;
        virtual ~Runnable() {}
};

/**
 * @ This class is used to wrap OS thread, which allows application logic
 * executes in parallel. Now it supports two kinds of initilization arguments:
 *
 * <ol>
 * <li>Function object</li>
 * <li>Runnable object</li>
 * </ol>
 *
 * We should use <code>Function object</code> in all of the library code unless
 * executor framework is used. Runnable object is only for using with executor
 * framework.
 */
class Thread : public Runnable {
    private:
        bool started;
public:
	typedef pthread_t native_handler_type;
	Thread() :
		runner(NULL) {
		threadId = 0;
	}

	Thread(int id) :
		threadId(id), runner(NULL) {
	}

    /**
     * Create a new thread with a runnable object.
     * @deprecated
     */
	Thread(string n, Runnable *r = NULL) :
		name(n), runner(r) {
            started = false;
	}

    /**
     * @brief Create a new thread from a runnable object.
     *
     * @warning This constructor should not be used unless using executor
     * framework
     */
	Thread(Runnable* r) :
		runner(r) {
            started = false;
	}

    /**
     * @brief Helper function which calls a function object.
     *
     * This function exists for calling function object from a pthread call
     */
	template <class F> static void * callFuncObj(void * ob) {
		//call function object
		(*((F*) (ob)))();
		return NULL;
	}

    /**
     * @brief This constructor will create a thread from a function object
     * directly. The new thread will be start imediately. This class is in
     * compatible with the C++ 0X standard.
     *
     * @param f A function object which will be executed in another thread.
     */
	template <class F> explicit Thread(F& f) {
		pthread_create(&tid, NULL, &callFuncObj<F>, &f);
        started = true;
	}

    /**
     * @brief default run() method, which is intended to be overrided by
     * subclass.
     */
	virtual void* run() {
		printf("Thread::run()\n");
		return NULL;
	}

    /**
     * @brief set internal runner of this thread.
     *
     * Where runner is set, the thread will execute runner->run() instead of
     * its own run() method.
     */
	void setRunner(Runnable *r) {
		this->runner = r;
	}

    /**
     * @brief Start to execute this thread.
     *
     * If the runner is set, it will be executed. If not, the thread.run()
     * method will be executed.
     */
	void start() {
		pthread_create(&tid, NULL, Thread::execute, this);
	}

    /**
     * @brief return OS handler of background OS handler
     *
     */
	native_handler_type native_handle() {
		return tid;
	}

    /**
     * @deprecated
     */
	void setName(string name) {
		this->name = name;
	}

    /**
     * @deprecated
     */
	string getName() {
		return name;
	}

    /**
     * @brief caller thread will be blocked until this thread exits.
     */
	void join() {
		pthread_join(tid, NULL);
	}

    /**
     * @brief this method is used to tell if this thread can be joined.
     * TODO: if a thread is not started, it's not joinable. Modify it!
     */
	bool joinable() const {
		return true;
	}

    /**
     * @brief Detach this thread object with background OS thread.
     */
	void detach() {
		pthread_detach(tid);
	}

	virtual ~Thread() {
        //if(started)
            //pthread_destroy
	}

private:
    /**
     * @brief Helper function which calls runner->run() method if runner is not
     * NULL. If not, this->run() will be executed.
     */
	static void* execute(void* arg) {
		Thread* thread = (Thread*)arg;
		if (thread->runner != NULL) {
			return thread->runner->run();
		} else {
			return thread->run();
		}
	}

protected:
	int threadId;
	pthread_t tid;
	string name;
private:
	Runnable* runner;
};
}
#endif
