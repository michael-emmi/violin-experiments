
ROOT = ../..
CC = clang++
CCFLAGS = -std=c++0x
INCLUDE = -I.
INCLUDE += -I$(ROOT)/include
INCLUDE += -I$(ROOT)/include/basekit
INCLUDE += -I$(ROOT)/include/coroutine
LIBS = -L$(ROOT)/lib
LIBS += -lcoroutine
LIBS += -lgflags
LIB_OBJS = $(patsubst %.cc,%.o,$(wildcard util/*.cc))
LIBS += $(LIB_OBJS)

EXE = scal-test

util/%.o: util/%.cc
	$(CC) -c $(CCFLAGS) $(INCLUDE) $<
	mv $(@F) util

$(EXE): $(LIB_OBJS)
	$(CC) $(CCFLAGS) $(INCLUDE) $(LIBS) test.cpp -o $@

clean:
	rm -d $(EXE)
	rm -d $(wildcard **/*.o)
	