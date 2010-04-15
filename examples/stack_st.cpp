#include <iostream>

#include <amino/stack.h>

using namespace amino;

int main(int argc, char * argv[]){
    LockFreeStack<int> stack1;

    stack1.push(5);

    int res;
    if(stack1.pop(res)){
        std::cout<<res<<std::endl;
    }

    return 0;
} 
