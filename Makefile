CXX=g++
CXXFLAGS=-O3 -Wall -Werror -std=c++11
LDFLAGS=-L/usr/local/opt/boost/lib/ -pthread
LDLIBS=-lboost_system -lboost_program_options -lboost_thread -lboost_timer -lboost_chrono
SOURCES=$(wildcard *.cpp)
OBJECTS=$(SOURCES:.cpp=.o)
DEPS=$(OBJECTS:.o=.d)
EXECUTABLE=ssfi

all: $(SOURCES) $(EXECUTABLE)

-include $(DEPS)

%.o: %.cpp
	$(CXX) -MMD $(CXXFLAGS) $< -c -o $@

$(EXECUTABLE): $(OBJECTS) 
	$(CXX) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@

clean:
	$(RM) *.o *.d *.gch *.txt $(EXECUTABLE)
