
CLAGS= -Wall -ansi -pedantic -errors -std=gnu99

CC = gcc

OBJFILES = graph.o server.o
TARGET = server
TARGET2 = client
OBJFILES2 = client.o

all: $(TARGET) $(TARGET2)

$(TARGET) : $(OBJFILES)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) -pthread -lm

$(TARGET2): $(OBJFILES2)
	$(CC) $(CFLAGS) -o $(TARGET2) $(OBJFILES2)

clean:
	rm -f $(OBJFILES) $(OBJFILES2) $(TARGET) $(TARGET2) *~