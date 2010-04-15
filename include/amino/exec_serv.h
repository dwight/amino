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
#ifndef AMINO_UTIL_EXECUTOR_SERVICE
#define AMINO_UTIL_EXECUTOR_SERVICE

#include <pthread.h>
#include <string>
#include <stdio.h>
#include <time.h>
#include <cassert>

#include <amino/thread.h>
#include <amino/condition.h>
#include <amino/mutex.h>
#include <amino/lock.h>
#include <amino/cstdatomic>

namespace amino{

    /**
     * @brief This class is intended to be inherited by executors.
     */
    class ExecutorService{
        protected:
            volatile bool fShutdown;
            condition_variable condFinish;
            recursive_mutex f_mutex_finish;
            void notify_finish(){
                unique_lock<recursive_mutex> llock(f_mutex_finish);
                condFinish.notify_all();
            }
        public:
            /**
             * @brief call this method to mark executor as shutdown. No further
             * tasks can be accepted by the executor. Tasks already submitted will
             * be executed.
             */
            virtual void shutdown()=0;

            /**
             * @brief call this method to mark executor as halt. No further
             * tasks can be accepted by the executor. Tasks already submitted
             * but not started will be canceled.
             * TODO: this function shoud return a list of un-started tasks
             */
            virtual void halt()=0;

            virtual bool finished()=0;

            /**
             * @brief Block caller thread until all tasks inside the executor
             * get executed. shutdown() or halt() method must be called before
             * this method.
             */
            virtual void waitTermination(){
#ifdef DEBUG
                cout<<"Begin WaitTermination of ExecutorService\n";
#endif
                if(!fShutdown)
                    throw std::logic_error(
                            "Call shutdown()/halt() before wait "
                            "termination of this executor!\n");

                unique_lock<recursive_mutex> llock(f_mutex_finish);

                while(!finished())
                    condFinish.wait(llock);

#ifdef DEBUG
                assert(finished());
                cout<<"Finish WaitTermination\n";
#endif
                return;
            }

            /**
             * @brief Block caller thread until all tasks inside the executor
             * get executed or timeout is reached. Shutdown() or halt() method
             * must be called before this method.
             *
             * @param timeOut timeout in millisecond
             *
             * @return true if executor is terminated. Or false if timeout is
             * reached.
             */
            virtual bool waitTermination(int timeOut){
                if(!fShutdown)
                    throw std::logic_error(
                            "Call shutdown()/halt() before wait "
                            "termination of this executor!\n");

                unique_lock<recursive_mutex> llock(f_mutex_finish);
                if(!finished()){
                    condFinish.timed_wait(llock, timeOut);

                    return finished();
                }

                return finished();
            }

            virtual ~ExecutorService(){
            }
    };
}
#endif
