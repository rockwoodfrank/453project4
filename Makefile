CC = gcc
CFLAGS = -Wall -std=gnu99 -pedantic -g
PROG = tinyFSDemo
OBJS = tinyFSDemo.o libTinyFS.o libDisk.o

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS)

clean:
	rm -rf $(PROGS)
	rm -f $(OBJS) *~ TAGS

declutter: 
	rm -f $(OBJS) *~ TAGS

tinyFsDemo.o: tinyFSDemo.c libTinyFS.h tinyFS.h tinyFS_errno.h
	$(CC) $(CFLAGS) -c -o $@ $<

libTinyFS.o: libTinyFS.c libTinyFS.h tinyFS.h libDisk.h libDisk.o tinyFS_errno.h
	$(CC) $(CFLAGS) -c -o $@ $<

libDisk.o: libDisk.c libDisk.h tinyFS.h tinyFS_errno.h
	$(CC) $(CFLAGS) -c -o $@ $<

basicDiskTest:
	$(CC) $(CFLAGS) -o basic $(OBJS) basicDiskTest.c 

tarball: clean
	tar -czvf project4.tar.gz ./