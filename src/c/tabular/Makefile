
EXECUTABLE_BASE=ccscan
BUILD_ARCH=$(shell uname -m)

ifdef DEBUG
CFLAGS+=-g -D_DEBUG
else
CFLAGS+=-O3 -DNDEBUG
endif

CFLAGS+=-Wall -std=c99

# c99 necessary to use the inline keyword.
# But note from https://gcc.gnu.org/onlinedocs/gcc/Inline.html#Inline:
#  GCC does not inline any functions when not optimizing unless you specify
#  the 'always_inline' attribute for the function, like this: 
#  // Prototype.
#  inline void foo (const char) __attribute__((always_inline));

CFLAGS += -D_POSIX_C_SOURCE=200809L
# _POSIX_SOURCE sufficient for localtime_r.
# getline,getdelim requires _POSIX_C_SOURCE=200809L.

ifdef DEBUG
EXECUTABLE=$(EXECUTABLE_BASE)
else
EXECUTABLE=$(EXECUTABLE_BASE)
endif

############################################################################

OBJECTS=main.o\
	scan.o\
	bstring.o\
	pptable.o\
	utf8.o\
	sspp.o\
	format.o\
	line.o\
	strset.o\
	ascii.o\
	murmur3.o \
	json.o \
	util.o

############################################################################
# Rules

all : $(EXECUTABLE)

# Following object dependencies use GNU make's implicit rules.

# Core modules.

main.o :
scan.o : 
bstring.o :
pptable.o :
sspp.o : sspp.h

$(EXECUTABLE) : $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ -lm

############################################################################
# Unit tests

UNITTESTS=$(addprefix ut-,format sspp csv strset)

allunit : $(UNITTESTS)

ut-format : format.c
	$(CC) -o $@ $(CFLAGS) -DUNIT_TEST_FORMAT -D_POSIX_C_SOURCE=200809L $^

ut-sspp : sspp.c
	$(CC) -o $@ $(CFLAGS) -DUNIT_TEST_SSPP -D_POSIX_C_SOURCE=200809L $^

ut-csv : csv.c utf8.c
	$(CC) -o $@ $(CFLAGS) -DUNIT_TEST_CSV -D_POSIX_C_SOURCE=200809L $^

ut-strset : strset.c murmur3.c
	$(CC) -o $@ $(CFLAGS) -DUNIT_TEST_STRSET -D_POSIX_C_SOURCE=200809L $^

at-strset : strset.c murmur3.c
	$(CC) -o $@ $(CFLAGS) -DAUTO_TEST_STRSET -D_POSIX_C_SOURCE=200809L $^

ut-stats : stats.c
	$(CC) -o $@ $(CFLAGS) -DUNIT_TEST_STATS -D_POSIX_C_SOURCE=200809L $^

ut-recstats : recstats.c
	$(CC) -o $@ $(CFLAGS) -D_POSIX_C_SOURCE=200809L $^

############################################################################

clean : 
	rm -f *.o ut-* $(EXECUTABLE_BASE)*

.PHONY : all clean

