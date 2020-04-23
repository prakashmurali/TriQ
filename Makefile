CC=g++
Z3INCLUDE=
CXXFLAGS=-I$(Z3INCLUDE) -std=c++11

src = $(wildcard src/*.cpp)
obj = $(src:.cpp=.o)

LDFLAGS = -lz3 -lboost_system

%.o: %.c
	$(CC) $(CXXFLAGS) -o $@ -c $<

triq: $(obj)
	$(CC) -o $@ $^ $(LDFLAGS) 

.PHONY: clean
clean:
	rm -f $(obj) triq
