ROOT = ../..
CC = clang++
A = a
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

MAIN = scal.cpp
EXE = $(basename $(MAIN))

$(EXE): $(LIB_OBJS) $(ROOT)/lib/libcoroutine.$(A) $(MAIN)
	$(CC) $(CCFLAGS) $(INCLUDE) $(LIBS) $(MAIN) -o $@

util/%.o: util/%.cc
	$(CC) -c $(CCFLAGS) $(INCLUDE) $<
	mv $(@F) util

$(ROOT)/lib/libcoroutine.$(A):
	cd $(ROOT) && make lib/libcoroutine.$(A)

clean:
	rm -rf $(EXE)
	rm -rf $(wildcard **/*.o)