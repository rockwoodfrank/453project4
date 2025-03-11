CC = gcc

CFLAGS = -Wall -std=gnu99 -pedantic -g

PROGS = tinyFSDemo

TESTPROGS = libDiskTest basicDiskTest runBasicDiskTest basicTinyFSTest runBasicTinyFSTest tinyFSTest timeStampTest consistencyCheckTest basicDisk basicFS

OBJS =  tinyFS.o libDisk.o libTinyFS_helpers.o

DISKOBJS = disk0.dsk disk1.dsk disk2.dsk disk3.dsk demo.dsk

TFSHEADERS = libTinyFS.h tinyFS.h tinyFS_errno.h libTinyFS_helpers.h

all: tinyFSDemo

clean:
	rm -rf $(PROGS)
	rm -f $(OBJS) *~ TAGS
	rm -rf $(TESTPROGS)
	rm -rf $(DISKOBJS)

rmdemodisk: 
	rm -rf demo.dsk

tinyFSDemo: tinyFSDemo.c $(TFSHEADERS) tinyFS.o libDisk.o libTinyFS_helpers.o
	$(CC) $(CFLAGS) -o tinyFSDemo tinyFSDemo.c $(TFSHEADERS) tinyFS.o libDisk.o libTinyFS_helpers.o

tinyFS.o: tinyFS.c $(TFSHEADERS) libDisk.o libTinyFS_helpers.o
	$(CC) $(CFLAGS) -c -o $@ $<

libTinyFS_helpers.o: libTinyFS_helpers.c $(TFSHEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

libDisk.o: libDisk.c libDisk.h tinyFS.h tinyFS_errno.h
	$(CC) $(CFLAGS) -c -o $@ $<

tarball: clean
	tar -czvf project4.tar.gz ./

# TESTING

basicDiskTest: $(OBJS) basicDiskTest.c
	$(CC) $(CFLAGS) -o basicDisk $(OBJS) basicDiskTest.c 

runBasicDiskTest: basicDiskTest
	rm -f $(DISKOBJS)
	./basicDisk | diff testOutputs/basicDiskTestOutput1.txt -
	./basicDisk | diff testOutputs/basicDiskTestOutput2.txt -
	echo \> basicDiskTest passed.

basicTinyFSTest: $(OBJS) basicTinyFSTest.c
	$(CC) $(CFLAGS) -o basicFS $(OBJS) basicTinyFSTest.c

runBasicTinyFSTest: basicTinyFSTest
	./basicFS
	./basicFS
	echo \> basicTinyFSTestPassed.

libDiskTest: libDisk.h libDisk.o libDiskTest.c 
	$(CC) $(CFLAGS) -o libDiskTest libDisk.o libDiskTest.c

tinyFSTest: tinyFS.h libDisk.h tinyFS.o libDisk.o tinyFSTest.c libTinyFS_helpers.o
	$(CC) $(CFLAGS) -o tinyFSTest tinyFS.o libDisk.o tinyFSTest.c libTinyFS_helpers.o

timeStampTest: tinyFS.h libDisk.h tinyFS.o libDisk.o timeStampTest.c libTinyFS_helpers.o
	$(CC) $(CFLAGS) -o timeStampTest tinyFS.o libDisk.o timeStampTest.c libTinyFS_helpers.o

consistencyCheckTest: tinyFS.h libDisk.h tinyFS.o libDisk.o consistencyCheckTest.c libTinyFS_helpers.o
	$(CC) $(CFLAGS) -o consistencyCheckTest tinyFS.o libDisk.o consistencyCheckTest.c libTinyFS_helpers.o

unitTests: libDiskTest tinyFSTest timeStampTest consistencyCheckTest
	./libDiskTest
	./tinyFSTest
	./timeStampTest
	./consistencyCheckTest

# Add any commands to run tests here, then we have a single command to run all tests.
test: clean unitTests runBasicDiskTest runBasicTinyFSTest
	$(info All tests passed!)
