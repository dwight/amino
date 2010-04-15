#include <iostream>

#include <amino/thread.h>
#include <amino/stack.h>

using namespace amino;

LockFreeStack<int> stack1;

class PushOp{
    public:
        void operator()(){
            for(int i=0;i<1000;i++)
                stack1.push(i);
        }
};

class PopOp{
    public:
        void operator()(){
            for(int i=999;i>=0;i--){
                int res;
                while( !stack1.pop(res) )
                {}

                if(res != i){
                    cout<<"Res: "<<res<<" I: "<<i<<"\n";
                    throw std::logic_error("Output is not correct!\n");
                }
            }
        }
};

int main(int argc, char * argv[]){
    PushOp op1;
    PopOp op2;
    Thread thread1(op1), thread2(op2);

    thread1.join();
    thread2.join();

    return 0;
} 
