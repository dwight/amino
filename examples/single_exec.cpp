#include <iostream>

#include <amino/foreach.h>
#include <amino/thread.h>
#include <amino/ftask.h>
#include <amino/single_exec.h>

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
    SingleExecutor sexec;
    TestRunnable trun(10);

    struct timeval t1, t2;
    gettimeofday(&t1, NULL);

    sexec.execute(&trun); 
    sexec.shutdown();
	sexec.waitTermination();
    
    gettimeofday(&t2, NULL);

    // calculate the computing time
    int takes1 = (t2.tv_sec - t1.tv_sec) * 1000000 + t2.tv_usec;
     
    cout<<"The execution time is: "<<takes1<<" microseconds!"<<endl;

    return 0;
}

