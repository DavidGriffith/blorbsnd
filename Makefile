CC = gcc
TARGET = blorbsnd5
OBJECT = $(TARGET).o blorblib.o
FLAGS = -lao -ldl -lm
WARN = -Wall

SNDFLAGS = -lsndfile
MODFLAGS = -lmodplug
OGGFLAGS = -lvorbisfile

MYFLAGS = $(SNDFLAGS) $(MODFLAGS) $(OGGFLAGS)

.SUFFIXES:
.SUFFIXES: .c .o .h


$(TARGET): $(OBJECT)
	$(CC) -o $(TARGET) $(OBJECT) $(FLAGS) $(MYFLAGS)

$(OBJECT): %.o: %.c
	$(CC) -o $@ -c $< $(WARN)


playaiff1: playaiff1.o
	$(CC) -o $@ $< $(FLAGS)

playaiff2: playaiff2.o
	$(CC) -o $@ $< $(FLAGS) $(SNDFLAGS)

playaiff3: playaiff3.o
	$(CC) -o $@ $< $(FLAGS) $(SNDFLAGS)

playaiff4: playaiff4.o
	$(CC) -o $@ $< $(FLAGS) $(SNDFLAGS)

playaiff5: playaiff5.o
	$(CC) -o $@ $< $(FLAGS) $(SNDFLAGS)

playaiff6: playaiff6.o
	$(CC) -o $@ $< $(FLAGS) $(SNDFLAGS)

playmod1: playmod1.o
	$(CC) -o $@ $< $(FLAGS) $(MODFLAGS)

playmod2: playmod2.o
	$(CC) -o $@ $< $(FLAGS) $(MODFLAGS)

playogg: playogg.o
	$(CC) -o $@ $< $(FLAGS) $(OGGFLAGS)

	
clean:
	rm -f *.o $(TARGET) playaiff1 playaiff2 playaiff3 playaiff4 playaiff5
	rm -f playmod1 playmod2
	rm -f playogg
