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
 *  Change History:
 *
 *  yy-mm-dd  Developer  Defect     Description
 *  --------  ---------  ------     -----------
 *  08-08-04  ganzhi     N/A        Initial version
 */
#ifndef AMINO_UTIL_SINGLE_EXECUTOR
#define AMINO_UTIL_SINGLE_EXECUTOR

#include <pthread.h>
#include <string>
#include <stdio.h>
#include <memory>

#include <amino/thread.h>
#include <amino/condition.h>
#include <amino/mutex.h>
#include <amino/lock.h>
#include <amino/exec_serv.h>
#include <amino/cstdatomic>
#include <amino/future.h>
#include <amino/ftask.h>

namespace amino {
    using namespace std;

    /**
     * @brief An executor which executes task immediately in current thread.
     */
    class SingleExecutor:public ExecutorService{
        private:
            volatile int fTasks;
        public:
            SingleExecutor(){
                fTasks = 0;
                fShutdown= false;
            }

            virtual ~SingleExecutor(){
            }

            /**
             * @brief Execute a task with another thread.
             *
             * @exception std::logic_error when the executor is already shutdown.
             */
            void execute(Runnable* task){
                unique_lock<mutex> llock(f_mutex_finish);
                if(fShutdown)
                    throw std::logic_error("Already shutdown!\n");
                fTasks++;
                llock.unlock();

                task->run();

                llock.lock();
                fTasks--;
                if(fTasks==0)
                    condFinish.notify_all();
                llock.unlock();
            }

            /**
             * @brief Execute a task with one thread from internal thread pool.
             *
             * @param task is a Runnable which can be executed.
             *
             * @return A future object which can be used to query result of task.
             *
             * @exception std::logic_error when the executor is already shutdown.
             */
            auto_ptr<Future> submit(Runnable* task){
                FutureTask * ft = new FutureTask(task);
                execute(ft);

                auto_ptr<Future> pFuture(ft);

                return pFuture;
            }

            /**
             * @brief Mark the executor as shutdown state, which mean executor will
             * throw an exception when execute(Runnable*) or submit(Runnable *) are
             * called in future.
             */
            virtual void shutdown(){
                unique_lock<mutex> llock(f_mutex_finish);
                fShutdown= true;
                llock.unlock();
            }

            virtual void halt(){
                unique_lock<mutex> llock(f_mutex_finish);
                fShutdown= true;
                llock.unlock();
            }
        protected:
            virtual bool finished(){
                return fTasks==0;
            }
    };
}
#endif
