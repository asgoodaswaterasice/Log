CC       = g++
CXXFLAGS = -I/usr/local/include -I. -I./common  -I${HOME}/message  -Wall -g
LDFLAGS  = -L/usr/local/lib  -L${HOME}/wiwo/libs -levent -lmessage -levent_pthreads

COMMON_PATH=./common
SRCDIR = .
SRCS = $(wildcard $(SRCDIR)/*.cc $(SRCDIR)/*.cpp $(COMMON_PATH)/*.cpp)
OBJS = $(addsuffix .o, $(basename $(SRCS)))
HEADS = $(addsuffix .h, $(basename $(SRCS)))
APP=gate

all:$(APP)


$(APP):$(OBJS)
	  $(CC) $(LDFLAGS) $^ -o $@

%.o:%.cpp
	    $(CC) $(CXXFLAGS) -c $^ -o $@

clean:
	    rm -f $(OBJS) $(APP)
.PHONY: all clean
