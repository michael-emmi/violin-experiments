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
DEPENDS += $(LIB_OBJS)
DEPENDS += $(wildcard datastructures/*.h)
DEPENDS += $(wildcard $(ROOT)/include/*.h)

JUST_SCAL = scal.cpp
VIOLIN_SCAL = violin-scal.cpp scal.cpp
EXE = $(basename $(VIOLIN_SCAL))
DYLIB = lib$(basename $(JUST_SCAL)).dylib

$(EXE): $(DEPENDS) $(ROOT)/lib/libcoroutine.$(A) $(VIOLIN_SCAL)
	@echo Building executable: $@
	@$(CC) $(CCFLAGS) $(INCLUDE) $(LIBS) $(VIOLIN_SCAL) -o $@

$(DYLIB): $(DEPENDS) $(ROOT)/lib/libcoroutine.$(A) $(JUST_SCAL)
	@echo Building dynamic library: $@
	@$(CC) -dynamiclib $(CCFLAGS) $(INCLUDE) $(LIBS) $(JUST_SCAL) -o $@

util/%.o: util/%.cc
	@$(CC) -c $(CCFLAGS) $(INCLUDE) $<
	@mv $(@F) util

$(ROOT)/lib/libcoroutine.$(A):
	@cd $(ROOT) && make lib/libcoroutine.$(A)

clean:
	@echo Removing make-generated files
	@rm -rf $(EXE)
	@rm -rf $(DYLIB)
	@rm -rf $(wildcard **/*.o)
