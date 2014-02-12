CXX=clang++
LINK=llvm-link

CXXFLAGS=-c -emit-llvm -DNUM_ITEMS=10 -DNUM_THREADS=6 -DKEYS=100

BCDIR = bcs
SRCDIR = src

_SOURCES = LockFreePriorityQueue.cpp

SOURCES = $(patsubst %cpp,$(SRCDIR)/%.cpp,$(_SOURCES))
BCS = $(patsubst %.cpp,$(BCDIR)/%.bc,$(_SOURCES))

EXEC = LockFreeDSQueue
LINKED = $(EXEC).bc

.PHONY: bc

bc: $(LINKED)

$(LINKED): $(BCS)
	$(LINK) -o $@ $^

$(BCDIR):
	mkdir $@

$(BCDIR)/%.bc: $(SRCDIR)/%.cpp | $(BCDIR)
	$(CXX) $(CXXFLAGS) -o $@ $<

.PHONY: clean

clean:
	rm -f $(BCS) $(EXEC) $(EXEC).s

.PHONY: exec

exec: $(EXEC)

$(EXEC): $(LINKED)
	llc $^
	$(CXX) $@.s -o $@
	