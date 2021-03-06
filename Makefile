CC = gcc
TARGET = blorbsnd4
OBJECT = $(TARGET).o
FLAGS = -lao -ldl -lm -g -ggdb
WARN = -Wall

SNDFLAGS = -lsndfile
MODFLAGS = -lmodplug
OGGFLAGS = -lvorbisfile

MYFLAGS = $(SNDFLAGS) $(MODFLAGS) $(OGGFLAGS)


#$(TARGET):	$(TARGET).c
#	$(CC) -o $(TARGET) $(TARGET).c blorblib.a $(FLAGS) $(MYFLAGS)

blorbsnd4:	blorbsnd4.c blorblib
	$(CC) -o blorbsnd4 blorbsnd4.c blorblib.a $(FLAGS) $(MYFLAGS)

blorbsnd5:	blorbsnd5.c blorblib
	$(CC) -o blorbsnd5 blorbsnd5.c blorblib.a $(FLAGS) $(MYFLAGS)

blorblib:
	$(CC) -o blorblib.o -c blorblib.c
	ar rc blorblib.a blorblib.o
	ranlib blorblib.a

playaiff9:	playaiff9.c
	$(CC) -o playaiff9 playaiff9.c -lao -ldl -lm -lsndfile -lsamplerate
	
clean:
	rm -f *.o $(TARGET) playaiff1 playaiff2 playaiff3 playaiff4 playaiff5
	rm -f playaiff6 playaiff7 playaiff8 playaiff9
	rm -f playmod1 playmod2
	rm -f playogg
	rm -f threadtest3 threadtest4
