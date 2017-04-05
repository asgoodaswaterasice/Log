CC       = g++
CXXFLAGS = -I/usr/local/include -I. -Wall -g
LDFLAGS  = -L/usr/local/lib -levent

APP=gate

all:$(APP)

gate:gate.o
	  $(CC) $(LDFLAGS) $^ -o $@


%.o:%.cpp
	    $(CC) $(CXXFLAGS) -c $^ -o $@

clean:
	    rm -f *.o $(APP)
.PHONY: all clean
