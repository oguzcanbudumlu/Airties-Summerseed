CC=gcc
SRC1=tx_raw.c
SRC2=rx_raw.c
TARGET1=tx_raw
TARGET2=rx_raw
OBJECT=myether.o

all: receive transmit

myether:
	$(CC) myether.c -c

receive: myether
	$(CC) $(SRC2) $(OBJECT) -o $(TARGET2)

transmit: myether
	$(CC) $(SRC1) $(OBJECT) -o $(TARGET1)

clean:
	rm $(OBJECT) $(TARGET1) $(TARGET2)
