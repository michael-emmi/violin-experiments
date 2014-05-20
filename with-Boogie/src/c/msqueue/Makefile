CC=gcc
CFLAG= -std=c11 -pthread

SRCDIR = src
_IMPL = queue_lock_mutex.c queue_lock_mutex_bugged.c
_HARNESSES = 2xEnqueue_0-2xDequeue.c

EXECS = $(patsubst %.c,%,$(_HARNESSES))
EXECSBUGGED = $(patsubst %.c,%_bugged,$(_HARNESSES))

.PHONY: exec

exec: $(EXECS) $(EXECSBUGGED)

$(EXECS): %: $(SRCDIR)/%.c $(SRCDIR)/queue_lock_mutex.c
	$(CC) $(CFLAG) -o $@ $^
	
$(EXECSBUGGED): %_bugged: $(SRCDIR)/%.c $(SRCDIR)/queue_lock_mutex_bugged.c
	$(CC) $(CFLAG) -o $@ $^

.PHONY: clean

clean:
	rm -f $(EXECS) $(EXECSBUGGED)
	