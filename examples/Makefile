all:stack_st stack_mt

stack_st:stack_st.cpp
	g++ -I../include -o stack_st -lpthread stack_st.cpp  

stack_mt:stack_mt.cpp
	g++ -I../include -o stack_mt -lpthread stack_mt.cpp  

clean:
	rm -rf stack_st stack_mt
