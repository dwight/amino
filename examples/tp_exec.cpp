#include <iostream>

#include <amino/foreach.h>
#include <amino/thread.h>
#include <amino/ftask.h>
#include <amino/tp_exec.h>

#include <sys/time.h>
#include <unistd.h>

using namespace amino;

class TestRunnable:public Runnable {
    private:
        int fNum;

    public:
        TestRunnable(int num):fNum(num){}

        void * run() {
            int i = 0;
            int j = 0;

            for ( ; i<fNum; i++) {
                j = 0;
                for ( ; j<fNum; j++) {
                    cout<<"I'm running!"<<endl;
                }
            }

            return NULL;
        }
};


int 
main(int argc, char * argv[]) {
    // Create the a thread pool executor with 2 threads
    ThreadPoolExecutor tpexec(2);
    TestRunnable trun(30);
    FutureTask ftask(&trun);

    struct timeval t1, t2;
    gettimeofday(&t1, NULL);

    tpexec.execute(&ftask); 
    
    ftask.get();

    tpexec.shutdown();
    tpexec.waitTermination();
    
    gettimeofday(&t2, NULL);

    // calculate the computing time
    int takes1 = (t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec;
     
    cout<<"The execution time is: "<<takes1<<" microseconds!"<<endl;

    return 0;
}

