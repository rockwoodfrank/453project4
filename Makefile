CC = gcc
CFLAGS = -Wall -std=gnu99 -pedantic -g
PROGS = tinyFSDemo
TESTPROGS = libDiskTest basicDiskTest runBasicDiskTest basicTinyFSTest runBasicTinyFSTest tinyFSTest
OBJS = tinyFSDemo.o tinyFS.o libDisk.o
DISKOBJS = disk0.dsk disk1.dsk disk2.dsk disk3.dsk

$(PROGS): $(OBJS)
	$(CC) $(CFLAGS) -o $(PROGS) $(OBJS)

clean:
	rm -rf $(PROGS)
	rm -f $(OBJS) *~ TAGS
	rm -rf $(TESTPROGS)
	rm -rf $(DISKOBJS)
	#rm -rf testFiles/*.dsk

declutter: 
	rm -f $(OBJS) *~ TAGS
	rm -f *.o

tinyFsDemo.o: tinyFSDemo.c libTinyFS.h tinyFS.h tinyFS_errno.h tinyFS.o
	$(CC) $(CFLAGS) -c -o $@ $<

tinyFS.o: tinyFS.c tinyFS.h libDisk.h libDisk.o tinyFS_errno.h
	$(CC) $(CFLAGS) -c -o $@ $<

libDisk.o: libDisk.c libDisk.h tinyFS.h tinyFS_errno.h
	$(CC) $(CFLAGS) -c -o $@ $<

# Testing
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
	echo \> basicTinyFSTestPassed.

libDiskTest: libDisk.h libDisk.o libDiskTest.c
	$(CC) $(CFLAGS) -o libDiskTest libDisk.o libDiskTest.c

tinyFSTest: tinyFS.h libDisk.h tinyFS.o libDisk.o tinyFSTest.c
	$(CC) $(CFLAGS) -o tinyFSTest tinyFS.o libDisk.o tinyFSTest.c

timeStampTest: tinyFS.h libDisk.h tinyFS.o libDisk.o timeStampTest.c
	$(CC) $(CFLAGS) -o timeStampTest tinyFS.o libDisk.o timeStampTest.c

consistencyCheckTest: tinyFS.h libDisk.h tinyFS.o libDisk.o consistencyCheckTest.c
	$(CC) $(CFLAGS) -o consistencyCheckTest tinyFS.o libDisk.o consistencyCheckTest.c

unitTests: libDiskTest tinyFSTest timeStampTest consistencyCheckTest
	./libDiskTest
	./tinyFSTest
	./timeStampTest
	./consistencyCheckTest

# Add any commands to run tests here, then we have a single command to run all tests.
test: clean unitTests runBasicDiskTest runBasicTinyFSTest
	$(info All tests passed!)

tarball: clean
	tar -czvf project4.tar.gz ./