ROOT = .
DLL = dylib
A = a
BASEKIT_LIB = src/basekit/_build/lib/libbasekit.$(A)
CORO_LIB = src/coroutine/_build/lib/libcoroutine.$(A)

all: lib/libcoroutine.$(A)
	cd src/scal && make

lib/libcoroutine.$(A): $(CORO_LIB)
	cp $(CORO_LIB) lib
	
$(CORO_LIB): $(BASEKIT_LIB)
	cd src/coroutine && make

$(BASEKIT_LIB):
	cd src/basekit && make
	
clean:
	rm -rf lib/libcoroutine.$(A)
	cd src/basekit && make clean
	cd src/coroutine && make clean
	cd src/scal && make clean
