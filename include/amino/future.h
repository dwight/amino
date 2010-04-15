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
 *  08-08-18  ganzhi     N/A        Initial version
 */
#ifndef AMINO_FUTURE_H
#define AMINO_FUTURE_H

#include <assert.h>

#include <amino/condition.h>
#include <amino/mutex.h>

#ifdef DEBUG
#include <sstream>
#include <iostream>
using namespace std;
#endif 

namespace amino{

    /**
     * @brief This class reprensents a return value from Runnable object.
     *
     * Since the Runnable object can run in other threads, it's very possible
     * that its return value is not availabe to its caller. It's convenient to have
     * a class to represent the future result of Runnable objects.
     */
    class Future{
        public:
            /**
             * @brief This method will block current thread until the result is
             * available.
             *
             * TODO: Make this a template member function so that it can return any
             * type?
             *
             * @return Return value from Runnable.run() method.
             */
            virtual void * get()=0;

            /**
             * @brief this method will block current thread until result becomes
             * available or timeout is reached.
             *
             * @param milli timout value in milli-second
             * @param result this pointer is used to return result of Runnable.run() method
             *
             * @return true if succed, false if timeout is reached.
             */
            virtual bool get(int milli, void ** result=NULL)=0;

            virtual ~Future(){}
    };

    /**
     * @brief This is a base class for other Future implementations.
     *
     * This class provides basic wait/notify mechanism for handling
     * notification. It can be used as parent classes is the wait/notify
     * mechanism is required.
     */
    class AbstractFuture:public Future{
        private:
            /**
             * @brief internal condition variable which is used to notify
             * result catcher of this Future object
             */
            condition_variable cond;

            /**
             * @brief This mutex is used to protect the wait/notify process for
             * implementing get() method.
             */
            recursive_mutex f_mutex;
            /**
             * @brief true when computation is finished and result is ready
             */
            volatile bool isAvail;
        protected: 
            /**
             * @brief result from task running
             */
            volatile void * f_result;

            /**
             * @brief subclass must call this function to notify that computation is finished.
             *
             * Note: get() method might wait infinitely if this method is not called
             */
            virtual void fireEvent(){
                unique_lock<mutex> llock(f_mutex);
                setAvailable(true);
                cond.notify_all();
            }

            /**
             * @brief Set the available flag.
             *
             * @param True if the result is ready
             */
            virtual void setAvailable(bool flag){
                unique_lock<mutex> llock(f_mutex);
                isAvail = flag;
            } 

            /**
             * @brief return the Available flag
             */
            virtual bool getAvailable(){
                unique_lock<mutex> llock(f_mutex);
                return isAvail;
            }
        public:
            AbstractFuture(){
                isAvail = false;
            }

            virtual ~AbstractFuture(){
            }

            /**
             * @brief This method will block current thread until the result is
             * available.
             *
             * @return Return value from Runnable.run() method.
             */
            virtual void * get(){
                while(!getAvailable()){
                    unique_lock<mutex> llock(f_mutex);
                    if(!getAvailable()){
                        cond.wait(llock);
                    }
                    else
                        break;
                }
#ifdef DEBUG_FUTURE
                stringstream ss;
                ss <<"IsAvail in get(): "<<isAvail<<this<<endl;
                cout << ss.str(); 
#endif
                assert(isAvail);
                return (void *) f_result;
            }

            /**
             * @brief this method will block current thread until result becomes
             * available or timeout is reached.
             *
             * @param milli timout value in milli-second
             * @param result this pointer is used to return result of Runnable.run() method
             *
             * @return true if succed, false if timeout is reached.
             */
            virtual bool get(int milli, void ** result=NULL){
                bool res=true;

                if(!isAvail){
                    unique_lock<mutex> llock(f_mutex);
                    res = cond.timed_wait(llock, milli);
                    if(res){
                        if(result!=NULL)
                            *result = (void *)f_result;
                    }
                    return res;
                }

                if(result!=NULL)
                    *result = (void *) f_result;

                return res;
            }
    };
};
#endif
