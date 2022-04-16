# CXX Make variable for compiler
CXX=g++
# Make variable for compiler options
#	-std=c++11  #C/C++ variant to use, e.g. C++ 2011
#	-g          #include information for symbolic debugger e.g. gdb 
CXXFLAGS=-std=c++17 -g

# Rules format:
# target : dependency1 dependency2 ... dependencyN
#     Command to make target, uses default rules if not specified

# First target is the one executed if you just type make
# make target specifies a specific target
# $^ is an example of a special variable.  It substitutes all dependencies

all : demo1 demo2 demo3

demo3 : demo3.o memlab.o
	$(CXX) -o demo3 $^ -lpthread
	
demo3.o : demo3.cpp
	$(CXX) $(CXXFLAGS) -c demo3.cpp

demo2 : demo2.o memlab.o
	$(CXX) -o demo2 $^ -lpthread
	
demo2.o : demo2.cpp
	$(CXX) $(CXXFLAGS) -c demo2.cpp

demo1 : demo1.o memlab.o
	$(CXX) -o demo1 $^ -lpthread
	
demo1.o : demo1.cpp
	$(CXX) $(CXXFLAGS) -c demo1.cpp

memlab.o : memlab.cpp
	$(CXX) $(CXXFLAGS) -c memlab.cpp
	
