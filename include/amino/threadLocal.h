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
 *  09-03-26  Zhang Yue             Initial
 */

#ifndef AMINO_THREAD_LOCAL_H
#define AMINO_THREAD_LOCAL_H 

#include <stdexcept>  
#include <pthread.h>

namespace amino {

/**
 * A C++ thread local class, which is similar to ThreadLocal class of JDK.
 */
template<typename T> class ThreadLocal {
private:
    pthread_key_t key;
public:
	ThreadLocal(void(*destroy)(void *)) {
	    int ret = pthread_key_create(&key, destroy);
	    if(ret != 0){
            throw std::logic_error("Create pthread local");
        }
#ifdef DEBUG
        //cout <<"pthread_key_create: " << key << " " << this <<endl;
#endif
	}

	~ThreadLocal() {
#ifdef DEBUG
	    //cout <<"pthread_key_delete: " << key << " " << this <<endl;
#endif
		pthread_key_delete(key);
	}

	T get() {
		return (T) pthread_getspecific(key);
	}

	void set(T t) {
		int res = pthread_setspecific(key, (void *) t);
        if(res!=0)
            throw std::logic_error("Set pthread local");
	}
};

} //End of amino

#endif //AMINO_THREAD_LOCAL_H
