CC=mpicc
CFLAGS=-l jpeg

all: jpeg2raw raw2jpeg

jpeg2raw: jpeg2raw.c
	$(CC) -o $(BUILD)/jpeg2raw jpeg2raw.c $(CFLAGS)

raw2jpeg: raw2jpeg.c
	$(CC) -o $(BUILD)/raw2jpeg raw2jpeg.c $(CFLAGS)

clean:
	rm $(BUILD)/jpeg2raw
	rm $(BUILD)/raw2jpeg
