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
 *  08-07-18  ganzhi     N/A        Initial version
 */
#ifndef AMINO_UTIL_CONDITION
#define AMINO_UTIL_CONDITION

#include <time.h>
#include <pthread.h>
#include <amino/mutex.h>
#include <amino/lock.h>

namespace amino{
    /**
     * This is a class which implements Condition variable;
     * 
     * @author Zhi Gan(ganzhi@gmail.com) 
     */
    class condition_variable{
        private:
            pthread_cond_t fCond;
        public:
            typedef pthread_cond_t native_handle_type;

            condition_variable(){
                pthread_cond_init(&fCond, NULL);
            }

            ~condition_variable(){
                 pthread_cond_destroy(&fCond);
            }

            native_handle_type native_handle(){
                return fCond;
            }

            /**
             * @brief block current thread until notification is fired by other
             * threads.
             *
             * Before calling wait(long) function, thread must already
             * hold the mutex specified in argument.
             *
             * @param lock An lock which is alread hold by current thread.
             * This lock will be released when thread is blocked waiting. And
             * it will be re-acquired after the wait is finished.
             */
            template<typename mutexT>
            void wait(unique_lock<mutexT>& lock){
                pthread_cond_wait(&fCond, lock.mutex()->native_handle());
            } 

            void notify_one(){
                pthread_cond_signal(&fCond);
            }

            void notify_all(){
                pthread_cond_broadcast(&fCond);
            }

            /**
             * @brief block current thread until notification is fired by other
             * threads.
             *
             * Before calling timed_wait(long) fucntion, thread must already
             * acquired the mutex
             *
             * @param lock An lock which is alread hold by current thread.
             * This lock will be released when thread is blocked waiting. And
             * it will be re-acquired after the wait is finished.
             *
             * @param milli time in milliseconds to wait until notified
             *
             * @return true if condition vairable is notified. Or false if
             * timout is reached Or false if timout is reached.
             */
            template<typename mutexT>
            bool timed_wait(unique_lock<mutexT>&lock, long milli){ 
                struct timespec timeout;
                clock_gettime(CLOCK_REALTIME, &timeout);
                timeout.tv_sec += milli/1000;
                timeout.tv_nsec += (milli%1000)*1000000;
                return 0==pthread_cond_timedwait(&fCond, 
                        lock.mutex()->native_handle(), &timeout);
            }
    };
}

#endif
