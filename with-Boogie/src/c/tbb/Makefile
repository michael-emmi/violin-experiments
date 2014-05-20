UNAME   = $(shell uname -m)
GCCARCH = -march=$(UNAME)
CXX=clang++
LINK=llvm-link

TBBDIR = /home/jad/src/tbb41_20130516oss/include
INC = -I$(TBBDIR)

CFLAGS=-c -emit-llvm $(GCCARCH) $(INC)

BCDIR = bcs
SRCDIR = src

_SOURCES = tbbQueue.cpp\
	concurrent_queue.cpp concurrent_monitor.cpp semaphore.cpp\
	cache_aligned_allocator.cpp dynamic_link.cpp tbb_misc.cpp

SOURCES = $(patsubst %cpp,$(SRCDIR)/%.cpp,$(_SOURCES))
BCS = $(patsubst %.cpp,$(BCDIR)/%.bc,$(_SOURCES))

EXEC = tbbQueue
LINKED = $(EXEC).bc

.PHONY: bc

bc: $(LINKED)

$(LINKED): $(BCS)
	$(LINK) -o $@ $^

$(BCDIR):
	mkdir -p $@

$(BCDIR)/%.bc: $(SRCDIR)/%.cpp | $(BCDIR)
	$(CXX) $(CFLAGS) -o $@ $<

.PHONY: clean

clean:
	rm -f $(BCS) $(EXEC) $(EXEC).s

.PHONY: exec

exec: $(EXEC)

$(EXEC): $(LINKED)
	llc $^
	$(CXX) $@.s -o $@
	