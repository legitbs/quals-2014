CC=gcc
CCFLAGS=-g3 -Wall -w -pedantic -fmessage-length=0 -fno-stack-protector

O_FILES = main.o malloc.o

babyheap : $(O_FILES)
	$(CC) $(CCFLAGS) -o babyheap $(O_FILES)

# dependency stuff, see: http://www.cs.berkeley.edu/~smcpeak/autodepend/autodepend.html
# pull in dependency info for *existing* .o files
-include $(O_FILES:.o=.d)

.SUFFIXES : .o .c

.c.o :  
	$(CC) $(CCFLAGS) -c $<
	$(CC) -MM $(CFLAGS) $*.c > $*.d

clean:
	rm -f babyheap *.o *.d
