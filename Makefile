CC = gcc
CFLAGS = -Wall -g
PROG = tinyFSDemo
OBJS = tinyFSDemo.o libTinyFS.o libDisk.o

$(PROG): $(OBJS)
	$(CC) $(CFLAGS) -o $(PROG) $(OBJS)

clean:
	rm -rf $(PROGS)
	rm -f $(OBJS) *~ TAGS

declutter: 
	rm -f $(OBJS) *~ TAGS

tinyFsDemo.o: tinyFSDemo.c libTinyFS.h tinyFSDemo.h tinyFS_errno.h
	$(CC) $(CFLAGS) -c -o $@ $<

libTinyFS.o: libTinyFS.c libTinyFS.h tinyFSDemo.h libDisk.h libDisk.o tinyFS_errno.h
	$(CC) $(CFLAGS) -c -o $@ $<

libDisk.o: libDisk.c libDisk.h tinyFSDemo.h tinyFS_errno.h
	$(CC) $(CFLAGS) -c -o $@ $<

readBlock.o: readBlock.c readBlock.h
	$(CC) $(CFLAGS) -c -o $@ $<

test: readBlockTests.c readBlock.h readBlock.o 
	$(CC) $(CFLAGS) -o test readBlock.o readBlockTests.c

tarball: clean
	tar -czvf project4.tar.gz ./