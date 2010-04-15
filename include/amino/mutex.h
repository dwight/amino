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

#ifndef AMINO_UTIL_MUTEX
#define AMINO_UTIL_MUTEX

#include <pthread.h>
#include <stdexcept>
#include <errno.h>
#include <iostream>
#include <sstream>
#include <amino/cstdatomic>

using namespace std;

namespace amino{
    class condition_variable;

    /** This is a class which implements Mutex. This kind of mutex is not
     * reentrable. If threads try to lock it twice, a deadlock will occur.
     *
     * @author Zhi Gan(ganzhi@gmail.com)
     */
    class mutex{
        protected:
            pthread_mutex_t fMutex;
            pthread_mutexattr_t attr_t;

            /**
             * private constructor for initialize a reentrant mutex
             */
            mutex(bool recursive){
                pthread_mutexattr_init(&attr_t);
                pthread_mutexattr_settype(&attr_t,PTHREAD_MUTEX_RECURSIVE);
                pthread_mutex_init(&fMutex, &attr_t);
            }
        private:
            // forbid copy
            const mutex & operator =(const mutex & mut){return *this;}
            //forbid copy
            mutex(const mutex & mut){};
        public:
            typedef pthread_mutex_t * native_handle_type;

            friend class condition_variable;

            mutex(){
                pthread_mutexattr_init(&attr_t);
                pthread_mutexattr_settype(&attr_t,PTHREAD_MUTEX_ERRORCHECK);
                pthread_mutex_init(&fMutex, &attr_t);
            }

            virtual ~mutex(){
                pthread_mutexattr_destroy(&attr_t);
                pthread_mutex_destroy(&fMutex);
            }

            void lock() volatile{
                int res = pthread_mutex_lock((pthread_mutex_t *)&fMutex);
                if(0!=res){
                    if(res == EDEADLK)
                        throw std::range_error("dead lock");
                    else if(res == EINVAL)
                        throw std::range_error("mutex is not initialized correctly");

#ifdef DEBUG
                    std::stringstream ss;
                    ss<<"Can't lock "<<res<<std::endl;
                    throw std::range_error(ss.str());
#endif
                }
            }

            /**
             * @brief Try to lock the mutex. This function will immediate
             * return no matter succeed or not.
             *
             * @return true if succeed
             */
            bool try_lock() volatile{
                int res = pthread_mutex_trylock((pthread_mutex_t *) &fMutex);
                if(0==res){
                    return true;
                }
                return false;
            }

            void unlock() volatile{
                int res =pthread_mutex_unlock((pthread_mutex_t *)&fMutex);
                if(0!=res){
                    if(EPERM == res)
                        throw std::range_error("Current thread doesn't own this mutex");
                }
            }

            native_handle_type native_handle(){
                return &fMutex;
            }
    };

    /**********************************************
     * This kind of mutex is reentrantable
     *
     * @author Zhi Gan(ganzhi@gmail.com)
     */
    class recursive_mutex:public mutex{
        public:
            recursive_mutex():mutex(true){}
    };
}

#endif
