CC = gcc
CFLAGS = -Wall -std=gnu99 -pedantic -g
PROG = tinyFSDemo
OBJS = tinyFSDemo.o tinyFS.o libDisk.o

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS)

clean:
	rm -rf $(PROGS)
	rm -f $(OBJS) *~ TAGS

declutter: 
	rm -f $(OBJS) *~ TAGS

tinyFsDemo.o: tinyFSDemo.c libTinyFS.h tinyFS.h tinyFS_errno.h
	$(CC) $(CFLAGS) -c -o $@ $<

tinyFS.o: tinyFS.c tinyFS.h libDisk.h libDisk.o tinyFS_errno.h
	$(CC) $(CFLAGS) -c -o $@ $<

libDisk.o: libDisk.c libDisk.h tinyFS.h tinyFS_errno.h
	$(CC) $(CFLAGS) -c -o $@ $<

basicDiskTest:
	$(CC) $(CFLAGS) -o basic $(OBJS) basicDiskTest.c 

unitTests: tinyFS.h libDisk.h tinyFS.o  libDisk.o unitTests.c
	$(CC) $(CFLAGS) -o unitTests tinyFS.o libDisk.o unitTests.c

# Add any commands to run tests here, then we have a single command to run all tests.
test: unitTests
	./unitTests

tarball: clean
	tar -czvf project4.tar.gz ./