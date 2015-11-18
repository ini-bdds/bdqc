
# Make "DEBUG" the default for test code such as this.
ifdef DEBUG
CFLAGS=-g -D_DEBUG
else
CFLAGS=-O3 -DNDEBUG
endif

CPPFLAGS+=-D_POSIX_C_SOURCE=200809L
CPPFLAGS+=-DHAVE_VERIFY_PARTIAL_SORT_ASSUMPTION

CFLAGS+=-Wall -std=c99 -save-temps 
LDLIBS=-lm

all : test-nmc test-uint32_t test-float

test-nmc : mcnaive.c
	$(CC) -o $@ $(CFLAGS) $(CPPFLAGS) -DUNIT_TEST_NMC $< $(LDLIBS)

test-uint32_t : qsel.c
	gcc -o $@ $(CFLAGS) $(CPPFLAGS) -DUNIT_TEST_QSEL -Disfp=false -Dqselnum_t=uint32_t $< $(LDLIBS)

test-float : qsel.c
	gcc -o $@ $(CFLAGS) $(CPPFLAGS) -DUNIT_TEST_QSEL -Disfp=true -Dqselnum_t=float $< $(LDLIBS)

clean :
	rm -f test-* *.o *.i *.s

.PHONY : clean

