CC = gcc
TARGET = blorbsnd4
OBJECT = $(TARGET).o blorblib.o
FLAGS = -lao -ldl -lm -lsndfile -lmodplug

.SUFFIXES:
.SUFFIXES: .c .o .h


$(TARGET): $(OBJECT)
	$(CC) -o $(TARGET) $(OBJECT) $(FLAGS)

$(OBJECT): %.o: %.c
	$(CC) -o $@ -c $<


playaiff1: playaiff1.o
	$(CC) -o $@ $< $(FLAGS)

playaiff2: playaiff2.o
	$(CC) -o $@ $< $(FLAGS)

playaiff3: playaiff3.o
	$(CC) -o $@ $< $(FLAGS)

playaiff4: playaiff4.o
	$(CC) -o $@ $< $(FLAGS)

playaiff5: playaiff5.o
	$(CC) -o $@ $< $(FLAGS)

playmod1: playmod1.o
	$(CC) -o $@ $< $(FLAGS)

playmod2: playmod2.o
	$(CC) -o $@ $< $(FLAGS)

	
clean:
	rm -f *.o $(TARGET) playaiff1 playaiff2 playaiff3 playaiff4 playaiff5
	rm -f playmod1 playmod2
