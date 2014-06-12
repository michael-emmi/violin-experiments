ROOT = .
DLL = dylib
A = a
BASEKIT_LIB = src/basekit/_build/lib/libbasekit.$(A)
CORO_LIB = src/coroutine/_build/lib/libcoroutine.$(A)

all: scal

scal: lib/libcoroutine.$(A)
	@echo Building scal
	@cd src/scal && make

lib/libcoroutine.$(A): $(CORO_LIB)
	@mkdir -p lib
	@cp $(CORO_LIB) lib
	
$(CORO_LIB): $(BASEKIT_LIB)
	@echo Building coroutine library
	@cd src/coroutine && make

$(BASEKIT_LIB):
	@echo Building basekit library
	@cd src/basekit && make
	
clean:
	@echo Removing make-generated files
	@rm -rf lib
	@cd src/basekit && make clean
	@cd src/coroutine && make clean
	@cd src/scal && make clean
