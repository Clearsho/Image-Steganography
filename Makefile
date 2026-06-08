CC = gcc

CFLAGS = -Wall

SRC = test_encode.c encode.c decode.c

OUT = stego

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT)

clean:
	rm -f $(OUT)