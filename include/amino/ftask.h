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
#ifndef AMINO_FUTURE_TASK
#define AMINO_FUTURE_TASK

#include <assert.h>

#include <amino/condition.h>
#include <amino/mutex.h>
#include <amino/future.h>
#include <amino/thread.h>

namespace amino{

    class FutureTask:public AbstractFuture, public Runnable{
        private:
            volatile bool running;
            Runnable * fTask;
        public:
            /**
             * @brief construct a runnable future which will notifier its
             * listener when task is finished.
             *
             * @param task actual task which should be executed to generate result
             */
            FutureTask(Runnable * task){
                running = false;
                fTask = task;
                assert(fTask!=NULL);
            };

            virtual ~FutureTask(){
                if(running)
                    throw logic_error("This task are still running");
            }

            /**
             * @brief Execute the task and notify listener of this future object.
             */
            virtual void* run(){
                running = true;
                f_result = fTask->run();
                running = false;
                fireEvent();
                return NULL;
            }
    };
}
#endif
