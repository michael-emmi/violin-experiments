ROOT = ../..
CC = clang++
CCFLAGS = -std=c++0x
INCLUDE = -I.
INCLUDE += -I$(ROOT)/include
INCLUDE += -I$(ROOT)/src/basekit/source
INCLUDE += -I$(ROOT)/src/coroutine/source
LIBS = -L$(ROOT)/lib
LIBS += -lcoroutine
LIBS += -lgflags
LIB_OBJS = $(patsubst %.cc,%.o,$(wildcard util/*.cc))
LIBS += $(LIB_OBJS)

EXE = scal-test

top: $(EXE)

util/%.o: util/%.cc
	$(CC) -c $(CCFLAGS) $(INCLUDE) $<
	mv $(@F) util

$(EXE): $(LIB_OBJS)
	$(CC) $(CCFLAGS) $(INCLUDE) $(LIBS) test.cpp -o $@

clean:
	rm -rf $(EXE)
	rm -rf $(wildcard **/*.o)
