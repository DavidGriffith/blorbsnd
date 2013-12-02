CC = gcc
OBJECT = blorbsnd.o blorblib.o
TARGET = blorbsnd
FLAGS = -lao -ldl -lm

.SUFFIXES:
.SUFFIXES: .c .o .h


$(TARGET): $(OBJECT)
	$(CC) -o $(TARGET) $(OBJECT) $(FLAGS)

$(OBJECT): %.o: %.c
	$(CC) -o $@ -c $<

clean:
	rm -f *.o $(TARGET) 
