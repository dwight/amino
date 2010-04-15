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
 *
 *  Change History:
 *
 *  yy-mm-dd  Developer  Defect     Description
 *  --------  ---------  ------     -----------
 *  08-06-28  ganzhi     N/A        Initial implementation
 */
#ifndef AMINO_UTIL_LOCK
#define AMINO_UTIL_LOCK

#include <pthread.h>
#include <iostream>
#include <amino/stdatomic.h>

namespace amino {
    class condition_variable;

    enum LOCK_TYPE {COMMON_LOCK,TRY_LOCK,NO_LOCK};//there maybe some other types

    template <class Mutex> class unique_lock {
        public:
            friend class condition_variable;

            typedef Mutex mutex_type;

            unique_lock() {
                internal_mut = new mutex_type();
                needDel = true;
                lock_count = 0;
            }

            explicit unique_lock(mutex_type& m, LOCK_TYPE t = COMMON_LOCK) {
                internal_mut = &m;
                needDel = false;
                lock_count = 0;
                switch (t) {
                    case COMMON_LOCK:
                        internal_mut->lock();
                        ++lock_count;
                        break;
                    case TRY_LOCK:
                        if (internal_mut->try_lock())
                            ++lock_count;
                        break;
                    case NO_LOCK:
                        break;
                    default:
                        throw std::logic_error("Not a valid lock type");
                }
            }

            void lock() {
#ifndef DEBUG
                //		std::cout << "lock_count:"<< lock_count.load()<< std::endl;
#endif
                internal_mut->lock();
                lock_count++;
            }

            bool try_lock() {
                if (internal_mut->try_lock()) {
                    lock_count++;
#ifndef DEBUG
                    //			std::cout << "lock_count:"<< lock_count.load()<< std::endl;
#endif
                    return true;
                }
                return false;
            }


            void unlock() {
                lock_count--;
                internal_mut->unlock();
#ifndef DEBUG
                //		std::cout << "lock_count:"<< lock_count.load()<< std::endl;
#endif
            }

            bool owns_lock() {
                return lock_count.load() > 0;
            }

            mutex_type* mutex() const {
                return internal_mut;
            }

            virtual ~unique_lock() {
#ifndef DEBUG
                //		std::cout << "lock_count:"<< lock_count.load()<< std::endl;
#endif
                while (owns_lock()) {
                    unlock();
                }
                if (needDel)
                    delete internal_mut;
            }
        private:
            bool needDel;
            atomic_int lock_count;
            mutex_type * internal_mut;
    };
}

#endif
