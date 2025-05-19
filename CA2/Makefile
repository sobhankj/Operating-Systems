CC = g++
CFLAGS = -std=c++17 -Wall

all: main warehouse item

main: main.cpp
	$(CC) $(CFLAGS) main.cpp -o main

warehouse: warehouse.cpp
	$(CC) $(CFLAGS) warehouse.cpp -o warehouse

item: item.cpp
	$(CC) $(CFLAGS) item.cpp -o item

clean:
	rm -f main warehouse item
