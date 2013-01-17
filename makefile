CC=g++
CFLAGS=-g -c -Wall -fexceptions -D_FILE_OFFSET_BITS=64 -O2
LDFLAGS=-lpthread
SOURCES=main.cpp ParamReader.cpp Vocab.cpp ivfpq_new.cpp entry.cpp  Index.cpp SearchEngine.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=ndk

all: $(OBJECTS)
	$(CC) $(OBJECTS) -o $(EXECUTABLE) $(LDFLAGS)

main.o:
	$(CC) $(CFLAGS) main.cpp
Search.o:
	$(CC) $(CFLAGS) SearchEngine.cpp
Index.o:
	$(CC) $(CFLAGS) Index.cpp
ParamReader.o:
	$(CC) $(CFLAGS) ParamReader.cpp
Vocab.o:
	$(CC) $(CFLAGS) Vocab.cpp
Entry.o:
	$(CC) $(CFLAGS) Entry.cpp
Ivfpq_new.o:
	$(CC) $(CFLAGS) ivfpq_new.cpp
HE.o:
	$(CC) $(CFLAGS) HE.cpp
clean:
	rm -rf $(OBJECTS)
	rm -rf $(EXECUTABLE)
